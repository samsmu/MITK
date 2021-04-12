/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include "QmitkStdMultiWidgetEditor.h"

#include <berryUIException.h>
#include <berryIWorkbenchPage.h>
#include <berryIPreferencesService.h>
#include <berryIPartListener.h>
#include <berryIPreferences.h>

#include <QWidget>
#include <QWheelEvent>

#include <mitkColorProperty.h>
#include <mitkCrosshairManager.h>
#include <mitkNodePredicateNot.h>
#include <mitkNodePredicateProperty.h>

#include <mitkDataStorageEditorInput.h>
#include <mitkIDataStorageService.h>

#include <mitkStandaloneDataStorage.h>

#include <QmitkMouseModeSwitcher.h>
#include <QmitkStdMultiWidget.h>

#include <QHBoxLayout>

namespace
{
  QmitkStdMultiWidget* createStdMultiWidget(QHash<QString, QmitkRenderWindow*>& renderWindows, berry::IPreferences::Pointer preferences, mitk::DataStorage::Pointer ds)
  {
    bool useFXAA = preferences->GetBool("Use FXAA", true);
    bool planeVisibility3D = preferences->GetBool("Plane Visibility 3D", true);
    mitk::BaseRenderer::RenderingMode::Type renderingMode = static_cast<mitk::BaseRenderer::RenderingMode::Type>(preferences->GetInt("Rendering Mode", 2));

    auto stdMultiWidget = new QmitkStdMultiWidget(nullptr, 0, mitk::RenderingManager::New(), renderingMode, useFXAA, "additionalMultiWidget", false, QmitkStdMultiWidget::WidgetType::ADDITIONAL_WIDGET);

    stdMultiWidget->SetDataStorage(ds);
    stdMultiWidget->DisableStandardLevelWindow();
    stdMultiWidget->ActivateMenuWidget(false);

    mitk::Color mitkColor;
    mitkColor.SetRed(0);
    mitkColor.SetGreen(0);
    mitkColor.SetBlue(0);
    stdMultiWidget->SetGradientBackgroundColorForRenderWindow(mitkColor, mitkColor, 3);

    stdMultiWidget->GetRenderWindow4()->GetRenderer()->SetMapperID(mitk::BaseRenderer::Standard3D);

    bool displayMetainfo = preferences->GetBool("Display metainfo", true);
    stdMultiWidget->setDisplayMetaInfo(displayMetainfo);

    return stdMultiWidget;
  }
}

class QmitkStdMultiWidgetEditorPrivate
{
public:

  QmitkStdMultiWidgetEditorPrivate();
  ~QmitkStdMultiWidgetEditorPrivate();

  std::vector<QmitkStdMultiWidget*> m_StdMultiWidgets;
  QmitkMouseModeSwitcher* m_MouseModeToolbar;
  /**
  * @brief Members for the MultiWidget decorations.
  */
  QString m_WidgetBackgroundColor1[4];
  QString m_WidgetBackgroundColor2[4];
  QString m_WidgetDecorationColor[4];
  bool m_MenuWidgetsEnabled;
  QScopedPointer<berry::IPartListener> m_PartListener;

  QHash<QString, QmitkRenderWindow*> m_RenderWindows;

};

struct QmitkStdMultiWidgetPartListener : public berry::IPartListener
{
  QmitkStdMultiWidgetPartListener(QmitkStdMultiWidgetEditorPrivate* dd)
    : d(dd)
  {}

  Events::Types GetPartEventTypes() const override
  {
    return Events::CLOSED | Events::HIDDEN | Events::VISIBLE;
  }

  void PartClosed(const berry::IWorkbenchPartReference::Pointer& partRef) override
  {
    if (partRef->GetId() == QmitkStdMultiWidgetEditor::EDITOR_ID)
    {
      QmitkStdMultiWidgetEditor::Pointer stdMultiWidgetEditor = partRef->GetPart(false).Cast<QmitkStdMultiWidgetEditor>();

      //TO DO: get from focused widget
      if (stdMultiWidgetEditor->GetStdMultiWidgetCount() > 0 && d->m_StdMultiWidgets.size() > 0) {
        if (d->m_StdMultiWidgets.at(0) == stdMultiWidgetEditor->GetStdMultiWidget()) {
          d->m_StdMultiWidgets.at(0)->RemovePlanesFromDataStorage();
          stdMultiWidgetEditor->RequestActivateMenuWidget(false);
        }
      }

      stdMultiWidgetEditor->resetAdvancedMode();
    }
  }

