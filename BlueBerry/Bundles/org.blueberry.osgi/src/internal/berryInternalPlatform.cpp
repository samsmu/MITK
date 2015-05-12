/*===================================================================

BlueBerry Platform

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include "berryInternalPlatform.h"
#include "berryLog.h"

#include <Poco/Exception.h>
#include <Poco/File.h>
#include <Poco/FileStream.h>
#include <Poco/AutoPtr.h>
#include <Poco/Util/PropertyFileConfiguration.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Util/OptionException.h>

#include <ctkPluginFrameworkLauncher.h>
#include <ctkPluginFrameworkFactory.h>
#include <ctkPluginFramework.h>
#include <ctkPluginContext.h>
#include <ctkPlugin.h>
#include <ctkPluginException.h>

#include <iostream>

#include "berryPlatform.h"
#include "berryPlatformException.h"
#include "berryDebugUtil.h"
#include "event/berryPlatformEvents.h"
#include "berryPlatformLogChannel.h"
#include "berryIBundle.h"
#include "berryCodeCache.h"
#include "berryBundleLoader.h"
#include "berrySystemBundle.h"
#include "berryBundleDirectory.h"
#include "berryProvisioningInfo.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDebug>

namespace berry {

Poco::Mutex InternalPlatform::m_Mutex;

InternalPlatform::InternalPlatform() : m_Initialized(false), m_Running(false),
  m_ConsoleLog(false), m_ServiceRegistry(0),
  m_CodeCache(0), m_BundleLoader(0), m_SystemBundle(0), m_PlatformLogger(0),
  m_ctkPluginFrameworkFactory(0),
  m_EventStarted(PlatformEvent::EV_PLATFORM_STARTED)
{
}

InternalPlatform::~InternalPlatform()
{

}

InternalPlatform* InternalPlatform::GetInstance()
{
  Poco::Mutex::ScopedLock lock(m_Mutex);
  static InternalPlatform instance;
  return &instance;
}

bool InternalPlatform::ConsoleLog() const
{
  return m_ConsoleLog;
}

ctkPluginContext* InternalPlatform::GetCTKPluginFrameworkContext() const
{
  if (m_ctkPluginFrameworkFactory)
  {
    return m_ctkPluginFrameworkFactory->getFramework()->getPluginContext();
  }
  return 0;
}

ServiceRegistry& InternalPlatform::GetServiceRegistry()
{
  AssertInitialized();
  return *m_ServiceRegistry;
}

void InternalPlatform::Initialize(int& argc, char** argv, Poco::Util::AbstractConfiguration* config)
{
  // initialization
  Poco::Mutex::ScopedLock lock(m_Mutex);

  m_Argc = &argc;
  m_Argv = argv;

  try
  {
    this->init(argc, argv);
  }
  catch (const Poco::Util::UnknownOptionException& e)
  {
    BERRY_WARN << e.displayText();
  }
  this->loadConfiguration();
  if (config)
  {
    this->config().add(config, 50, false);
  }

  m_ServiceRegistry = new ServiceRegistry();

  m_ConsoleLog = this->GetConfiguration().hasProperty(Platform::ARG_CONSOLELOG);

  m_ConfigPath.assign(this->GetConfiguration().getString("application.configDir"));
  m_InstancePath.assign(this->GetConfiguration().getString("application.dir"));
  try
  {
    m_InstallPath.assign(this->GetConfiguration().getString(Platform::ARG_HOME));
  }
  catch (Poco::NotFoundException& )
  {
    m_InstallPath.assign(m_InstancePath);
  }

  if (this->GetConfiguration().hasProperty(Platform::ARG_STORAGE_DIR))
  {
    std::string dataLocation = this->GetConfiguration().getString(Platform::ARG_STORAGE_DIR, "");
    if (dataLocation.at(dataLocation.size()-1) != '/')
    {
      dataLocation += '/';
    }
    m_UserPath.assign(dataLocation);
  }
  else
  {
    // Append a hash value of the absolute path of the executable to the data location.
    // This allows to start the same application from different build or install trees.
    QString dataLocation = QDesktopServices::storageLocation(QDesktopServices::DataLocation) + '_';
    dataLocation += QString::number(qHash(QCoreApplication::applicationDirPath())) + "/";
	m_UserPath.assign(dataLocation.toUtf8().constData());
  }
  BERRY_INFO(m_ConsoleLog) << "Framework storage dir: " << m_UserPath.toString();

  Poco::File userFile(m_UserPath);

  try
  {
    userFile.createDirectories();
    userFile.canWrite();
  }
  catch(const Poco::IOException& e)
  {
    BERRY_WARN << e.displayText();
    m_UserPath.assign(Poco::Path::temp());
    m_UserPath.pushDirectory("." + this->commandName());
    userFile = m_UserPath;
  }

  // Initialize the CTK Plugin Framework
  ctkProperties fwProps;
  fwProps.insert(ctkPluginConstants::FRAMEWORK_STORAGE, QString::fromUtf8(userFile.path().c_str()));
  if (this->GetConfiguration().hasProperty(Platform::ARG_CLEAN))
  {
    fwProps.insert(ctkPluginConstants::FRAMEWORK_STORAGE_CLEAN, ctkPluginConstants::FRAMEWORK_STORAGE_CLEAN_ONFIRSTINIT);
  }
  if (this->GetConfiguration().hasProperty(Platform::ARG_CONSOLELOG))
  {
    fwProps.insert("org.commontk.pluginfw.debug.framework", true);
    fwProps.insert("org.commontk.pluginfw.debug.errors", true);
    fwProps.insert("org.commontk.pluginfw.debug.pluginfw", true);
    fwProps.insert("org.commontk.pluginfw.debug.lazy_activation", true);
    fwProps.insert("org.commontk.pluginfw.debug.resolve", true);
  }
  if (this->GetConfiguration().hasProperty(Platform::ARG_PRELOAD_LIBRARY))
  {
	  QString preloadLibs = QString::fromUtf8(this->GetConfiguration().getString(Platform::ARG_PRELOAD_LIBRARY).c_str());
    fwProps.insert(ctkPluginConstants::FRAMEWORK_PRELOAD_LIBRARIES, preloadLibs.split(',', QString::SkipEmptyParts));
  }
  m_ctkPluginFrameworkFactory = new ctkPluginFrameworkFactory(fwProps);
  QSharedPointer<ctkPluginFramework> pfw = m_ctkPluginFrameworkFactory->getFramework();
  ctkPluginContext* pfwContext = NULL;

  std::string provisioningFile = this->GetConfiguration().getString(Platform::ARG_PROVISIONING);
  if (!provisioningFile.empty())
  {
    BERRY_INFO(m_ConsoleLog) << "Using provisioning file: " << provisioningFile;

    ProvisioningInfo provInfo(provisioningFile.c_str());

    // it can still happen, that the encoding is not compatible with the fromUtf8 function ( i.e. when manipulating the LANG variable
    // in such case, the QStringList in provInfo is empty which we can easily check for
    if( provInfo.getPluginDirs().empty() )
    {
      BERRY_ERROR << "Cannot search for provisioning file, the retrieved directory list is empty.\n" <<
                     "This can occur if there are some special (non-ascii) characters in the install path.";
      throw berry::PlatformException("No provisioning file specified. Terminating...");
    }

    foreach(QString pluginPath, provInfo.getPluginDirs())
    {
      ctkPluginFrameworkLauncher::addSearchPath(pluginPath);
    }

    pfw->init();
    pfwContext = pfw->getPluginContext();

    bool forcePluginOverwrite = this->GetConfiguration().hasOption(Platform::ARG_FORCE_PLUGIN_INSTALL);
    QList<QUrl> pluginsToStart = provInfo.getPluginsToStart();
    foreach(QUrl pluginUrl, provInfo.getPluginsToInstall())
    {
      if (forcePluginOverwrite)
      {
        uninstallPugin(pluginUrl, pfwContext);
      }
      try
      {
		  BERRY_INFO(m_ConsoleLog) << "Installing CTK plug-in from: " << pluginUrl.toString().toUtf8().constData();
        QSharedPointer<ctkPlugin> plugin = pfwContext->installPlugin(pluginUrl);
        if (pluginsToStart.contains(pluginUrl))
        {
          m_CTKPluginsToStart << plugin->getPluginId();
        }
      }
      catch (const ctkPluginException& e)
      {
        QString errorMsg;
        QDebug dbg(&errorMsg);
        dbg << e.printStackTrace();
        BERRY_ERROR << qPrintable(errorMsg);
      }
    }
  }
  else
  {
    pfw->init();
    pfwContext = pfw->getPluginContext();
    BERRY_INFO << "No provisioning file set.";
  }

  m_BaseStatePath = m_UserPath;
  m_BaseStatePath.pushDirectory("bb-metadata");
  m_BaseStatePath.pushDirectory("bb-plugins");

  Poco::Path logPath(m_UserPath);
  logPath.setFileName(this->commandName() + ".log");
  m_PlatformLogChannel = new PlatformLogChannel(logPath.toString());
  m_PlatformLogger = &Poco::Logger::create("PlatformLogger", m_PlatformLogChannel, Poco::Message::PRIO_TRACE);

  try
  {
    m_CodeCache = new CodeCache(this->GetConfiguration().getString(Platform::ARG_PLUGIN_CACHE));
  }
  catch (Poco::NotFoundException&)
  {
    Poco::Path cachePath(m_UserPath);
    cachePath.pushDirectory("bb-plugin_cache");
    m_CodeCache = new CodeCache(cachePath.toString());
  }
  m_BundleLoader = new BundleLoader(m_CodeCache, *m_PlatformLogger);

  // tell the BundleLoader about the installed CTK plug-ins
  QStringList installedCTKPlugins;
  foreach(QSharedPointer<ctkPlugin> plugin, pfwContext->getPlugins())
  {
    installedCTKPlugins << plugin->getSymbolicName();
  }
  m_BundleLoader->SetCTKPlugins(installedCTKPlugins);

  m_Initialized = true;

  // Clear the CodeCache
  if (this->GetConfiguration().hasProperty(Platform::ARG_CLEAN))
    m_CodeCache->Clear();

  try
  {
    // assemble a list of base plugin-directories (which contain
    // the real plugins as directories)
    std::vector<std::string> pluginBaseDirs;

    Poco::StringTokenizer tokenizer(this->GetConfiguration().getString(Platform::ARG_PLUGIN_DIRS, ""), ";",
                                    Poco::StringTokenizer::TOK_IGNORE_EMPTY | Poco::StringTokenizer::TOK_TRIM);

    for (Poco::StringTokenizer::Iterator token = tokenizer.begin();
         token != tokenizer.end(); ++token)
    {
      pluginBaseDirs.push_back(*token);
    }

    std::vector<Poco::Path> pluginPaths;
    for (std::vector<std::string>::iterator pluginBaseDir = pluginBaseDirs.begin();
         pluginBaseDir != pluginBaseDirs.end(); ++pluginBaseDir)
    {
      BERRY_INFO(m_ConsoleLog) << "Plugin base directory: " << *pluginBaseDir;
      Poco::File pluginDir(*pluginBaseDir);

      if (!pluginDir.exists() || !pluginDir.isDirectory())
      {
        BERRY_WARN(m_ConsoleLog) << *pluginBaseDir << " is not a direcotry or does not exist. SKIPPED.\n";
        continue;
      }

      std::vector<std::string> pluginList;
      pluginDir.list(pluginList);

      std::vector<std::string>::iterator iter;
      for (iter = pluginList.begin(); iter != pluginList.end(); iter++)
      {
        Poco::Path pluginPath = Poco::Path::forDirectory(*pluginBaseDir);
        pluginPath.pushDirectory(*iter);

        Poco::File file(pluginPath);
        if (file.exists() && file.isDirectory())
        {
          pluginPaths.push_back(pluginPath);
        }
      }
    }

    std::vector<Poco::Path>::iterator pathIter;
    for (pathIter = pluginPaths.begin(); pathIter != pluginPaths.end(); pathIter++)
    {
      try
      {
      Bundle::Pointer bundle = m_BundleLoader->LoadBundle(*pathIter);
      if (bundle)
      {
        BERRY_INFO(m_ConsoleLog) << "Bundle state (" << pathIter->toString() << "): " << bundle->GetStateString() << std::endl;
      }
      }
      catch (const BundleStateException& exc)
      {
        BERRY_WARN << exc.displayText() << std::endl;
      }
    }

    // resolve plugins
    m_BundleLoader->ResolveAllBundles();
  }
  catch (Poco::Exception& exc)
  {
    this->logger().log(exc);
  }

#ifdef BLUEBERRY_DEBUG_SMARTPOINTER
  DebugUtil::RestoreState();
#endif

}

void InternalPlatform::uninstallPugin(const QUrl& pluginUrl, ctkPluginContext* pfwContext)
{
  QFileInfo libInfo(pluginUrl.toLocalFile());
  QString libName = libInfo.baseName();
  if (libName.startsWith("lib"))
  {
    libName = libName.mid(3);
  }
  QString symbolicName = libName.replace('_', '.');

  foreach(QSharedPointer<ctkPlugin> plugin, pfwContext->getPlugins())
  {
    if (plugin->getSymbolicName() == symbolicName &&
        plugin->getLocation() != pluginUrl.toString())
    {
		BERRY_WARN << "A plug-in with the symbolic name " << symbolicName.toUtf8().constData() <<
                    " but different location is already installed. Trying to uninstall " << plugin->getLocation().toUtf8().constData();
      plugin->uninstall();
      return;
    }
  }
}

void InternalPlatform::Launch()
{
  AssertInitialized();

  if (m_Running) return;

  m_Running = true;

  this->run();
}

void InternalPlatform::Shutdown()
{
  QSharedPointer<ctkPluginFramework> ctkPluginFW;

  {
    Poco::Mutex::ScopedLock lock(m_Mutex);
    AssertInitialized();
    DebugUtil::SaveState();
    ctkPluginFW = m_ctkPluginFrameworkFactory->getFramework();
    m_Initialized = false;
  }

  ctkPluginFW->stop();

  this->uninitialize();

  // wait 10 seconds for the CTK plugin framework to stop
  ctkPluginFW->waitForStop(10000);

  {
    Poco::Mutex::ScopedLock lock(m_Mutex);
    delete m_ServiceRegistry;
    delete m_BundleLoader;
    delete m_CodeCache;
  }
}


void InternalPlatform::AssertInitialized()
{
  if (!m_Initialized)
    throw Poco::SystemException("The Platform has not been initialized yet!");
}

IExtensionPointService::Pointer InternalPlatform::GetExtensionPointService()
{
  Poco::Mutex::ScopedLock lock(m_Mutex);
  this->AssertInitialized();

  return m_ServiceRegistry->GetServiceById<IExtensionPointService>(IExtensionPointService::SERVICE_ID);
}

const Poco::Path& InternalPlatform::GetConfigurationPath()
{
  return m_ConfigPath;
}

const Poco::Path& InternalPlatform::GetInstallPath()
{
  return m_InstallPath;
}

const Poco::Path& InternalPlatform::GetInstancePath()
{
  return m_InstancePath;
}

bool InternalPlatform::GetStatePath(Poco::Path& statePath, IBundle::Pointer bundle, bool create)
{
  statePath = m_BaseStatePath;
  statePath.pushDirectory(bundle->GetSymbolicName());
  try
  {
  Poco::File stateFile(statePath);
  if (!stateFile.exists() && create)
    stateFile.createDirectories();
  }
  catch (Poco::FileException&)
  {
    return false;
  }

  return true;
}

PlatformEvents& InternalPlatform::GetEvents()
{
  return m_Events;
}

const Poco::Path& InternalPlatform::GetUserPath()
{
  return m_UserPath;
}

bool InternalPlatform::IsRunning() const
{
  Poco::Mutex::ScopedLock lock(m_Mutex);
  return (m_Initialized && m_Running);
}

IBundle::Pointer InternalPlatform::GetBundle(const std::string& id)
{
  Poco::Mutex::ScopedLock lock(m_Mutex);

  AssertInitialized();

  return m_BundleLoader->FindBundle(id);
}

std::vector<IBundle::Pointer> InternalPlatform::GetBundles() const
{
  return m_BundleLoader->GetBundles();
}

Poco::Logger* InternalPlatform::GetLogger()
{
  return m_PlatformLogger;
}

Poco::Util::LayeredConfiguration& InternalPlatform::GetConfiguration() const
{
  return this->config();
}

std::vector<std::string> InternalPlatform::GetApplicationArgs() const
{
  return m_FilteredArgs;
}

int& InternalPlatform::GetRawApplicationArgs(char**& argv)
{
  argv = m_Argv;
  return *m_Argc;
}

void InternalPlatform::defineOptions(Poco::Util::OptionSet& options)
{
  Poco::Util::Option helpOption("help", "h", "print this help text");
  helpOption.callback(Poco::Util::OptionCallback<InternalPlatform>(this, &InternalPlatform::PrintHelp));
  options.addOption(helpOption);

  Poco::Util::Option newInstanceOption(Platform::ARG_NEWINSTANCE, "", "forces a new instance of this application");
  newInstanceOption.binding(Platform::ARG_NEWINSTANCE);
  options.addOption(newInstanceOption);

  Poco::Util::Option cleanOption(Platform::ARG_CLEAN, "", "cleans the plugin cache");
  cleanOption.binding(Platform::ARG_CLEAN);
  options.addOption(cleanOption);

  Poco::Util::Option appOption(Platform::ARG_APPLICATION, "", "the id of the application extension to be executed");
  appOption.argument("<id>").binding(Platform::ARG_APPLICATION);
  options.addOption(appOption);

  Poco::Util::Option storageDirOption(Platform::ARG_STORAGE_DIR, "", "the location for storing persistent application data");
  storageDirOption.argument("<dir>").binding(Platform::ARG_STORAGE_DIR);
  options.addOption(storageDirOption);

  Poco::Util::Option consoleLogOption(Platform::ARG_CONSOLELOG, "", "log messages to the console");
  consoleLogOption.binding(Platform::ARG_CONSOLELOG);
  options.addOption(consoleLogOption);

  Poco::Util::Option forcePluginOption(Platform::ARG_FORCE_PLUGIN_INSTALL, "", "force installing plug-ins with same symbolic name");
  forcePluginOption.binding(Platform::ARG_FORCE_PLUGIN_INSTALL);
  options.addOption(forcePluginOption);

  Poco::Util::Option preloadLibsOption(Platform::ARG_PRELOAD_LIBRARY, "", "preload a library");
  preloadLibsOption.argument("<library>").repeatable(true).callback(Poco::Util::OptionCallback<InternalPlatform>(this, &InternalPlatform::handlePreloadLibraryOption));
  options.addOption(preloadLibsOption);

  Poco::Util::Option testPluginOption(Platform::ARG_TESTPLUGIN, "", "the plug-in to be tested");
  testPluginOption.argument("<id>").binding(Platform::ARG_TESTPLUGIN);
  options.addOption(testPluginOption);

  Poco::Util::Option testAppOption(Platform::ARG_TESTAPPLICATION, "", "the application to be tested");
  testAppOption.argument("<id>").binding(Platform::ARG_TESTAPPLICATION);
  options.addOption(testAppOption);

  Poco::Util::Option xargsOption(Platform::ARG_XARGS, "", "Extended argument list");
  xargsOption.argument("<args>").binding(Platform::ARG_XARGS);
  options.addOption(xargsOption);

  Poco::Util::Application::defineOptions(options);
}

void InternalPlatform::handlePreloadLibraryOption(const std::string& name, const std::string& value)
{
  std::string oldVal;
  if (this->config().hasProperty(Platform::ARG_PRELOAD_LIBRARY))
  {
    oldVal = this->config().getString(Platform::ARG_PRELOAD_LIBRARY);
  }
  this->config().setString(Platform::ARG_PRELOAD_LIBRARY, oldVal + "," + value);
}

int InternalPlatform::main(const std::vector<std::string>& args)
{
  m_FilteredArgs = args;
  //m_FilteredArgs.insert(m_FilteredArgs.begin(), this->config().getString("application.argv[0]"));

  ctkPluginContext* context = GetCTKPluginFrameworkContext();
  QFileInfo storageDir = context->getDataFile("");
  BundleDirectory::Pointer bundleStorage(new BundleDirectory(Poco::Path(storageDir.absolutePath().toUtf8().constData())));
  SystemBundle::Pointer systemBundle(new SystemBundle(*m_BundleLoader, bundleStorage));
  if (systemBundle == 0)
    throw PlatformException("Could not find the system bundle");
  m_BundleLoader->m_SystemBundle = systemBundle;
  m_BundleLoader->LoadBundle(systemBundle);

  m_ctkPluginFrameworkFactory->getFramework()->start();
  foreach(long pluginId, m_CTKPluginsToStart)
  {
    BERRY_INFO(m_ConsoleLog) << "Starting CTK plug-in: " << context->getPlugin(pluginId)->getSymbolicName().toUtf8().constData()
                             << " [" << pluginId << "]";
    // do not change the autostart setting of this plugin
    context->getPlugin(pluginId)->start(ctkPlugin::START_TRANSIENT | ctkPlugin::START_ACTIVATION_POLICY);
  }

  m_BundleLoader->StartSystemBundle(systemBundle);

  systemBundle->Resume();

  return EXIT_OK;
}

void InternalPlatform::PrintHelp(const std::string&, const std::string&)
{
  Poco::Util::HelpFormatter help(this->options());
  help.setAutoIndent();
  help.setCommand(this->commandName());
  help.format(std::cout);

  exit(EXIT_OK);
}

}
