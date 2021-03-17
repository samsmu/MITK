MITK_CREATE_MODULE(
  INCLUDE_DIRS PRIVATE resource # for xpm includes
  DEPENDS MitkPlanarFigure MitkOverlays MitkConfig MitkUtilities MitkAnnotation
  PACKAGE_DEPENDS
    PUBLIC ITK|ITKIOImageBase VTK|vtkGUISupportQt+vtkRenderingQt Qt5|Widgets+OpenGL+Core Boost
  SUBPROJECTS MITK-CoreUI
)

add_subdirectory(test)
