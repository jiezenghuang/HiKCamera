cmake_minimum_required (VERSION 3.12)

project (HiKCameraPlugin 
    DESCRIPTION "HiKVision Camera Plugin"
    VERSION 0.0.1
    LANGUAGES CXX C)

list(APPEND CMAKE_MODULE_PATH "$ENV{FAL_ROOT}/cmake")

find_package(fal REQUIRED)

find_package(MVS REQUIRED)

set(PLUGIN_INSTALL_LIBDIR "plugin/camera/HiKRobotic")

add_subdirectory(src)
add_subdirectory(app)