# Findrockchip_mpp
# -----------
#
# Find the Rockchip MPP library.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# ``ROCKCHIP_MPP_FOUND``
#   System has the Rockchip MPP library.
# ``ROCKCHIP_MPP_INCLUDE_DIR``
#   The Rockchip MPP include directory.
# ``ROCKCHIP_MPP_LIBRARIES``
#   All Rockchip MPP libraries.
#
# Hints
# ^^^^^
#

find_package(PkgConfig QUIET)
pkg_check_modules(ROCKCHIP_MPP QUIET "rockchip_mpp")

include(FindPackageHandleStandardArgs)
find_path(ROCKCHIP_MPP_INCLUDE_DIR 
    NAMES 
        rockchip/rk_mpi.h
    )
find_library(ROCKCHIP_MPP_LIBRARIES 
    NAMES
        rockchip_mpp
    )

find_package_handle_standard_args(ROCKCHIP_MPP
    DEFAULT_MSG
    ROCKCHIP_MPP_INCLUDE_DIR
    ROCKCHIP_MPP_LIBRARIES
    )

mark_as_advanced(
    ROCKCHIP_MPP_INCLUDE_DIR
    ROCKCHIP_MPP_LIBRARIES
    )
