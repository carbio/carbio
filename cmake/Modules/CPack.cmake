#----------------------------------------------------------------------
include_guard(GLOBAL)
set(CPACK_VERBATIM_VARIABLES ON)
set(CPACK_STRIP_FILES ON)
set(CPACK_GENERATOR "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_PACKAGE_FILE_EXTENSION "tar.gz")
if (WIN32)
set(CPACK_GENERATOR "ZIP")
set(CPACK_SOURCE_GENERATOR "ZIP")
set(CPACK_PACKAGE_FILE_EXTENSION "zip")
endif()
include(CPack)
#----------------------------------------------------------------------