  void PartHidden(const berry::IWorkbenchPartReference::Pointer& partRef) override
  {
    if (partRef->GetId() == QmitkStdMultiWidgetEditor::EDITOR_ID)
    {
      QmitkStdMultiWidgetEditor::Pointer stdMultiWidgetEditor = partRef->GetPart(false).Cast<QmitkStdMultiWidgetEditor>();

      //TO DO: get from focused widget
      if (stdMultiWidgetEditor->GetStdMultiWidgetCount() > 0 && d->m_StdMultiWidgets.size() > 0) {
        if (d->m_StdMultiWidgets.at(0) == stdMultiWidgetEditor->GetStdMultiWidget()) {
          d->m_StdMultiWidgets.at(0)->RemovePlanesFromDataStorage();
          stdMultiWidgetEditor->RequestActivateMenuWidget(false);
        }
      }

      stdMultiWidgetEditor->resetAdvancedMode();
    }
  }

  void PartVisible(const berry::IWorkbenchPartReference::Pointer& partRef) override
  {
    if (partRef->GetId() == QmitkStdMultiWidgetEditor::EDITOR_ID)
    {
      QmitkStdMultiWidgetEditor::Pointer stdMultiWidgetEditor = partRef->GetPart(false).Cast<QmitkStdMultiWidgetEditor>();

      //TO DO: get from focused widget
      if (stdMultiWidgetEditor->GetStdMultiWidgetCount() > 0 && d->m_StdMultiWidgets.size() > 0) {
        if (d->m_StdMultiWidgets.at(0) == stdMultiWidgetEditor->GetStdMultiWidget()) {
          d->m_StdMultiWidgets.at(0)->AddPlanesToDataStorage();
          stdMultiWidgetEditor->RequestActivateMenuWidget(true);
        }
      }
    }
  }

private:

  QmitkStdMultiWidgetEditorPrivate* const d;

};

QmitkStdMultiWidgetEditorPrivate::QmitkStdMultiWidgetEditorPrivate()
  : m_MouseModeToolbar(0)
  , m_MenuWidgetsEnabled(false)
  , m_PartListener(new QmitkStdMultiWidgetPartListener(this))
{}

QmitkStdMultiWidgetEditorPrivate::~QmitkStdMultiWidgetEditorPrivate()
{
}

const QString QmitkStdMultiWidgetEditor::EDITOR_ID = "org.mitk.editors.stdmultiwidget";

QmitkStdMultiWidgetEditor::QmitkStdMultiWidgetEditor()
  : d(new QmitkStdMultiWidgetEditorPrivate)
{
}

QmitkStdMultiWidgetEditor::~QmitkStdMultiWidgetEditor()
{
  this->GetSite()->GetPage()->RemovePartListener(d->m_PartListener.data());
}

void QmitkStdMultiWidgetEditor::SetWindowPresetBeforeLoading()
{
  //TO DO: get from focused widget
  if (!d->m_StdMultiWidgets.empty()) {
    d->m_StdMultiWidgets.at(0)->changeLayoutToWidget1();
  }
}

void QmitkStdMultiWidgetEditor::SetWindowPresetAfterLoading()
{
  //TO DO: get from focused widget
  if (!d->m_StdMultiWidgets.empty()) {
    d->m_StdMultiWidgets.at(0)->changeLayoutToDefault();
  }
}


QmitkStdMultiWidget* QmitkStdMultiWidgetEditor::GetStdMultiWidget(int index)
{
  QmitkStdMultiWidget* multiWidget = nullptr;
  if (0 <= index && index < d->m_StdMultiWidgets.size()) {
    multiWidget = d->m_StdMultiWidgets.at(index);
  }
  return multiWidget;
}

