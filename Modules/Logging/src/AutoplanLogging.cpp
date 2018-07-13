#include "AutoplanLogging.h"

#include <iostream>
#include <cstdlib>
#ifndef _WIN32
///gethostname
#include <unistd.h>
#endif

// boost
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/move/utility.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/locale/encoding.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/locale.hpp>

#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/stream.hpp>

#include <boost/algorithm/string.hpp>

// boost log
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/common.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/support/date_time.hpp>

// boost log::sources
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

// boost log::sinks
#include <boost/log/sinks/text_file_backend.hpp>

// boost log::utility
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/phoenix/bind.hpp>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

struct ThrowAwayPattern _ = {};

namespace
{
  const char TIME_STAMP_FORMAT[] = "%Y-%m-%d %H:%M:%S";

  // convert to UTF8 (req for dumps)
  std::string MessageToUTF8(const boost::log::value_ref<std::string>& message)
  {
    if (!message) {
      return std::string();
    }

    std::string msg = *message;
#ifdef _WIN32
    return Utilities::convertLocalToUTF8(msg);
#else
    return msg;
#endif
  }

  // convert to UTF8, escape slashes (req for JSON format)
  std::string MessageForLogstash(const boost::log::value_ref<std::string>& message)
  {
    if (!message) {
      return std::string();
    }

    std::string msg = *message;
    boost::replace_all(msg, "\\", "\\\\");
    boost::replace_all(msg, "\n", "\\n");

#ifdef _WIN32
    return Utilities::convertLocalToUTF8(msg);
#else
    return msg;
#endif
  }

  std::string MessageToOEM(const boost::log::value_ref<std::string>& message)
  {
#ifdef _WIN32
    return message ? Utilities::convertLocalToOEM(*message) : std::string();
#else
    return message ? *message : std::string();
#endif
  }

  enum class NameType { USER, HOST };

#ifdef _WIN32
  const wchar_t*
#else
  const char*
#endif
  getEnvName(NameType type)
  {
    switch (type) {
    case NameType::USER:
#ifdef _WIN32
      return L"USERNAME";
#else
      return "USER";
#endif
    case NameType::HOST:
#ifdef _WIN32
      return L"COMPUTERNAME";
#else
      return "HOSTNAME";
#endif
    default:
      return nullptr;
    }
  }

  std::string getName(NameType type)
  {
    auto envName = getEnvName(type);
    auto name =
#ifdef _WIN32
      _wgetenv(envName);
#else
      std::getenv(envName);
#endif
    if (name){
#ifdef _WIN32
      return Utilities::convertToUtf8(name);
#else
      return name;
#endif
    }
    char temp[512] = {};
    return gethostname(temp, sizeof(temp)) ? std::string() : temp;
  }
}

