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


#ifndef SUBIMAGESELECTOR_H_HEADER_INCLUDED_C1E4F463
#define SUBIMAGESELECTOR_H_HEADER_INCLUDED_C1E4F463

#include <MitkCoreExports.h>
#include "mitkImageToImageFilter.h"
#include "mitkImageDataItem.h"
#include "mitkBaseData.h"

namespace mitk {

//##Documentation
//## @brief Base class of all classes providing access to parts of an image
//##
//## Base class of all classes providing access to parts of an image, e.g., to
//## a slice (mitk::ImageSilceSelector) or a volume at a specific time
//## (mitk::ImageTimeSelector). If the input is generated by a ProcessObject,
//## only the required data is requested.
//## @ingroup Process
class MITKCORE_EXPORT SubImageSelector : public ImageToImageFilter
{
public:
  /** Run-time type information (and related methods). */
  mitkClassMacro(SubImageSelector,ImageToImageFilter);

  itkFactorylessNewMacro(Self)
  itkCloneMacro(Self)

  virtual void SetPosNr(int p);

  SubImageSelector();

  virtual ~SubImageSelector();

protected:
  mitk::Image::ImageDataItemPointer GetVolumeData(int t = 0, int n = 0);
  mitk::Image::ImageDataItemPointer GetSliceData(int s = 0, int t = 0, int n = 0);

  void SetSliceData(void* data, int s = 0, int t = 0, int n = 0);

  void SetVolumeItem(mitk::Image::ImageDataItemPointer dataItem, int t = 0, int n = 0);
};

} // namespace mitk

#endif /* SUBIMAGESELECTOR_H_HEADER_INCLUDED_C1E4F463 */