QmitkRenderWindow *QmitkStdMultiWidgetEditor::GetActiveQmitkRenderWindow() const
{
  //TO DO: get from focused widget
  QmitkRenderWindow* window = nullptr;
  if (!d->m_StdMultiWidgets.empty()) {
    d->m_StdMultiWidgets.at(0)->GetRenderWindow1();
  }
  return window;
}

QHash<QString, QmitkRenderWindow *> QmitkStdMultiWidgetEditor::GetQmitkRenderWindows() const
{
  return d->m_RenderWindows;
}

QmitkRenderWindow *QmitkStdMultiWidgetEditor::GetQmitkRenderWindow(const QString &id) const
{
  //TO DO: get from focused widget
  if (d->m_RenderWindows.contains(id))
    return d->m_RenderWindows[id];

  return 0;
}

mitk::Point3D QmitkStdMultiWidgetEditor::GetSelectedPosition(const QString & /*id*/) const
{
  //TO DO: get from focused widget
  mitk::Point3D position = mitk::Point3D();
  if (!d->m_StdMultiWidgets.empty()) {
    d->m_StdMultiWidgets.at(0)->GetCrossPosition();
  }
  return position;
}

void QmitkStdMultiWidgetEditor::SetSelectedPosition(const mitk::Point3D &pos, const QString &/*id*/)
{
  //TO DO: get from focused widget
  if (!d->m_StdMultiWidgets.empty()) {
    d->m_StdMultiWidgets.at(0)->MoveCrossToPosition(pos);
  }
}

void QmitkStdMultiWidgetEditor::EnableDecorations(bool enable, const QStringList &decorations)
{
  //TO DO: get from focused widget
  if (d->m_StdMultiWidgets.empty()) {
    return;
  }

  auto multiWidget = d->m_StdMultiWidgets.at(0);
  if (decorations.isEmpty() || decorations.contains(DECORATION_BORDER))
  {
    enable ? multiWidget->EnableColoredRectangles()
           : multiWidget->DisableColoredRectangles();
  }
  if (decorations.isEmpty() || decorations.contains(DECORATION_MENU))
  {
    multiWidget->ActivateMenuWidget(enable);
  }
  if (decorations.isEmpty() || decorations.contains(DECORATION_BACKGROUND))
  {
    enable ? multiWidget->EnableGradientBackground()
           : multiWidget->DisableGradientBackground();
  }
  if (decorations.isEmpty() || decorations.contains(DECORATION_CORNER_ANNOTATION))
  {
    enable ? multiWidget->SetCornerAnnotationVisibility(true)
           : multiWidget->SetCornerAnnotationVisibility(false);
  }
}

bool QmitkStdMultiWidgetEditor::IsDecorationEnabled(const QString &decoration) const
{
  //TO DO: get from focused widget
  if (d->m_StdMultiWidgets.empty()) {
    return false;
  }

  auto multiWidget = d->m_StdMultiWidgets.at(0);
  if (decoration == DECORATION_BORDER)
  {
    return multiWidget->IsColoredRectanglesEnabled();
  }
  else if (decoration == DECORATION_MENU)
  {
    return multiWidget->IsMenuWidgetEnabled();
  }
  else if (decoration == DECORATION_BACKGROUND)
  {
    return multiWidget->GetGradientBackgroundFlag();
  }
  else if (decoration == DECORATION_CORNER_ANNOTATION)
  {
    return multiWidget ->IsCornerAnnotationVisible();
  }

  return false;
}

QStringList QmitkStdMultiWidgetEditor::GetDecorations() const
{
  QStringList decorations;
  decorations << DECORATION_BORDER << DECORATION_MENU << DECORATION_BACKGROUND << DECORATION_CORNER_ANNOTATION;
  return decorations;
}

void QmitkStdMultiWidgetEditor::EnableSlicingPlanes(bool enable)
{
  if (!d->m_StdMultiWidgets.empty()) {
    d->m_StdMultiWidgets.at(0)->SetWidgetPlanesVisibility(enable);
  }
}

bool QmitkStdMultiWidgetEditor::IsSlicingPlanesEnabled() const
{
  bool isEnabled(false);
  if (!d->m_StdMultiWidgets.empty()) {
    isEnabled = d->m_StdMultiWidgets.at(0)->crosshairManager->getCrosshairMode() != CrosshairMode::NONE;
  }
  return isEnabled;
}

