#include "mitkStructuredReportSerializer.h"
#include "mitkStructuredReport.h"
#include "mitkIOUtil.h"
#include <Poco/Path.h>

MITK_REGISTER_SERIALIZER(StructuredReportSerializer)

mitk::StructuredReportSerializer::StructuredReportSerializer()
{
}

mitk::StructuredReportSerializer::~StructuredReportSerializer()
{
}

std::string mitk::StructuredReportSerializer::Serialize()
{
  const StructuredReport* report = dynamic_cast<const StructuredReport*>( m_Data.GetPointer() );
  if (!report)
  {
    MITK_ERROR << " Object at " << (const void*) this->m_Data
              << " is not an mitk::StructuredReport. Cannot serialize as structured report.";
    return "";
  }

  std::string filename( this->GetUniqueFilenameInWorkingDirectory() );
  std::cout << "creating file " << filename << " in " << m_WorkingDirectory << std::endl;
  filename += "_";
  filename += m_FilenameHint;

  std::string fullname(m_WorkingDirectory);
  fullname += Poco::Path::separator();
  fullname += filename + ".xml";

  try
  {
    IOUtil::Save(report, fullname);
  }
  catch (std::exception& e)
  {
    MITK_ERROR << " Error serializing object at " << (const void*) this->m_Data
              << " to "
              << fullname
              << ": "
              << e.what();
    return "";
  }
  return Poco::Path(fullname).getFileName();// + ".pic";
}
