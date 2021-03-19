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

#include "mitkBaseDICOMReaderService.h"

#include <mitkCustomMimeType.h>
#include <mitkIOMimeTypes.h>
#include <mitkDICOMFileReaderSelector.h>
#include <mitkImage.h>
#include <mitkDICOMFilesHelper.h>
#include <mitkDICOMTagsOfInterestHelper.h>
#include <mitkDICOMProperty.h>
#include "legacy/mitkDicomSeriesReader.h"
#include <mitkDICOMDCMTKTagScanner.h>
#include <mitkLocaleSwitch.h>
#include "mitkIPropertyProvider.h"
#include "mitkPropertyNameHelper.h"

#include <iostream>


#include <itksys/SystemTools.hxx>
#include <itksys/Directory.hxx>

namespace mitk {

  BaseDICOMReaderService::BaseDICOMReaderService(const std::string& description)
    : AbstractFileReader(CustomMimeType(IOMimeTypes::DICOM_MIMETYPE()), description)
{
}

  BaseDICOMReaderService::BaseDICOMReaderService(const mitk::CustomMimeType& customType, const std::string& description)
    : AbstractFileReader(customType, description)
  {
  }

std::vector<itk::SmartPointer<BaseData> > BaseDICOMReaderService::Read()
{
  std::vector<BaseData::Pointer> result;

  mitk::StringList relevantFiles = this->GetRelevantFiles();

  mitk::DICOMFileReader::Pointer reader = this->GetReader(relevantFiles);

  reader->SetAdditionalTagsOfInterest(mitk::GetCurrentDICOMTagsOfInterest());
  reader->SetTagLookupTableToPropertyFunctor(mitk::GetDICOMPropertyForDICOMValuesFunctor);
  reader->SetInputFiles(relevantFiles);
  reader->AnalyzeInputFiles();
  reader->LoadImages();

  for (unsigned int i = 0; i < reader->GetNumberOfOutputs(); ++i)
  {
    const mitk::DICOMImageBlockDescriptor& desc = reader->GetOutput(i);
    mitk::BaseData::Pointer data = desc.GetMitkImage().GetPointer();

    std::string nodeName = "Unnamed_DICOM";

    std::string studyDescription = desc.GetPropertyAsString("studyDescription");
    std::string seriesDescription = desc.GetPropertyAsString("seriesDescription");

    if (!studyDescription.empty())
    {
      nodeName = studyDescription;
    }

    if (!seriesDescription.empty())
    {
      if (!studyDescription.empty())
      {
        nodeName += "/";
      }
      nodeName += seriesDescription;
    }

    StringProperty::Pointer nameProp = StringProperty::New(nodeName);
    data->SetProperty("name", nameProp);

    result.push_back(data);
  }

  return result;
}

StringList BaseDICOMReaderService::GetRelevantFiles() const
{
  std::string fileName = this->GetLocalFileName();

  mitk::StringList relevantFiles = mitk::GetDICOMFilesInSameDirectory(fileName);

  return relevantFiles;
}

IFileReader::ConfidenceLevel BaseDICOMReaderService::GetConfidenceLevel() const
{
  IFileReader::ConfidenceLevel abstractConfidence = AbstractFileReader::GetConfidenceLevel();

  if (Unsupported == abstractConfidence)
  {
    if (itksys::SystemTools::FileIsDirectory(this->GetInputLocation().c_str()))
    {
      // In principle we support dicom directories
      return Supported;
    }
  }

  return abstractConfidence;
}

std::string GenerateNameFromDICOMProperties(const mitk::IPropertyProvider* provider)
{
  std::string nodeName = mitk::DataNode::NO_NAME_VALUE();

  auto studyProp = provider->GetConstProperty(mitk::GeneratePropertyNameForDICOMTag(0x0020, 0x000D).c_str());
  if (studyProp.IsNotNull())
  {
    nodeName = studyProp->GetValueAsString();
  }

  auto seriesProp = provider->GetConstProperty(mitk::GeneratePropertyNameForDICOMTag(0x0020, 0x000E).c_str());

  if (seriesProp.IsNotNull())
  {
    if (studyProp.IsNotNull())
    {
      nodeName += " / ";
    }
    nodeName += seriesProp->GetValueAsString();
  }

  return nodeName;
};


}
