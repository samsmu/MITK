#pragma once 

#include <mitkAbstractFileReader.h>
#include <mitkStructuredReport.h>

namespace mitk
{
class StructuredReportReaderService: public AbstractFileReader
{
public:

  StructuredReportReaderService();
  virtual ~StructuredReportReaderService();

  using AbstractFileReader::Read;
  virtual std::vector<itk::SmartPointer<BaseData>> Read() override;

private:

  StructuredReportReaderService(const StructuredReportReaderService& other);

  virtual StructuredReportReaderService* Clone() const override;
};
}
