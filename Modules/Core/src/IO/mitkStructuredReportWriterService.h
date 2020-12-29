#pragma once

#include <mitkAbstractFileWriter.h>
#include <mitkStructuredReport.h>

namespace mitk
{

class StructuredReportWriterService : public AbstractFileWriter
{
public:

  StructuredReportWriterService();
  virtual ~StructuredReportWriterService();

  using AbstractFileWriter::Write;
  virtual void Write() override;

private:

  StructuredReportWriterService(const StructuredReportWriterService& other);

  virtual mitk::StructuredReportWriterService* Clone() const override;
};
}
