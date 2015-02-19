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

#include "berryPartInfo.h"

namespace berry
{

// TODO ism temporary translate
// it hart to find point, when plugin name read from file

const QString PartInfo::plugingTranslateNames[] = { 
  QObject::tr("BasicImageProcessing"),   
  QObject::tr("Clipping Plane"),
  QObject::tr("Display"),
  QObject::tr("Data Manager"),
  QObject::tr("Image Navigator"),
  QObject::tr("LiverSegmentation"),
  QObject::tr("Measurement"),
  QObject::tr("PointSet Interaction"),
  QObject::tr("RegionGrowing Segmentation"),
  QObject::tr("Segmentation"),
  QObject::tr("Segmentation Utilities"),
  QObject::tr("Statistics"),
  QObject::tr("Vascular Structure Segmentation"),
  QObject::tr("Volume Visualization")
};


PartInfo::PartInfo() :
  image(0)
{
  plugingSrcNames << "BasicImageProcessing" << "Clipping Plane" << "Display" << "Data Manager" << "Image Navigator" << "LiverSegmentation" <<
    "Measurement" << "PointSet Interaction" << "RegionGrowing Segmentation" << "Segmentation" << "Segmentation Utilities" <<
    "Statistics" << "Vascular Structure Segmentation" << "Volume Visualization";
}

PartInfo::PartInfo(IPresentablePart::Pointer part)
{
  this->Set(part);
}

void PartInfo::Set(IPresentablePart::Pointer part)
{
  name = part->GetName().c_str();

  for (int i=0; i<plugingSrcNames.size(); i++) {
    if (plugingSrcNames[i].compare(QString(part->GetName().c_str())) == 0) {
      name = plugingTranslateNames[i];
    }
  }
  
  title = part->GetTitle().c_str();
  contentDescription = part->GetTitleStatus().c_str();
  image = static_cast<QIcon*> (part->GetTitleImage());
  toolTip = part->GetTitleToolTip().c_str();
  dirty = part->IsDirty();
}

}