void QmitkStdMultiWidgetEditor::CreateQtPartControl(QWidget* parent)
{
  if (d->m_StdMultiWidgets.empty())
  {
    QHBoxLayout* layout = new QHBoxLayout(parent);
    layout->setContentsMargins(0,0,0,0);

    if (d->m_MouseModeToolbar == NULL)
    {
      d->m_MouseModeToolbar = new QmitkMouseModeSwitcher(); // delete by Qt via parent
      layout->addWidget(d->m_MouseModeToolbar);
    }

    berry::IPreferences::Pointer prefs = this->GetPreferences();

    mitk::BaseRenderer::RenderingMode::Type renderingMode = static_cast<mitk::BaseRenderer::RenderingMode::Type>(prefs->GetInt( "Rendering Mode" , 2 ));
    bool useFXAA = prefs->GetBool("Use FXAA", true);

    QString planeProperty("Plane Visibility 3D");
    bool planeVisibility3D = prefs->GetBool(planeProperty, true);

    auto multiWidget = new QmitkStdMultiWidget(parent, 0, 0, renderingMode, useFXAA, "stdmulti", planeVisibility3D);
    d->m_RenderWindows.insert("axial", multiWidget->GetRenderWindow1());
    d->m_RenderWindows.insert("sagittal", multiWidget->GetRenderWindow2());
    d->m_RenderWindows.insert("coronal", multiWidget->GetRenderWindow3());
    d->m_RenderWindows.insert("3d", multiWidget->GetRenderWindow4());

    d->m_MouseModeToolbar->setMouseModeSwitcher(&mitk::MouseModeSwitcher::GetInstance());

    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(2);
    multiWidget->setSizePolicy(sizePolicy);

    m_MultiWidgetSplit = new QSplitter(Qt::Horizontal);
    layout->addWidget(m_MultiWidgetSplit);

    m_MultiWidgetSplit->addWidget(multiWidget);

    mitk::DataStorage::Pointer ds = this->GetDataStorage();

    // Tell the multiWidget which (part of) the tree to render
    multiWidget->SetDataStorage(ds);

    // Initialize views as axial, sagittal, coronar to all data objects in DataStorage
    // (from top-left to bottom)
    mitk::TimeGeometry::Pointer geo = ds->ComputeBoundingGeometry3D(ds->GetAll());
    mitk::RenderingManager::GetInstance()->InitializeViews(geo);

    // Initialize bottom-right view as 3D view
    multiWidget->GetRenderWindow4()->GetRenderer()->SetMapperID(
      mitk::BaseRenderer::Standard3D );

    // Enable standard handler for levelwindow-slider
    multiWidget->EnableStandardLevelWindow();

    // Add the displayed views to the tree to see their positions
    // in 2D and 3D
    multiWidget->AddDisplayPlaneSubTree();

    // Store the initial visibility status of the menu widget.
    d->m_MenuWidgetsEnabled = multiWidget->IsMenuWidgetEnabled();

    d->m_StdMultiWidgets.push_back(multiWidget);

    d->m_MouseModeToolbar->setParent(multiWidget);

    this->GetSite()->GetPage()->AddPartListener(d->m_PartListener.data());

    berry::IBerryPreferences* berryprefs = dynamic_cast<berry::IBerryPreferences*>(prefs.GetPointer());
    InitializePreferences(berryprefs);
    this->OnPreferencesChanged(berryprefs);

    this->RequestUpdate();

    multiWidget->GetRenderWindow4()->updateAllWindows();

    connect(multiWidget, &QmitkStdMultiWidget::saveCrosshairPreferences, this, [berryprefs, planeProperty] (bool visibility3D, CrosshairMode /*mode*/) {
      berryprefs->PutBool(planeProperty, visibility3D);
    });
  }
}

