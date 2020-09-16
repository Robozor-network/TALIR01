INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_TALIR talir)

FIND_PATH(
    TALIR_INCLUDE_DIRS
    NAMES talir/api.h
    HINTS $ENV{TALIR_DIR}/include
        ${PC_TALIR_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /var/empty/local/include
          /var/empty/include
)

FIND_LIBRARY(
    TALIR_LIBRARIES
    NAMES gnuradio-talir
    HINTS $ENV{TALIR_DIR}/lib
        ${PC_TALIR_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /var/empty/local/lib
          /var/empty/local/lib64
          /var/empty/lib
          /var/empty/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TALIR DEFAULT_MSG TALIR_LIBRARIES TALIR_INCLUDE_DIRS)
MARK_AS_ADVANCED(TALIR_LIBRARIES TALIR_INCLUDE_DIRS)

