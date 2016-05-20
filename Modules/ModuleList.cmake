# The entries in the mitk_modules list must be
# ordered according to their dependencies.

set(mitk_modules
  Logging
  Core
  AppUtil
  DataTypesExt
  Overlays
  LegacyGL
  AlgorithmsExt
  SceneSerializationBase
  PlanarFigure
  ImageExtraction
  ImageStatistics
  SceneSerialization
  QtWidgets
  QtWidgetsExt
)

if(MITK_ENABLE_PIC_READER)
  list(APPEND mitk_modules IpPicSupportIO)
endif()
