MITK_CREATE_MODULE(DEPENDS MitkDataTypesExt MitkMapperExt MitkSceneSerialization MitkLegacyIO
                   PACKAGE_DEPENDS PRIVATE ITK|ITKIOImageBase VTK
                   AUTOLOAD_WITH MitkCore
                  )
