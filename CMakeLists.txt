cmake_minimum_required(VERSION 3.31)
project(D3D12_3D_Animation)

set(CMAKE_CXX_STANDARD 20)

add_executable(D3D12_3D_Animation WIN32
        src/main.cpp
        src/application.cpp
        src/application.hpp
        src/application.cpp
        src/application.hpp
        src/camera.cpp
        src/camera.hpp
        src/model.cpp
        src/model.hpp
        src/mesh.cpp
        src/mesh.hpp
        src/texture.cpp
        src/texture.hpp
        src/utils.cpp
        src/utils.hpp
        src/bone.cpp
        src/bone.hpp)

add_subdirectory(dependencies/stb/)

target_include_directories(${PROJECT_NAME} PRIVATE
        dependencies/assimp/include
        dependencies/d3dx12/include
        dependencies/glm/include
        dependencies/assimp/include
)

set(ASSIMP_LIB ${CMAKE_CURRENT_LIST_DIR}/dependencies/assimp/lib/assimp-vc143-mt.lib)

target_link_libraries(${PROJECT_NAME} PRIVATE
        d3d12.lib
        dxgi.lib
        d3dcompiler.lib
        dxguid.lib
        stb
        ${ASSIMP_LIB}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_RADIANS
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_LEFT_HANDED
)