namespace Logger
{
  typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_file_backend > file_sink;
  typedef boost::log::sinks::synchronous_sink< boost::log::sinks::text_ostream_backend > ostream_sink;
/*
  class SyncSSLClient {
  public:
    SyncSSLClient(const std::string& raw_ip_address, unsigned short port_num) :
                  m_ep(boost::asio::ip::address::from_string(raw_ip_address), port_num),
                  m_ssl_context(boost::asio::ssl::context::sslv23_client),
                  m_ssl_stream(m_ios, m_ssl_context) {

      m_ssl_context.set_default_verify_paths();

      //std::string keyPath = Options::get().logsPath + "key.pem";
      //m_ssl_context.load_verify_file(keyPath);
      //m_ssl_context.set_default_verify_paths();

      // Set verification mode and designate that
      // we want to perform verification.
      m_ssl_stream.set_verify_mode(boost::asio::ssl::verify_peer); //verify_none);

      // Set verification callback.
      m_ssl_stream.set_verify_callback([this](bool preverified,
                                              boost::asio::ssl::verify_context& context) -> bool {
          return on_peer_verify(preverified, context);
        });
    }
    void connect() {
      // Connect the TCP socket.
      m_ssl_stream.lowest_layer().connect(m_ep);
      // Perform the SSL handshake.
      //m_ssl_stream.handshake(boost::asio::ssl::stream_base::client);
      boost::asio::ip::tcp::resolver resolver(m_ios);
      boost::asio::ip::tcp::resolver::query query(Options::get().iphost, Options::get().ipport);

      boost::asio::connect(m_ssl_stream.lowest_layer(), resolver.resolve(query));
      // +
      m_ios.run();
    }
    void close() {
      // We ignore any errors that might occur
      // during shutdown as we anyway can't
      // do anything about them.
      boost::system::error_code ec;
      m_ssl_stream.shutdown(ec); // Shutdown SSL.

      // Shut down the socket.
      m_ssl_stream.lowest_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
      m_ssl_stream.lowest_layer().close(ec);
    }
    std::string emulateLongComputationOp(
      unsigned int duration_sec) {
      std::string request = "{EMULATE_LONG_COMP_OP "
                            + std::to_string(duration_sec)
                            + "}";
      sendRequest(request);
      return "OK"; //receiveResponse();
    }
  private:
    bool on_peer_verify(bool preverified, boost::asio::ssl::verify_context& context)
    {
      // Here the certificate should be verified and the
      // verification result should be returned.
      return true;
    }
    void sendRequest(const std::string& request) {
      boost::system::error_code ec;
      boost::asio::write(m_ssl_stream, boost::asio::buffer(request), ec);
      std::cout << ec.message() << "!!!" << std::endl;
    }
    std::string receiveResponse() {
      boost::asio::streambuf buf;
      boost::asio::read_until(m_ssl_stream, buf, '\n');
      std::istream input(&buf);
      std::string response;
      std::getline(input, response);
      return response;
    }
  private:
    boost::asio::io_service m_ios;
    boost::asio::ip::tcp::endpoint m_ep;

    boost::asio::ssl::context m_ssl_context;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket>m_ssl_stream;
  };
*/
  Options::Options()
  {
    // defaults
    consolelog = true;
    filelog = true;
    tcplog = true;
    datastoragelog = false;
    tcpdump = false;

#ifdef _WIN32
    if(const char* ifAppData = std::getenv("LOCALAPPDATA")) {
      logsPath = std::string(ifAppData) + "\\DKFZ\\logs\\";
    }
    else {
      logsPath = "./logs/";
    }
#else
    if(const char* ifHome = std::getenv("HOME")) {
      logsPath = std::string(ifHome) + "/.local/share/DKFZ/logs/";
    }
    else {
      logsPath = "./logs/";
    }
#endif
  }

  Options& Options::get()
  {
    static Options INSTANCE;
    return INSTANCE;
  }

  std::string Options::getIphost() const
  {
    return iphost;
  }

  std::string Options::getIpport() const
  {
    return ipport;
  }

  std::string Options::getLogsPath() const
  {
    return logsPath;
  }

  Log::Log()
    : sourceAttribute(std::string())
    , fullNameAttribute(std::string())
    , organizationAttribute(std::string())
    , additionalField(std::string())
    , m_StartTime(clock())
    , sessionTag(boost::uuids::random_generator()())
  {
    dataBackend =
      boost::make_shared< boost::log::sinks::text_ostream_backend >();

    dataStream =
      boost::make_shared< std::stringstream >();

    reinitLogger();
  }

  boost::shared_ptr< std::stringstream > Log::getDataStream() const
  {
    return dataStream;
  }

  boost::shared_ptr< boost::log::sinks::text_ostream_backend > Log::getDataBackend() const
  {
    return dataBackend;
  }

  Log& Log::get()
  {
    static Log INSTANCE;
    return INSTANCE;
  }

  Log& Log::get(const std::string& src)
  {
    auto& impl = get();
    impl.setSource(src);
    return impl;
  }

