# The entries in the mitk_modules list must be
# ordered according to their dependencies.

set(mitk_modules
  Utilities
  Logging
  Config
  Core
  CommandLine
  AppUtil
  DCMTesting
  RDF
  LegacyIO
  DataTypesExt
  Overlays
  LegacyGL
  AlgorithmsExt
  MapperExt
  DICOMReader
  DICOMReaderServices
  DICOMTesting
  SceneSerializationBase
  PlanarFigure
  ImageDenoising
  ImageExtraction
  ImageStatistics
  LegacyAdaptors
  SceneSerialization
  Gizmo
  GraphAlgorithms
  Multilabel
  ContourModel
  SurfaceInterpolation
  Segmentation
  MatchPointRegistration
  MatchPointRegistrationUI
  PlanarFigureSegmentation
  QtWidgets
  QtWidgetsExt
  QtWidgetsWeb
  SegmentationUI
  DiffusionImaging
  GPGPU
  OpenIGTLink
  IGTBase
  IGT
  CameraCalibration
  RigidRegistration
  RigidRegistrationUI
  DeformableRegistration
  DeformableRegistrationUI
  OpenCL
  OpenCVVideoSupport
  QtOverlays
  ToFHardware
  ToFProcessing
  ToFUI
  US
  USUI
  DicomUI
  Remeshing
  Python
  QtPython
  Persistence
  OpenIGTLinkUI
  IGTUI
  DicomRT
  RTUI
  IOExt
  XNAT
  TubeGraph
  BiophotonicsHardware
  Classification
  TumorInvasionAnalysis
)

if(MITK_ENABLE_PIC_READER)
  list(APPEND mitk_modules IpPicSupportIO)
endif()