void QmitkStdMultiWidgetEditor::OnPreferencesChanged(const berry::IBerryPreferences* prefs)
{
  //TO DO: get from focused widget
  if (d->m_StdMultiWidgets.empty()) {
    return;
  }

  auto multiWidget = d->m_StdMultiWidgets.at(0);
  //Update internal members
  this->FillMembersWithCurrentDecorations();
  this->GetPreferenceDecorations(prefs);
  //Now the members can be used to modify the stdmultiwidget
  mitk::Color upper = HexColorToMitkColor(d->m_WidgetBackgroundColor1[0]);
  mitk::Color lower = HexColorToMitkColor(d->m_WidgetBackgroundColor2[0]);
  multiWidget->SetGradientBackgroundColorForRenderWindow(upper, lower, 0);
  upper = HexColorToMitkColor(d->m_WidgetBackgroundColor1[1]);
  lower = HexColorToMitkColor(d->m_WidgetBackgroundColor2[1]);
  multiWidget->SetGradientBackgroundColorForRenderWindow(upper, lower, 1);
  upper = HexColorToMitkColor(d->m_WidgetBackgroundColor1[2]);
  lower = HexColorToMitkColor(d->m_WidgetBackgroundColor2[2]);
  multiWidget->SetGradientBackgroundColorForRenderWindow(upper, lower, 2);
  upper = HexColorToMitkColor(d->m_WidgetBackgroundColor1[3]);
  lower = HexColorToMitkColor(d->m_WidgetBackgroundColor2[3]);
  multiWidget->SetGradientBackgroundColorForRenderWindow(upper, lower, 3);
  multiWidget->EnableGradientBackground();

  // preferences for renderWindows
  mitk::Color colorDecorationWidget1 = HexColorToMitkColor(d->m_WidgetDecorationColor[0]);
  mitk::Color colorDecorationWidget2 = HexColorToMitkColor(d->m_WidgetDecorationColor[1]);
  mitk::Color colorDecorationWidget3 = HexColorToMitkColor(d->m_WidgetDecorationColor[2]);
  mitk::Color colorDecorationWidget4 = HexColorToMitkColor(d->m_WidgetDecorationColor[3]);
  multiWidget->SetDecorationColor(0, colorDecorationWidget1);
  multiWidget->SetDecorationColor(1, colorDecorationWidget2);
  multiWidget->SetDecorationColor(2, colorDecorationWidget3);
  multiWidget->SetDecorationColor(3, colorDecorationWidget4);

  std::vector<mitk::Color> colors;
  colors.push_back(colorDecorationWidget1);
  colors.push_back(colorDecorationWidget2);
  colors.push_back(colorDecorationWidget3);
  colors.push_back(colorDecorationWidget4);

  multiWidget->crosshairManager->setWindowsColors(colors);

  // Crosshair gap
  multiWidget->crosshairManager->setCrosshairGap(prefs->GetInt("crosshair gap size", 32));

  //refresh colors of rectangles
  multiWidget->DisableColoredRectangles(); //EnableColoredRectangles();

  // Set preferences respecting zooming and panning
  bool constrainedZooming = prefs->GetBool("Use constrained zooming and panning", true);

  mitk::RenderingManager::GetInstance()->SetConstrainedPanningZooming(constrainedZooming);

  mitk::RenderingManager::GetInstance()->InitializeViewsByBoundingObjects(this->GetDataStorage());

  mitk::RenderingManager::GetInstance()->RequestUpdateAll();

  // level window setting
  bool showLevelWindowWidget = prefs->GetBool("Show level/window widget", true);
  if (showLevelWindowWidget)
  {
    multiWidget->EnableStandardLevelWindow();
  }
  else
  {
    multiWidget->DisableStandardLevelWindow();
  }

  bool displayMetainfo = prefs->GetBool("Display metainfo", true);
  multiWidget->setDisplayMetaInfo( displayMetainfo );

  bool selectionMode = prefs->GetBool("Selection on 3D View", false);
  multiWidget->setSelectionMode(selectionMode);

  // mouse modes toolbar
  // deleted mouse mode "PACS"
  //bool newMode = prefs->GetBool("PACS like mouse interaction", false);
  d->m_MouseModeToolbar->setVisible(false);
  /*d->m_StdMultiWidget->GetMouseModeSwitcher()*/mitk::MouseModeSwitcher::GetInstance().SetInteractionScheme( /*newMode ? mitk::MouseModeSwitcher::PACS :*/ mitk::MouseModeSwitcher::MITK);

  mitk::DisplayInteractor::SetClockRotationSpeed(prefs->GetInt("Rotation Step", 90));
  multiWidget->crosshairManager->updateAllWindows();
}

