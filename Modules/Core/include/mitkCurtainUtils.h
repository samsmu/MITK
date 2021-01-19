#pragma once

#include "mitkBaseRenderer.h"
#include "mitkDataNode.h"

#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>

namespace mitk {
namespace CurtainUtils {

template <typename LocalStorage>
bool checkAndCalculateBounds(
  LocalStorage* localStorage, 
  mitk::DataNode* node, 
  mitk::BaseRenderer* renderer, 
  double depth, 
  double textureClippingBounds[6]
)
{
  mitk::Point2D displayStart;
  displayStart[0] = 0.0;
  displayStart[1] = 0.0;
  mitk::Point2D displayEnd;
  displayEnd[0] = renderer->GetSizeX();
  displayEnd[1] = renderer->GetSizeY();

  mitk::Point2D planeStartMm;
  mitk::Point2D planeEndMm;
  renderer->DisplayToPlane(displayStart, planeStartMm);
  renderer->DisplayToPlane(displayEnd, planeEndMm);

  int horStart = 0;
  int horEnd = 100;
  int verStart = 0;
  int verEnd = 100;
  node->GetIntProperty("curtain.horizontal.start", horStart, renderer);
  node->GetIntProperty("curtain.horizontal.stop", horEnd, renderer);
  node->GetIntProperty("curtain.vertical.start", verStart, renderer);
  node->GetIntProperty("curtain.vertical.stop", verEnd, renderer);

  if (horEnd < horStart) { horEnd = horStart; }
  if (verEnd < verStart) { verEnd = verStart; }

  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();

  auto addLine = [points, lines](std::vector<double> start, std::vector<double> end) {
    vtkIdType p1;
    vtkIdType p2;
    p1 = points->InsertNextPoint(start[0], start[1], start[2]);
    p2 = points->InsertNextPoint(end[0], end[1], end[2]);
    lines->InsertNextCell(2);
    lines->InsertCellPoint(p1);
    lines->InsertCellPoint(p2);
  };

  bool retval = false;
  double lengthMm = planeEndMm[0] - planeStartMm[0];
  double heightMm = planeEndMm[1] - planeStartMm[1];
  if (horStart > 0 && horStart < 100) {
    planeStartMm[0] = planeStartMm[0] + lengthMm * horStart / 100.0;
    addLine(
    { planeStartMm[0], planeStartMm[1], depth },
    { planeStartMm[0], planeEndMm[1], depth }
    );
    if (textureClippingBounds[0] < planeStartMm[0]) { textureClippingBounds[0] = planeStartMm[0]; }
    retval = true;
  }
  if (horEnd > 0 && horEnd < 100) {
    planeEndMm[0] = planeStartMm[0] + lengthMm * horEnd / 100.0;
    addLine(
    { planeEndMm[0], planeStartMm[1], depth },
    { planeEndMm[0], planeEndMm[1], depth }
    );
    if (textureClippingBounds[1] > planeEndMm[0]) { textureClippingBounds[1] = planeEndMm[0]; }
    retval = true;
  }
  if (verStart > 0 && verStart < 100) {
    planeStartMm[1] = planeStartMm[1] + heightMm * verStart / 100.0;
    addLine(
    { planeStartMm[0], planeStartMm[1], depth },
    { planeEndMm[0], planeStartMm[1], depth }
    );
    if (textureClippingBounds[2] < planeStartMm[1]) { textureClippingBounds[2] = planeStartMm[1]; }
    retval = true;
  }
  if (verEnd > 0 && verEnd < 100) {
    planeEndMm[1] = planeStartMm[1] + heightMm * verEnd / 100.0;
    addLine(
    { planeStartMm[0], planeEndMm[1], depth },
    { planeEndMm[0], planeEndMm[1], depth }
    );
    if (textureClippingBounds[3] > planeEndMm[1]) { textureClippingBounds[3] = planeEndMm[1]; }
    retval = true;
  }

  localStorage->m_CurtainPlaneStart[0] = planeStartMm[0] / localStorage->m_mmPerPixel[0];
  localStorage->m_CurtainPlaneStart[1] = planeStartMm[1] / localStorage->m_mmPerPixel[1];
  localStorage->m_CurtainPlaneEnd[0] = planeEndMm[0] / localStorage->m_mmPerPixel[0];
  localStorage->m_CurtainPlaneEnd[1] = planeEndMm[1] / localStorage->m_mmPerPixel[1];

  localStorage->m_CurtainPolyData->SetPoints(points);
  localStorage->m_CurtainPolyData->SetLines(lines);

  return retval;
}

}
}