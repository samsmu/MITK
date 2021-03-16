#-----------------------------------------------------------------------------
# VTK
#-----------------------------------------------------------------------------

if(WIN32)
  option(VTK_USE_SYSTEM_FREETYPE OFF)
else(WIN32)
  option(VTK_USE_SYSTEM_FREETYPE ON)
endif(WIN32)

# Sanity checks
if(DEFINED VTK_DIR AND NOT EXISTS ${VTK_DIR})
  message(FATAL_ERROR "VTK_DIR variable is defined but corresponds to non-existing directory")
endif()

set(proj VTK)
set(proj_DEPENDENCIES )
set(VTK_DEPENDS ${proj})

if(MITK_USE_HDF5)
  list(APPEND proj_DEPENDENCIES HDF5)
endif()

if(NOT DEFINED VTK_DIR)

  set(additional_cmake_args )
  if(MINGW)
    set(additional_cmake_args
        -DCMAKE_USE_WIN32_THREADS:BOOL=ON
        -DCMAKE_USE_PTHREADS:BOOL=OFF
        -DVTK_USE_VIDEO4WINDOWS:BOOL=OFF # no header files provided by MinGW
        )
  endif()

  # Optionally enable memory leak checks for any objects derived from vtkObject. This
  # will force unit tests to fail if they have any of these memory leaks.
  option(MITK_VTK_DEBUG_LEAKS OFF)
  mark_as_advanced(MITK_VTK_DEBUG_LEAKS)
  list(APPEND additional_cmake_args
       -DVTK_DEBUG_LEAKS:BOOL=${MITK_VTK_DEBUG_LEAKS}
      )

  if(MITK_USE_Python)
    if(NOT MITK_USE_SYSTEM_PYTHON)
     list(APPEND proj_DEPENDENCIES Python)
     set(_vtk_install_python_dir -DVTK_INSTALL_PYTHON_MODULE_DIR:FILEPATH=${MITK_PYTHON_SITE_DIR})
    else()
      set(_vtk_install_python_dir -DVTK_INSTALL_PYTHON_MODULE_DIR:PATH=${ep_prefix}/lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages)
    endif()

    list(APPEND additional_cmake_args
         -DVTK_WRAP_PYTHON:BOOL=ON
         -DVTK_USE_TK:BOOL=OFF
         -DVTK_WINDOWS_PYTHON_DEBUGGABLE:BOOL=OFF
         -DPYTHON_EXECUTABLE:FILEPATH=${PYTHON_EXECUTABLE}
         -DPYTHON_INCLUDE_DIR:PATH=${PYTHON_INCLUDE_DIR}
         -DPYTHON_INCLUDE_DIR2:PATH=${PYTHON_INCLUDE_DIR2}
         -DPYTHON_LIBRARY:FILEPATH=${PYTHON_LIBRARY}
         ${_vtk_install_python_dir}
        )
  else()
    list(APPEND additional_cmake_args
         -DVTK_WRAP_PYTHON:BOOL=OFF
         -DVTK_WINDOWS_PYTHON_DEBUGGABLE:BOOL=OFF
        )
  endif()

  if(MITK_USE_QT)
    list(APPEND additional_cmake_args
        -DVTK_QT_VERSION:STRING=5
        -DVTK_Group_Qt:BOOL=ON
        -DVTK_INSTALL_NO_QT_PLUGIN:BOOL=ON
     )
  endif()

  if(CTEST_USE_LAUNCHERS)
    list(APPEND additional_cmake_args
      "-DCMAKE_PROJECT_${proj}_INCLUDE:FILEPATH=${CMAKE_ROOT}/Modules/CTestUseLaunchers.cmake"
    )
  endif()
  
  if(MITK_PDB)
    list(APPEND additional_cmake_args
      "-DCMAKE_CXX_FLAGS_RELEASE:STRING=/MD /Zi /O2 /Ob1 /DNDEBUG"
      "-DCMAKE_SHARED_LINKER_FLAGS_RELEASE:STRING=/DEBUG /INCREMENTAL /OPT:REF /OPT:ICF"
      "-DCMAKE_EXE_LINKER_FLAGS_RELEASE:STRING=/DEBUG /INCREMENTAL /OPT:REF /OPT:ICF"
      "-DCMAKE_MODULE_LINKER_FLAGS_RELEASE:STRING=/DEBUG /INCREMENTAL /OPT:REF /OPT:ICF"
      "-DCMAKE_PDB_OUTPUT_DIRECTORY_RELEASE:FILEPATH=${ep_prefix}/bin"
      "-DCMAKE_PDB_OUTPUT_DIRECTORY_DEBUG:FILEPATH=${ep_prefix}/bin"
    )
  endif()

  ExternalProject_Add(${proj}
    LIST_SEPARATOR ${sep}
    URL http://mitk.org/download/thirdparty/VTK-8.1.0.tar.gz
    URL_MD5 4fa5eadbc8723ba0b8d203f05376d932
    PATCH_COMMAND ${PATCH_COMMAND} -N -p1 -i ${CMAKE_CURRENT_LIST_DIR}/VtkCornerAnnotation.patch
      COMMAND ${PATCH_COMMAND} -N -p1 -i ${CMAKE_CURRENT_LIST_DIR}/VtkZShiftFix.patch
      COMMAND ${PATCH_COMMAND} -N -p1 -i ${CMAKE_CURRENT_LIST_DIR}/CommonVTK.patch
      COMMAND ${PATCH_COMMAND} -N -p1 -i ${CMAKE_CURRENT_LIST_DIR}/VtkRotatedVolumes.patch
      COMMAND ${PATCH_COMMAND} -N -p1 -i ${CMAKE_CURRENT_LIST_DIR}/VTK-gcc-10.patch
      COMMAND ${PATCH_COMMAND} -N -p1 -i ${CMAKE_CURRENT_LIST_DIR}/VtkClippingJitter.patch
      COMMAND ${PATCH_COMMAND} -N -p1 -i ${CMAKE_CURRENT_LIST_DIR}/VTK-Qt515.patch
    CMAKE_GENERATOR ${gen}
    CMAKE_ARGS
        ${ep_common_args}
        -DVTK_WRAP_TCL:BOOL=OFF
        -DVTK_WRAP_PYTHON:BOOL=OFF
        -DVTK_WRAP_JAVA:BOOL=OFF
        -DVTK_USE_SYSTEM_FREETYPE:BOOL=${VTK_USE_SYSTEM_FREETYPE}
        -DVTK_LEGACY_REMOVE:BOOL=ON
        -DModule_vtkTestingRendering:BOOL=ON
        -DVTK_MAKE_INSTANTIATORS:BOOL=ON
        -DVTK_USE_CXX11_FEATURES:BOOL=ON
        -DVTK_RENDERING_BACKEND:STRING=OpenGL2
        -DVTK_SMP_IMPLEMENTATION_TYPE=OpenMP
        ${additional_cmake_args}
    CMAKE_CACHE_ARGS
      ${ep_common_cache_args}
    CMAKE_CACHE_DEFAULT_ARGS
      ${ep_common_cache_default_args}
    DEPENDS ${proj_DEPENDENCIES}
  )

  set(VTK_DIR ${ep_prefix})
  mitkFunctionInstallExternalCMakeProject(${proj})

else()

  mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")

endif()
