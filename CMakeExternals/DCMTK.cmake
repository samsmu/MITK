#-----------------------------------------------------------------------------
# DCMTK
#-----------------------------------------------------------------------------

if(MITK_USE_DCMTK)

  # Sanity checks
  if(DEFINED DCMTK_DIR AND NOT EXISTS ${DCMTK_DIR})
    message(FATAL_ERROR "DCMTK_DIR variable is defined but corresponds to non-existing directory")
  endif()

  set(proj DCMTK)
  set(proj_DEPENDENCIES )
  if(WIN32)
      set(proj_DEPENDENCIES LIBICONV)
  endif()
  set(DCMTK_DEPENDS ${proj})

  if(NOT DEFINED DCMTK_DIR)
    if(DCMTK_DICOM_ROOT_ID)
      set(DCMTK_CXX_FLAGS "${DCMTK_CXX_FLAGS} -DSITE_UID_ROOT=\\\"${DCMTK_DICOM_ROOT_ID}\\\"")
      set(DCMTK_C_FLAGS "${DCMTK_CXX_FLAGS} -DSITE_UID_ROOT=\\\"${DCMTK_DICOM_ROOT_ID}\\\"")
    endif()

    set(additional_args )
    if(CTEST_USE_LAUNCHERS)
      list(APPEND additional_args
        "-DCMAKE_PROJECT_${proj}_INCLUDE:FILEPATH=${CMAKE_ROOT}/Modules/CTestUseLaunchers.cmake"
      )
    endif()
    if(WIN32)
      if(LIBICONV_DIR STREQUAL "")
        set(LIBICONV_DIR ${ep_prefix})
      endif()
      list(APPEND additional_args
        "-DDCMTK_WIDE_CHAR_FILE_IO_FUNCTIONS:BOOL=ON"
        "-DDCMTK_WIDE_CHAR_MAIN_FUNCTION:BOOL=ON"
        "-DDCMTK_WITH_ICONV:BOOL=ON"
        "-DLIBICONV_DIR:PATH=${LIBICONV_DIR}"
        "-DDCMTK_ENABLE_CHARSET_CONVERSION:STRING=libiconv"
      )
    endif()

    ExternalProject_Add(${proj}
      LIST_SEPARATOR ${sep}
      GIT_REPOSITORY "git@github.com:samsmu/dcmtk.git"
      GIT_TAG "e119e9dd1bd0c7044ba4017cd6c6de825994cf6a"
      CMAKE_GENERATOR ${gen}
      CMAKE_ARGS
         ${ep_common_args}
         ${additional_args}
         "-DCMAKE_CXX_FLAGS:STRING=${CMAKE_CXX_FLAGS} ${DCMTK_CXX_FLAGS}"
         "-DCMAKE_C_FLAGS:STRING=${CMAKE_C_FLAGS} ${DCMTK_C_FLAGS}"
         -DDCMTK_ENABLE_CXX11:BOOL=ON
         -DDCMTK_ENABLE_STL:BOOL=ON
         -DDCMTK_WITH_DOXYGEN:BOOL=OFF
         -DDCMTK_WITH_ZLIB:BOOL=OFF # see bug #9894
         -DDCMTK_WITH_OPENSSL:BOOL=OFF # see bug #9894
         -DDCMTK_WITH_PNG:BOOL=OFF # see bug #9894
         -DDCMTK_WITH_TIFF:BOOL=OFF  # see bug #9894
         -DDCMTK_WITH_XML:BOOL=OFF  # see bug #9894
      CMAKE_CACHE_ARGS
        ${ep_common_cache_args}
      CMAKE_CACHE_DEFAULT_ARGS
        ${ep_common_cache_default_args}
      DEPENDS ${proj_DEPENDENCIES}
      )
    set(DCMTK_DIR ${ep_prefix})
    mitkFunctionInstallExternalCMakeProject(${proj})

  else()

    mitkMacroEmptyExternalProject(${proj} "${proj_DEPENDENCIES}")

  endif()
endif()
