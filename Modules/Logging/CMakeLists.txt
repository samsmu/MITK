find_package(OpenSSL REQUIRED)

if(OPENSSL_FOUND)
    include_directories("${OPENSSL_INCLUDE_DIR}")
endif()

MITK_CREATE_MODULE(
  DEPENDS MitkUtilities
  PACKAGE_DEPENDS PUBLIC Boost
  ADDITIONAL_LIBS ${OPENSSL_LIBRARIES}
)

if (UNIX)
  set_target_properties(${MODULE_TARGET} PROPERTIES LINK_FLAGS "-lpthread -lcrypto -lssl")
endif (UNIX)