  void Log::reinitLogger()
  {
    boost::log::core::get()->remove_all_sinks();

    /// Just return if everything is disabled
    if (!(Options::get().consolelog || Options::get().filelog || Options::get().tcplog || Options::get().datastoragelog)) return;

    if (Options::get().filelog) {
      boost::shared_ptr< file_sink > sink(new file_sink(
        boost::log::keywords::file_name = Options::get().logsPath + "/%Y%m%d_%H%M%S_%5N.json",
        boost::log::keywords::rotation_size = 16384
      ));
      sink->locked_backend()->set_file_collector(boost::log::sinks::file::make_collector(
        boost::log::keywords::target = Options::get().logsPath,     /*< the target directory >*/
        boost::log::keywords::max_size = 16 * 1024 * 1024,          /*< maximum total size of the stored files, in bytes >*/
        boost::log::keywords::min_free_space = 100 * 1024 * 1024    /*< minimum free space on the drive, in bytes >*/
      ));
      sink->set_formatter(
        boost::log::expressions::format("{\"datetime\": \"%8%\", \"user\": \"%3%@%2%\", \"severity\": \"%4%\", \"source\": \"%5%\", \"fullname\": \"%6%\", \"organization\": \"%7%\", \"message\": \"%1%\", \"sessiontag\": \"%9%\" %10%}")
        % boost::phoenix::bind(&MessageForLogstash, boost::log::expressions::attr<std::string>("Message"))
        % boost::log::expressions::attr<std::string>("ComputerName")
        % boost::log::expressions::attr<std::string>("UserName")
        % boost::log::trivial::severity
        % boost::log::expressions::attr<std::string>("Source")
        % boost::log::expressions::attr<std::string>("FullName")
        % boost::log::expressions::attr<std::string>("Organization")
        % boost::log::expressions::format_date_time< boost::posix_time::ptime >("TimeStamp", TIME_STAMP_FORMAT)
        % boost::log::expressions::attr<boost::uuids::uuid>("SessionTag")
        % boost::log::expressions::attr<std::string>("AdditionalField")
      );

      /// Add the sink to the core
      boost::log::core::get()->add_sink(sink);
    }

    if (Options::get().tcplog) {
      //TODO: if ssl
      /*
      boost::asio::io_service io_service;

      boost::asio::ip::tcp::resolver resolver(io_service);
      boost::asio::ip::tcp::resolver::query query(Options::get().iphost, Options::get().ipport);
      boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);

      std::string certPath = Options::get().logsPath + "self_signed.crt";
      std::string keyPath = Options::get().logsPath + "key.pem";

      boost::asio::ssl::context context(/*io_service,*//* boost::asio::ssl::context::sslv23);
      //context.set_options(
      //    boost::asio::ssl::context::default_workarounds
      //      | boost::asio::ssl::context::no_sslv2
      //      | boost::asio::ssl::context::no_sslv3);
      context.load_verify_file(keyPath);
      //context.use_certificate_chain_file(certPath);
      //context.use_private_key_file(keyPath, boost::asio::ssl::context::pem);

      context.set_default_verify_paths();
      context.set_verify_mode(boost::asio::ssl::verify_none);

*/
      boost::shared_ptr< boost::log::sinks::text_ostream_backend > backend =
        boost::make_shared< boost::log::sinks::text_ostream_backend >();

      boost::shared_ptr< boost::asio::ip::tcp::iostream > stream =
        boost::make_shared< boost::asio::ip::tcp::iostream >();

      //auto portnum = Options::get().ipport.empty() ? 5065 : std::stoi(Options::get().ipport);
      //SyncSSLClient client(Options::get().iphost, portnum);

      std::cout << "Sending request to the server... " << std::endl;
      std::string response = client.emulateLongComputationOp(10);
      std::cout << "Response received: " << response << std::endl;

      client.close();

      m_TaskGroup.Enqueue([stream] {
        stream->connect(Options::get().iphost, Options::get().ipport);
      });

      backend->add_stream(stream);
      backend->auto_flush(true);

      boost::shared_ptr< ostream_sink > sink2(new ostream_sink(backend));

      // raw json data to TCP
      if (Options::get().tcpdump) {
        sink2->set_formatter(
          boost::log::expressions::format("%1%")
          % boost::phoenix::bind
              (&MessageToUTF8, boost::log::expressions::attr<std::string>("Message"))
        );
      } else {
        sink2->set_formatter(
          boost::log::expressions::format("{\"datetime\": \"%8%\", \"user\": \"%3%@%2%\", \"severity\": \"%4%\", \"source\": \"%5%\", \"fullname\": \"%6%\", \"organization\": \"%7%\", \"message\": \"%1%\", \"sessiontag\": \"%9%\" %10%}")
          % boost::phoenix::bind(&MessageForLogstash, boost::log::expressions::attr<std::string>("Message"))
          % boost::log::expressions::attr<std::string>("ComputerName")
          % boost::log::expressions::attr<std::string>("UserName")
          % boost::log::trivial::severity
          % boost::log::expressions::attr<std::string>("Source")
          % boost::log::expressions::attr<std::string>("FullName")
          % boost::log::expressions::attr<std::string>("Organization")
          % boost::log::expressions::format_date_time< boost::posix_time::ptime >("TimeStamp", TIME_STAMP_FORMAT)
          % boost::log::expressions::attr<boost::uuids::uuid>("SessionTag")
          % boost::log::expressions::attr<std::string>("AdditionalField")
        );
      }

      boost::log::core::get()->add_sink(sink2);
    }

    if (Options::get().datastoragelog) {
      dataBackend->add_stream(dataStream);

      boost::shared_ptr< ostream_sink > sink3(new ostream_sink(dataBackend));
      sink3->set_formatter(
        boost::log::expressions::format("%1% [%3% %4%] %5% (%6%) > %2%")
        % boost::log::expressions::format_date_time< boost::posix_time::ptime >("TimeStamp", TIME_STAMP_FORMAT)
        % boost::log::expressions::xml_decor[boost::log::expressions::stream << boost::log::expressions::smessage]
        % boost::log::expressions::attr<std::string>("FullName")
        % boost::log::expressions::attr<std::string>("Organization")
        % boost::log::trivial::severity
        % boost::log::expressions::attr<std::string>("Source")
        );

      boost::log::core::get()->add_sink(sink3);
    }

    if (Options::get().consolelog) {
      boost::log::add_console_log(
        std::cout,
        boost::log::keywords::format = (
          boost::log::expressions::stream
          << '[' << boost::log::expressions::format_date_time< boost::posix_time::ptime >("TimeStamp", TIME_STAMP_FORMAT) << "] "
          << '(' << boost::log::expressions::attr<std::string>("Source") << ')'
          << boost::log::expressions::if_(boost::log::trivial::severity != boost::log::trivial::info)
          [
            boost::log::expressions::stream << ' ' << boost::log::trivial::severity
          ] << ": "
          << boost::phoenix::bind(&MessageToOEM, boost::log::expressions::attr<std::string>("Message"))
          )
        );
    }

    boost::log::core::get()->add_global_attribute("TimeStamp", boost::log::attributes::local_clock());
    boost::log::core::get()->add_global_attribute("RecordID", boost::log::attributes::counter< unsigned int >());

    boost::log::core::get()->add_global_attribute("UserName", boost::log::attributes::constant<std::string>(getName(NameType::USER)));
    boost::log::core::get()->add_global_attribute("ComputerName", boost::log::attributes::constant<std::string>(getName(NameType::HOST)));

    boost::log::core::get()->add_global_attribute("Source", sourceAttribute);

    boost::log::core::get()->add_global_attribute("FullName", fullNameAttribute);
    boost::log::core::get()->add_global_attribute("Organization", organizationAttribute);

    boost::log::core::get()->add_global_attribute("AdditionalField", additionalField);

    boost::log::core::get()->add_global_attribute("SessionTag", sessionTag);

    boost::log::add_common_attributes();
    boost::log::core::get()->flush();
  }

