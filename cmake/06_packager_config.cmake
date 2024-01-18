# =================================================================================================
# Copyright (C) 2023-2024 EOLO Contributors
# =================================================================================================

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${EOLO_VERSION})
set(CPACK_PACKAGE_VENDOR "Vilas Kumar Chitrakaran")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_VENDOR})
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

set(CPACK_PACKAGE_DIRECTORY ${PROJECT_BINARY_DIR}/package)

set(CPACK_SET_DESTDIR ON)
set(CPACK_INSTALL_PREFIX "/opt/${PROJECT_NAME}")

include(CPack)
