if(MITK_USE_Boost_LIBRARIES)
  find_package(Boost 1.60.0 REQUIRED COMPONENTS ${MITK_USE_Boost_LIBRARIES} QUIET)
else()
  find_package(Boost 1.60.0 REQUIRED QUIET)
endif()

list(APPEND ALL_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIRS})

if(Boost_LIBRARIES)
  if(WIN32)
    # Force dynamic linking
    list(APPEND ALL_COMPILE_OPTIONS -DBOOST_ALL_DYN_LINK)
  else()
    set(${_boost_log_setup_lib} NOTFOUND)
    foreach(_boost_lib ${Boost_LIBRARIES})
      if(${_boost_lib} MATCHES "boost_log")
        string(REPLACE "boost_log" "boost_log_setup" _boost_log_setup_lib ${_boost_lib})
      endif()
    endforeach()
    if(_boost_log_setup_lib)
      list(APPEND Boost_LIBRARIES ${_boost_log_setup_lib})
    endif()

    # Boost has an auto link feature (pragma comment lib) for Windows
    list(APPEND ALL_LIBRARIES ${Boost_LIBRARIES})
  endif()
endif()
