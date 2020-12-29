#include <dcmdata/dcfilefo.h>
#include <dcmsr/dsrxmld.h>
#include <dcmsr/dsrdoc.h>

#include <mitkLocaleSwitch.h>

#include "mitkIOMimeTypes.h"
#include "mitkStructuredReportReaderService.h"

mitk::StructuredReportReaderService::StructuredReportReaderService()
  : AbstractFileReader()
{
  std::string category = "MITK Structured Report File";
  mitk::CustomMimeType customMimeType;
  customMimeType.SetCategory(category);
  customMimeType.AddExtension("xml");

  SetDescription(category);
  SetMimeType(customMimeType);

  RegisterService();
}

mitk::StructuredReportReaderService::~StructuredReportReaderService()
{
}

std::vector<itk::SmartPointer<mitk::BaseData>> mitk::StructuredReportReaderService::Read()
{
  LocaleSwitch localeSwitch("C");

  std::vector<itk::SmartPointer<mitk::BaseData>> result;
  /*
  // Switch the current locale to "C"
  LocaleSwitch localeSwitch("C");

  InputStream stream(this);

  TiXmlDocument doc;
  stream >> doc;
  if (!doc.Error())
  {
    TiXmlHandle docHandle( &doc );
    //unsigned int pointSetCounter(0);
    for( TiXmlElement* currentPointSetElement = docHandle.FirstChildElement("point_set_file").FirstChildElement("point_set").ToElement();
         currentPointSetElement != NULL; currentPointSetElement = currentPointSetElement->NextSiblingElement())
    {
      mitk::PointSet::Pointer newPointSet = mitk::PointSet::New();

      // time geometry assembled for addition after all points
      // else the SetPoint method would already transform the points that we provide it
      mitk::ProportionalTimeGeometry::Pointer timeGeometry = mitk::ProportionalTimeGeometry::New();

      if(currentPointSetElement->FirstChildElement("time_series") != NULL)
      {
        for( TiXmlElement* currentTimeSeries = currentPointSetElement->FirstChildElement("time_series")->ToElement();
             currentTimeSeries != NULL; currentTimeSeries = currentTimeSeries->NextSiblingElement())
        {
          unsigned int currentTimeStep(0);
          TiXmlElement* currentTimeSeriesID = currentTimeSeries->FirstChildElement("time_series_id");

          currentTimeStep = atoi(currentTimeSeriesID->GetText());

          timeGeometry->Expand( currentTimeStep + 1 ); // expand (default to identity) in any case
          TiXmlElement* geometryElem = currentTimeSeries->FirstChildElement("Geometry3D");
          if ( geometryElem )
          {
              Geometry3D::Pointer geometry = Geometry3DToXML::FromXML(geometryElem);
              if (geometry.IsNotNull())
              {
                timeGeometry->SetTimeStepGeometry(geometry,currentTimeStep);
              }
              else
              {
                MITK_ERROR << "Could not deserialize Geometry3D element.";
              }
          }
          else
          {
              MITK_WARN << "Fallback to legacy behavior: defining PointSet geometry as identity";
          }

          newPointSet = this->ReadPoints(newPointSet, currentTimeSeries, currentTimeStep);
        }
      }
      else
      {
        newPointSet = this->ReadPoints(newPointSet, currentPointSetElement, 0);
      }

      newPointSet->SetTimeGeometry(timeGeometry);

      result.push_back( newPointSet.GetPointer() );
    }
  }
  else
  {
    mitkThrow() << "Parsing error at line " << doc.ErrorRow() << ", col " << doc.ErrorCol() << ": " << doc.ErrorDesc();
  }
  */

  size_t opt_readFlags = 0;
  mitk::StructuredReport::Pointer newReport = mitk::StructuredReport::New();
  OFCondition state = newReport->reportData->readXML(this->GetLocalFileName(), opt_readFlags);
  if (state.good()) {
    result.push_back(newReport.GetPointer());
    //DcmFileFormat fileformat;
    //DcmDataset *dataset = fileformat.getDataset();
    //state = newReport->reportData.write(*dataset);
    //if (state.bad()) {
      //MITK_ERROR << "Could not deserialize structured report.";
    //}
  }

  return result;
}

mitk::StructuredReportReaderService::StructuredReportReaderService(const mitk::StructuredReportReaderService& other)
  : mitk::AbstractFileReader(other)
{
}

mitk::StructuredReportReaderService* mitk::StructuredReportReaderService::Clone() const
{
  return new mitk::StructuredReportReaderService(*this);
}
