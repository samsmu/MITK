#pragma once

#include "mitkBaseDataSerializer.h"

namespace mitk
{
/**
  \brief Serializes mitk::StructuredReport for mitk::SceneIO
*/
class StructuredReportSerializer : public BaseDataSerializer
{
  public:

    mitkClassMacro(StructuredReportSerializer, BaseDataSerializer );
    itkFactorylessNewMacro(Self)
    itkCloneMacro(Self)

    virtual std::string Serialize() override;

  protected:

    StructuredReportSerializer();
    virtual ~StructuredReportSerializer();
};

} // namespace
