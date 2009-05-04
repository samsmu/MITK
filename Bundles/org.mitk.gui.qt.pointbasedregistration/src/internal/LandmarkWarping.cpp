#include "LandmarkWarping.h"

LandmarkWarping::LandmarkWarping()
{
  m_Deformer = DeformationSourceType::New();
  m_LandmarkDeformer = DeformationSourceType::New();
}

LandmarkWarping::~LandmarkWarping()
{
}

void LandmarkWarping::SetFixedImage(FixedImageType::Pointer fixedImage)
{
  m_FixedImage = fixedImage;
  m_Deformer->SetOutputSpacing( m_FixedImage->GetSpacing() );
  m_Deformer->SetOutputOrigin(  m_FixedImage->GetOrigin() );
  m_Deformer->SetOutputRegion(  m_FixedImage->GetLargestPossibleRegion() );
}

void LandmarkWarping::SetMovingImage(MovingImageType::Pointer movingImage)
{
  m_MovingImage = movingImage;
  m_LandmarkDeformer->SetOutputSpacing( m_MovingImage->GetSpacing() );
  m_LandmarkDeformer->SetOutputOrigin(  m_MovingImage->GetOrigin() );
  m_LandmarkDeformer->SetOutputRegion(  m_MovingImage->GetLargestPossibleRegion() );
}

void LandmarkWarping::SetLandmarks(LandmarkContainerType::Pointer source, LandmarkContainerType::Pointer target)
{
  m_SourceLandmarks = source;
  m_TargetLandmarks = target;
  m_Deformer->SetSourceLandmarks( source );
  m_Deformer->SetTargetLandmarks( target );
  m_LandmarkDeformer->SetSourceLandmarks( target );
  m_LandmarkDeformer->SetTargetLandmarks( source );
}

LandmarkWarping::MovingImageType::Pointer LandmarkWarping::Register()
{
  try
  {
    m_Observer = Observer::New();
    unsigned long obs = m_Deformer->AddObserver(itk::ProgressEvent(), m_Observer);
    mitk::ProgressBar::GetInstance()->AddStepsToDo(120);
    m_Deformer->UpdateLargestPossibleRegion();
    m_Deformer->RemoveObserver(obs);
  }
  catch( itk::ExceptionObject & excp )
  {
    std::cerr << "Exception thrown " << std::endl;
    std::cerr << excp << std::endl;
    return NULL;
  }

  try
  {
    unsigned long obs2 = m_LandmarkDeformer->AddObserver(itk::ProgressEvent(), m_Observer);
    m_LandmarkDeformer->UpdateLargestPossibleRegion();
    m_LandmarkDeformer->RemoveObserver(obs2);
  }
  catch( itk::ExceptionObject & excp )
  {
    std::cerr << "Exception thrown " << std::endl;
    std::cerr << excp << std::endl;
    return NULL;
  }

  m_DeformationField = m_Deformer->GetOutput();
  m_InverseDeformationField = m_LandmarkDeformer->GetOutput();

  m_Warper = FilterType::New();

  typedef itk::LinearInterpolateImageFunction< 
                       MovingImageType, double >  InterpolatorType;

  InterpolatorType::Pointer interpolator = InterpolatorType::New();

  m_Warper->SetInterpolator( interpolator );


  m_Warper->SetOutputSpacing( m_DeformationField->GetSpacing() );
  m_Warper->SetOutputOrigin(  m_DeformationField->GetOrigin() );

  m_Warper->SetDeformationField( m_DeformationField );

  m_Warper->SetInput( m_MovingImage );

  unsigned long obs3 = m_Warper->AddObserver(itk::ProgressEvent(), m_Observer);
  m_Warper->UpdateLargestPossibleRegion();
  m_Warper->RemoveObserver(obs3);  

  return m_Warper->GetOutput();
}

LandmarkWarping::LandmarkContainerType::Pointer LandmarkWarping::GetTransformedTargetLandmarks()
{
  LandmarkContainerType::Pointer landmarks = LandmarkContainerType::New();
  LandmarkWarping::LandmarkPointType transformedTargetPoint;

  for(unsigned int pointId=0; pointId<m_TargetLandmarks->Size();++pointId)
  {
    LandmarkWarping::LandmarkPointType targetPoint=m_TargetLandmarks->GetElement(pointId);
    transformedTargetPoint  = m_LandmarkDeformer->GetKernelTransform()->TransformPoint(targetPoint);
    landmarks->InsertElement(pointId, transformedTargetPoint );
  }
  return landmarks;
}