mitk::Color QmitkStdMultiWidgetEditor::HexColorToMitkColor(const QString& widgetColorInHex)
{
  QColor qColor(widgetColorInHex);
  mitk::Color returnColor;
  float colorMax = 255.0f;
  if (widgetColorInHex.isEmpty()) // default value
  {
    returnColor[0] = 1.0;
    returnColor[1] = 1.0;
    returnColor[2] = 1.0;
    MITK_ERROR << "Using default color for unknown widget " << qPrintable(widgetColorInHex);
  }
  else
  {
    returnColor[0] = qColor.red() / colorMax;
    returnColor[1] = qColor.green() / colorMax;
    returnColor[2] = qColor.blue() / colorMax;
  }
  return returnColor;
}

QString QmitkStdMultiWidgetEditor::MitkColorToHex(const mitk::Color& color)
{
  QColor returnColor;
  float colorMax = 255.0f;
  returnColor.setRed(static_cast<int>(color[0]* colorMax + 0.5));
  returnColor.setGreen(static_cast<int>(color[1]* colorMax + 0.5));
  returnColor.setBlue(static_cast<int>(color[2]* colorMax + 0.5));
  return returnColor.name();
}

void QmitkStdMultiWidgetEditor::FillMembersWithCurrentDecorations()
{
  //TO DO: get from focused widget
  if (d->m_StdMultiWidgets.empty()) {
    return;
  }

  auto multiWidget = d->m_StdMultiWidgets.at(0);

  //fill members with current values (or default values) from the std multi widget
  for(unsigned int i = 0; i < 4; ++i)
  {
    d->m_WidgetDecorationColor[i] = MitkColorToHex(multiWidget->GetDecorationColor(i));
    d->m_WidgetBackgroundColor1[i] = MitkColorToHex(multiWidget->GetGradientColors(i).first);
    d->m_WidgetBackgroundColor2[i] = MitkColorToHex(multiWidget->GetGradientColors(i).second);
  }
}

void QmitkStdMultiWidgetEditor::GetPreferenceDecorations(const berry::IBerryPreferences * preferences)
{
  //overwrite members with values from the preferences, if they the prefrence is defined
  d->m_WidgetBackgroundColor1[0] = preferences->Get("widget1 first background color", d->m_WidgetBackgroundColor1[0]);
  d->m_WidgetBackgroundColor2[0] = preferences->Get("widget1 second background color", d->m_WidgetBackgroundColor2[0]);
  d->m_WidgetBackgroundColor1[1] = preferences->Get("widget2 first background color", d->m_WidgetBackgroundColor1[1]);
  d->m_WidgetBackgroundColor2[1] = preferences->Get("widget2 second background color", d->m_WidgetBackgroundColor2[1]);
  d->m_WidgetBackgroundColor1[2] = preferences->Get("widget3 first background color", d->m_WidgetBackgroundColor1[2]);
  d->m_WidgetBackgroundColor2[2] = preferences->Get("widget3 second background color", d->m_WidgetBackgroundColor2[2]);
  d->m_WidgetBackgroundColor1[3] = preferences->Get("widget4 first background color", d->m_WidgetBackgroundColor1[3]);
  d->m_WidgetBackgroundColor2[3] = preferences->Get("widget4 second background color", d->m_WidgetBackgroundColor2[3]);

  d->m_WidgetDecorationColor[0] = preferences->Get("widget1 decoration color", d->m_WidgetDecorationColor[0]);
  d->m_WidgetDecorationColor[1] = preferences->Get("widget2 decoration color", d->m_WidgetDecorationColor[1]);
  d->m_WidgetDecorationColor[2] = preferences->Get("widget3 decoration color", d->m_WidgetDecorationColor[2]);
  d->m_WidgetDecorationColor[3] = preferences->Get("widget4 decoration color", d->m_WidgetDecorationColor[3]);
}

