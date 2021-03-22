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
//#include "legacy/mitkDicomSeriesReader.h"
#include <mitkDICOMDCMTKTagScanner.h>
#include <mitkLocaleSwitch.h>
#include "mitkIPropertyProvider.h"
#include "mitkPropertyNameHelper.h"

#include <iostream>

#include <itkGDCMImageIO.h>
#include <itksys/SystemTools.hxx>
#include <gdcmDirectory.h>
#include <itksys/SystemTools.hxx>
#include <itksys/Directory.hxx>

namespace mitk {

  BaseDICOMReaderService::BaseDICOMReaderService(const std::string& description)
    : AbstractFileReader(CustomMimeType(IOMimeTypes::DICOM_MIMETYPE()), description)
{
}

std::vector<std::string> BaseDICOMReaderService::GetDICOMFilesInSameDirectory2(const std::string& filePath)
{
  std::vector<std::string> result;

  if (!filePath.empty())
  {
    std::string dir = itksys::SystemTools::GetFilenamePath(filePath);

    gdcm::Directory directoryLister;
    directoryLister.Load(dir.c_str(), false); // non-recursive
    result = FilterForDICOMFiles2(directoryLister.GetFilenames());
  }

  return result;
}

std::vector<std::string> BaseDICOMReaderService::FilterForDICOMFiles2(const std::vector<std::string>& fileList)
{
  std::vector<std::string> result;

  itk::GDCMImageIO::Pointer io = itk::GDCMImageIO::New();
  for (auto aFile : fileList)
  {
    if (io->CanReadFile(aFile.c_str()))
    {
      result.push_back(aFile);
    }
  }

  return result;
};


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

StringList BaseDICOMReaderService::GetRelevantFiles()
{
  std::string fileName = this->GetLocalFileName();

  mitk::StringList relevantFiles = GetDICOMFilesInSameDirectory2(fileName);

  return relevantFiles;
}


}