  void Log::setSource(const std::string& src)
  {
    sourceAttribute.set(src);
  }

  void Log::setUserData(const std::string& fullName, const std::string& organization)
  {
    fullNameAttribute.set(fullName);
    organizationAttribute.set(organization);
  }

  void Log::setAdditionalField(const std::string& field, const std::string& value)
  {
    const std::string str = field.empty() ? std::string() : ",\"" + field + "\":" + value;
    additionalField.set(str);
  }

  void Log::resetAdditionalField()
  {
    additionalField.set("");
  }

  void Log::setStartTime(clock_t time)
  {
    //Autoplan start time
    m_StartTime = time;
  }

  void Log::computeRunningTime(clock_t time)
  {
    double runningTime = double(time - m_StartTime) / CLOCKS_PER_SEC;
    setAdditionalField("runningTime", std::to_string(runningTime));
  }

  // returns true in case of date time parse success
  std::tuple<bool, boost::posix_time::ptime> dateFromString(const std::string& dateTime)
  {
    std::string formatDate("%Y-%b-%d %H:%M:%S%F"); // 2016-Jan-27 18:04:30.610194
    boost::posix_time::ptime date;
    try {
      std::istringstream stream(dateTime);
      stream.imbue(std::locale(std::locale::classic(),
        new boost::gregorian::date_input_facet(formatDate.c_str())));
      stream >> date;
    }
    catch (const std::exception& excp) {
      std::string text = excp.what();
      std::cout << text << std::endl;
      std::make_tuple(false, date);
    }
    return std::make_tuple(true, date);
  }

