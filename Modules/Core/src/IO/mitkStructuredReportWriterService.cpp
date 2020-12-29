#include <dcmsr/dsrdoc.h>

#include <mitkLocaleSwitch.h>

#include "mitkIOMimeTypes.h"
#include "mitkStructuredReportWriterService.h"

mitk::StructuredReportWriterService::StructuredReportWriterService()
  : AbstractFileWriter(StructuredReport::GetStaticNameOfClass())
{
  std::string category = "MITK Structured Report File";
  mitk::CustomMimeType customMimeType;
  customMimeType.SetCategory(category);
  customMimeType.AddExtension("xml");

  SetDescription(category);
  SetMimeType(customMimeType);

  RegisterService();
}

mitk::StructuredReportWriterService::StructuredReportWriterService(const mitk::StructuredReportWriterService& other)
  : AbstractFileWriter(other)
{
}

mitk::StructuredReportWriterService::~StructuredReportWriterService()
{
}

void mitk::StructuredReportWriterService::Write()
{
  LocaleSwitch localeSwitch("C");

  LocalFile f(this);
  //std::cout << f.GetFileName() << std::endl;
  std::ofstream stream(f.GetFileName());
  if (stream.good()) {
    const size_t writeFlags = 0;
    const StructuredReport* report = static_cast<const StructuredReport*>(this->GetInput());
    const_cast<StructuredReport*>(report)->reportData->writeXML(stream, writeFlags);
  }
}

mitk::StructuredReportWriterService* mitk::StructuredReportWriterService::Clone() const
{
  return new StructuredReportWriterService(*this);
}
