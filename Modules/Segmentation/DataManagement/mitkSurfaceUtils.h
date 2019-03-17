#pragma once

#include "MitkSegmentationExports.h"

#include <mitkDataNode.h>
#include <mitkDataStorage.h>
#include <mitkEnumerationProperty.h>

#include <vtkPolyData.h>

#include <itkProgressAccumulator.h>

namespace mitk
{
class MITKSEGMENTATION_EXPORT SurfaceCreator : public itk::ProcessObject
{

public:
  typedef SurfaceCreator                Self;
  typedef itk::ProcessObject            Superclass;
  typedef itk::SmartPointer<Self>       Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  itkNewMacro(Self);
  itkTypeMacro(SurfaceCreator, itk::ProcessObject);

  enum SurfaceCreationType {
    MITK,
    AGTK,
  };

  struct SurfaceCreationArgs {
    bool recreate = false;
    SurfaceCreationType creationType = SurfaceCreationType::MITK;
    float opacity = 1.f;
    bool decimation = false;
    float decimationRate = .2f;
    bool smooth = false;
    bool lightSmoothing = false; // Determines smoothing parameters if smoothing is enabled
    DataStorage::Pointer outputStorage = nullptr;
    bool overwrite = true;
    DataNode::Pointer removeOnComplete = nullptr; // Node will be removed after creating and adding new model
  };

  class SurfaceCreationTypeProperty : public EnumerationProperty
  {
  public:
    mitkClassMacro(SurfaceCreationTypeProperty, EnumerationProperty);

    itkFactorylessNewMacro(Self);
    itkCloneMacro(Self);

    mitkNewMacro1Param(SurfaceCreationTypeProperty, const IdType&);
    mitkNewMacro1Param(SurfaceCreationTypeProperty, const std::string&);

    virtual int GetSurfaceCreationType();

    using BaseProperty::operator=;

  protected:
    SurfaceCreationTypeProperty();
    SurfaceCreationTypeProperty(const IdType& value);
    SurfaceCreationTypeProperty(const std::string& value);

    virtual void AddSurfaceCreationTypes();

  private:
    SurfaceCreationTypeProperty& operator=(const SurfaceCreationTypeProperty&);
    virtual itk::LightObject::Pointer InternalClone() const override;
  };

  virtual void Update() override
  {
    UpdateProgress(0.f);

    try {
      UpdateOutputData(nullptr);
    }
    catch (itk::ProcessAborted&) {
      InvokeEvent(itk::AbortEvent());
      ResetPipeline();
      RestoreInputReleaseDataFlags();
    }
  }

  void setInput(DataNode::Pointer segmentation)
  {
    m_Input = segmentation;
  }

  DataNode::Pointer getOutput()
  {
    return m_Output;
  }

  void setArgs(SurfaceCreationArgs args)
  {
    m_Args = args;
  }

protected:
  SurfaceCreator();
  virtual ~SurfaceCreator() {}

  void GenerateData();

  DataNode::Pointer createModel();
  DataNode::Pointer recreateModel();

  DataNode::Pointer m_Input;
  DataNode::Pointer m_Output;

  SurfaceCreationArgs m_Args;

  vtkSmartPointer<vtkPolyData> createModelMitk(DataNode::Pointer segNode, SurfaceCreator::SurfaceCreationArgs args);
  vtkSmartPointer<vtkPolyData> createModelAgtk(DataNode::Pointer segNode, SurfaceCreator::SurfaceCreationArgs args);

  itk::ProgressAccumulator::Pointer m_ProgressAccumulator;

};
}