  // get actual seconds from boost::posix_time
  std::time_t pt_to_time_t(const boost::posix_time::ptime& pt)
  {
    boost::posix_time::ptime timet_start(boost::gregorian::date(1970, 1, 1));
    boost::posix_time::time_duration diff = pt - timet_start;
    return diff.ticks() / boost::posix_time::time_duration::rep_type::ticks_per_second;
  }

  std::string Log::getLastDateTime(std::string str)
  {
    std::vector<std::string> lines;
    boost::split(lines, str, boost::is_any_of("\n"));
    if (lines.size() > 1) {
      std::string tail = lines.end()[-2];
      std::vector<std::string> message;
      boost::split(message, tail, boost::is_any_of(" >"));
      if (message.size() > 1) {
        std::string date = message.front();
        std::string time = message.begin()[1];
        return (date + " " + time);
      }
    }
    return nullptr;
  }

  void Log::resetData() const
  {
    dataBackend->remove_stream(dataStream);
    dataStream->str("");
    dataStream->clear();
    dataBackend->add_stream(dataStream);
  }

  std::string Log::getData() const
  {
    dataBackend->flush();
    return dataStream->str();
  }

  std::string Log::getDataFromDate(std::string dateTime) const
  {
    dataBackend->flush();
    std::vector<std::string> lines;
    std::string datastring = dataStream->str();
    boost::split(lines, datastring, boost::is_any_of("\n"));
    if (lines.size() > 1) {
      std::vector<std::string> result;

      std::vector<std::string>::reverse_iterator rit = lines.rbegin();
      for (; rit != lines.rend(); ++rit) {
        std::vector<std::string> message;
        boost::split(message, *rit, boost::is_any_of(" >"));
        if (message.size() > 1) {
          std::string date = message.front();
          std::string time = message.begin()[1];
          std::string lineTime = (date + " " + time);

          auto lastDate = dateFromString(dateTime);
          auto lineDate = dateFromString(lineTime);

          if ( std::get<0>(lastDate) == true
            && std::get<0>(lineDate) == true) {
            std::time_t lastDateTime = pt_to_time_t( std::get<1>(lastDate) );
            std::time_t lastLineTime = pt_to_time_t( std::get<1>(lineDate) );
            if (lastDateTime < lastLineTime) {
              result.push_back(*rit);
            }
            else {
              break;
            }
          }
        }
      }

      std::reverse(result.begin(), result.end());
      return boost::join(result, "\n") + "\n";
    }
    return nullptr;
  }
}