void QmitkStdMultiWidgetEditor::InitializePreferences(berry::IBerryPreferences * preferences)
{
  this->FillMembersWithCurrentDecorations(); //fill members
  this->GetPreferenceDecorations(preferences); //overwrite if preferences are defined

  //create new preferences

  preferences->Put("widget1 decoration color", d->m_WidgetDecorationColor[0]);
  preferences->Put("widget2 decoration color", d->m_WidgetDecorationColor[1]);
  preferences->Put("widget3 decoration color", d->m_WidgetDecorationColor[2]);
  preferences->Put("widget4 decoration color", d->m_WidgetDecorationColor[3]);

  preferences->Put("widget1 first background color", d->m_WidgetBackgroundColor1[0]);
  preferences->Put("widget2 first background color", d->m_WidgetBackgroundColor1[1]);
  preferences->Put("widget3 first background color", d->m_WidgetBackgroundColor1[2]);
  preferences->Put("widget4 first background color", d->m_WidgetBackgroundColor1[3]);
  preferences->Put("widget1 second background color", d->m_WidgetBackgroundColor2[0]);
  preferences->Put("widget2 second background color", d->m_WidgetBackgroundColor2[1]);
  preferences->Put("widget3 second background color", d->m_WidgetBackgroundColor2[2]);
  preferences->Put("widget4 second background color", d->m_WidgetBackgroundColor2[3]);
}

void QmitkStdMultiWidgetEditor::SetFocus()
{
  if (!d->m_StdMultiWidgets.empty())
    d->m_StdMultiWidgets.at(0)->setFocus();
}

void QmitkStdMultiWidgetEditor::RequestActivateMenuWidget(bool on)
{
  //TO DO: get from focused widget
  if (!d->m_StdMultiWidgets.empty())
  {
    auto multiWidget = d->m_StdMultiWidgets.at(0);
    if (on)
    {
      multiWidget->ActivateMenuWidget(d->m_MenuWidgetsEnabled);
    }
    else
    {
      d->m_MenuWidgetsEnabled = multiWidget->IsMenuWidgetEnabled();
      multiWidget->ActivateMenuWidget(false);
    }
  }
}

int QmitkStdMultiWidgetEditor::GetStdMultiWidgetCount()
{
  return d->m_StdMultiWidgets.size();
}

void QmitkStdMultiWidgetEditor::nodeRemoved(const mitk::DataNode* node, mitk::DataStorage* globalStorage)
{
  if (!node || !globalStorage) {
    return;
  }

  for (auto multiWidget : d->m_StdMultiWidgets) {
    if (multiWidget) {
      multiWidget->nodeRemoved(node, globalStorage);
    }
  }
}

void QmitkStdMultiWidgetEditor::setAdvancedMode()
{
  resetAdvancedMode();

  mitk::DataStorage::Pointer ds = mitk::StandaloneDataStorage::New();
  auto additionalWidget = createStdMultiWidget(d->m_RenderWindows, this->GetPreferences(), ds);

  QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
  sizePolicy.setHorizontalStretch(1);
  additionalWidget->setSizePolicy(sizePolicy);

  m_MultiWidgetSplit->addWidget(additionalWidget);
  d->m_StdMultiWidgets.push_back(additionalWidget);

  additionalWidget->changeLayoutToSagittalUpAndAxialDown();
  additionalWidget->DisableColoredRectangles();

  connect(GetStdMultiWidget(1), &QmitkStdMultiWidget::WheelMoved, this, [this](QWheelEvent*) {
    auto window = GetStdMultiWidget(1)->GetRenderWindow1();
    if (window) {
      int pos = window->GetSliceNavigationController()->GetSlice()->GetPos();
      emit settingMarkerPosition(pos);
    }
  });

  mitk::RenderingManager::GetInstance()->RequestUpdateAll();
}

void QmitkStdMultiWidgetEditor::resetAdvancedMode()
{
  if (GetStdMultiWidgetCount() < 2) {
    return;
  }

  auto multiwidget = GetStdMultiWidget(1);
  if (multiwidget) {
    multiwidget->setParent(nullptr);
    auto it = std::find(d->m_StdMultiWidgets.begin(), d->m_StdMultiWidgets.end(), multiwidget);
    if (it != d->m_StdMultiWidgets.end()) {
      d->m_StdMultiWidgets.erase(it);
      delete multiwidget;
    }
  }

  emit resettingAdvancedMode();
}
