cmake_minimum_required(VERSION 4.3)
project(Fishmapper)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
set(CMAKE_OSX_DEPLOYMENT_TARGET 26.0)

include(FetchContent)
FetchContent_Declare(pointmapper
        GIT_REPOSITORY https://github.com/EggAllocationService/Pointmapper.git
        GIT_TAG 175d7d310b240600d0f596cf8fde31ba3df12bc6
)
FetchContent_MakeAvailable(pointmapper)

add_executable(Fishmapper MACOSX_BUNDLE main.cpp
        components/BoidsComponent.cpp
        components/BoidsComponent.h
        components/TexturedModelComponent.cpp
        components/TexturedModelComponent.h
        actors/PilotableCloud.cpp
        actors/PilotableCloud.h)
build_pak(
        NAME boidresources
        SOURCES
            assets/goldfish_fixed.obj
            assets/cylinder.obj
            assets/sand.jpg
        NAMES
            /models/goldfish.obj
            /models/env.obj
            /textures/sand.jpg
)

install(CODE "
    INCLUDE(BundleUtilities)
    fixup_bundle(${CMAKE_INSTALL_PREFIX}/Fishmapper.app \"\" \"\")
")

create_resource_library(
        NAME BoidShaders
        SOURCES components/boids.wgsl components/boidshaders.wgsl components/texture.wgsl components/voxels.wgsl components/fog.wgsl
        RAW_SOURCES boidresources.pak
)

target_include_directories(Fishmapper PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(Fishmapper PUBLIC pointmapper BoidShaders)
