cmake_minimum_required(VERSION 3.31)
project(D3D12_3D_Animation)

set(CMAKE_CXX_STANDARD 20)

add_executable(D3D12_3D_Animation WIN32 src/main.cpp
        src/application.cpp
        src/application.hpp
        src/application.cpp
        src/application.hpp
        src/camera.cpp
        src/camera.hpp)

add_subdirectory(dependencies/stb/)

target_include_directories(${PROJECT_NAME} PRIVATE
        dependencies/assimp/include
        dependencies/stb/include
        dependencies/d3dx12/include
        dependencies/glm/include
)

file(GLOB ASSIMP_LIBS dependencies/assimp/lib/*.a)

target_link_libraries(${PROJECT_NAME} PRIVATE
        stb
        ${ASSIMP_LIBS}
        d3d12.lib
        dxgi.lib
        d3dcompiler.lib
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_RADIANS
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_LEFT_HANDED
)

target_link_options(${PROJECT_NAME} PRIVATE -static)
