cmake_minimum_required(VERSION 4.3)
project(Fishmapper)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(pointmapper
        GIT_REPOSITORY https://github.com/EggAllocationService/Pointmapper.git
        GIT_TAG d7d874f
)
FetchContent_MakeAvailable(pointmapper)

add_executable(Fishmapper main.cpp
        components/BoidsComponent.cpp
        components/BoidsComponent.h
        components/TexturedModelComponent.cpp
        components/TexturedModelComponent.h)
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

create_resource_library(
        NAME BoidShaders
        SOURCES components/boids.wgsl components/boidshaders.wgsl components/texture.wgsl
        RAW_SOURCES boidresources.pak
)

target_include_directories(Fishmapper PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(Fishmapper PUBLIC pointmapper BoidShaders)
