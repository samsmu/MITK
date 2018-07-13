find_package(OpenSSL REQUIRED)

#INCLUDE_DIRS("${OPENSSL_INCLUDE_DIR}")

MITK_CREATE_MODULE(
  DEPENDS MitkUtilities
  PACKAGE_DEPENDS PUBLIC Boost
)

#if (UNIX)
# -L /usr/lib
set_target_properties(${MODULE_TARGET} PROPERTIES LINK_FLAGS "-lpthread -lcrypto -lssl")
#endif (UNIX)

#${OPENSSL_LIBRARIES}
