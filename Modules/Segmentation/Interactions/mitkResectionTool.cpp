#include "mitkResectionTool.h"

#include <vtkCamera.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include <mitkResectionFilter.h>
#include <mitkImageAccessByItk.h>
#include <mitkToolManager.h>
#include <mitkITKImageImport.h>

namespace mitk
{
MITK_TOOL_MACRO(MITKSEGMENTATION_EXPORT, ResectionTool, "Resection tool");

ResectionTool::ResectionTool() : ContourTool(),
  m_State(ResectionState::NO_CONTOUR)
{
  SegTool2D::SetAllow3dMapper(true);
  FeedbackContourTool::SetFeedbackContourDraw3D(true);
  Tool::setLock3D(true);
}

ResectionTool::~ResectionTool()
{
}

const char* ResectionTool::GetName() const
{
  return "Resection";
}

const char** ResectionTool::GetXPM() const
{
  return mitkDrawPaintbrushTool_xpm;
}

us::ModuleResource ResectionTool::GetIconResource() const
{
  us::Module* module = us::GetModuleContext()->GetModule();
  us::ModuleResource resource = module->GetResource("ResectionTool_48x48.png");
  return resource;
}

us::ModuleResource ResectionTool::GetCursorIconResource() const
{
  us::Module* module = us::GetModuleContext()->GetModule();
  us::ModuleResource resource = module->GetResource("ResectionTool_Cursor_32x32.png");
  return resource;
}

void ResectionTool::OnMousePressed(StateMachineAction* action, InteractionEvent* interactionEvent)
{
  bool renderIs2D = interactionEvent->GetSender()->GetMapperID() == mitk::BaseRenderer::Standard2D;
  m_FeedbackContourNode->SetProperty("contour.render2D", mitk::BoolProperty::New(renderIs2D));
  m_FeedbackContourNode->SetProperty("contour.render3D", mitk::BoolProperty::New(!renderIs2D));
  RenderingManager::GetInstance()->RequestUpdateAll();

  Superclass::OnMousePressed(action, interactionEvent);
}

void ResectionTool::OnMouseReleased(StateMachineAction*, InteractionEvent* interactionEvent)
{
  RenderingManager::GetInstance()->RequestUpdateAll();
  auto pointCount = m_FeedbackContour->GetNumberOfVertices();
  if (pointCount < 3) {
    return;
  }

  m_LastEventSender = interactionEvent->GetSender();
  m_State = ResectionState::CONTOUR_AVAILABLE;
  stateChanged.Send(m_State);
}

ResectionTool::ResectionState ResectionTool::GetState()
{
  return m_State;
}

void ResectionTool::Activated()
{
  Superclass::Activated();

  m_State = ResectionState::NO_CONTOUR;
  stateChanged.Send(m_State);
}

template<typename TPixel, unsigned int VImageDimension>
void AccessResectFilter(const itk::Image<TPixel, VImageDimension>* itkImage, vtkSmartPointer<vtkPoints> points, vtkSmartPointer<vtkMatrix4x4> viewMatrix, ResectionTool::ResectionType resectionType)
{
  using ResectionFilter = mitk::ResectionFilter<itk::Image<TPixel, VImageDimension>>;
  typename ResectionFilter::Pointer resBuilder = ResectionFilter::New();
  resBuilder->SetInputPoints(points);
  resBuilder->setViewportMatrix(viewMatrix);
  resBuilder->SetInput(itkImage);
  if (resectionType == ResectionTool::ResectionType::INSIDE) {
    resBuilder->setRegionType(ResectionFilter::ResectionRegionType::INSIDE);
  } else { // if (resectionType == ResectionTool::ResectionType::OUTSIDE
    resBuilder->setRegionType(ResectionFilter::ResectionRegionType::OUTSIDE);
  }
  resBuilder->InPlaceOn();
  resBuilder->Update();
}

vtkMatrix4x4* getWorldToViewTransform(vtkRenderer* renderer)
{
  vtkCamera* camera = renderer->GetActiveCamera();

  // figure out the same aspect ratio used by the render engine
  // (see vtkOpenGLCamera::Render())
  int  lowerLeft[2];
  int usize, vsize;
  double aspect1[2];
  double aspect2[2];
  renderer->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft + 1);
  renderer->ComputeAspect();
  renderer->GetAspect(aspect1);
  renderer->vtkViewport::ComputeAspect();
  renderer->vtkViewport::GetAspect(aspect2);
  double aspectModification = (aspect1[0] * aspect2[1]) / (aspect1[1] * aspect2[0]);
  double aspect = aspectModification * usize / vsize;
  vtkMatrix4x4* matrix = camera->GetCompositeProjectionTransformMatrix(aspect, -1, 1);

  return matrix;
}

void ResectionTool::Resect(ResectionType type)
{
  auto pointCount = m_FeedbackContour->GetNumberOfVertices();
  if (pointCount < 3) {
    return;
  }

  vtkSmartPointer<vtkPoints> points = vtkPoints::New();
  points->Resize(pointCount);
  for (int i = 0; i < pointCount; i++) {
    Point3D contourVertex = m_FeedbackContour->GetVertexAt(i)->Coordinates;
    points->InsertNextPoint(contourVertex.GetDataPointer());
  }

  DataNode* workingData = m_ToolManager->GetWorkingData().front();
  Image::Pointer segmentation = dynamic_cast<Image*>(workingData->GetData());
  vtkMatrix4x4* worldView = getWorldToViewTransform(m_LastEventSender->GetVtkRenderer());
  AccessByItk_n(segmentation, AccessResectFilter, (points, worldView, type));
  segmentation->Modified();
  m_FeedbackContour->Clear();
  RenderingManager::GetInstance()->RequestUpdateAll();

  m_State = ResectionState::NO_CONTOUR;
  stateChanged.Send(m_State);
}

}
