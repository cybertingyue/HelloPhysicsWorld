cmake_minimum_required(VERSION 3.20)

include("${CMAKE_CURRENT_LIST_DIR}/jolt_install_path.cmake")
get_filename_component(root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(work "${root}/temp/JoltPhysics/${jolt_version}")
set(source "${work}/source")

set(build "${work}/${jolt_platform}")
set(prefix "${root}/${jolt_install_dir}")

find_package(Git REQUIRED)
if(NOT EXISTS "${source}/.git")
    execute_process(COMMAND "${GIT_EXECUTABLE}" clone --depth 1 --branch "${jolt_version}"
        https://github.com/jrouwe/JoltPhysics.git "${source}"
        COMMAND_ERROR_IS_FATAL ANY)
endif()

execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${source}" rev-parse HEAD
    OUTPUT_VARIABLE revision OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
if(NOT revision STREQUAL "cf16769bb0ede9c45d57d06068ff0689e9702f5c")
    message(FATAL_ERROR "Expected Jolt ${jolt_version} at ${source}, found ${revision}")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" -S "${source}/Build" -B "${build}"
    "-DCMAKE_INSTALL_PREFIX=${prefix}"
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_BUILD_TYPE=Release
    -DDEBUG_RENDERER_IN_DEBUG_AND_RELEASE=ON
    -DJPH_BUILD_SHARED_LIBS=ON
    -DENABLE_INSTALL=ON
    -DCPP_RTTI_ENABLED=ON
    -DUSE_STATIC_MSVC_RUNTIME_LIBRARY=OFF
    -DJPH_USE_DX12=OFF -DJPH_USE_VK=OFF -DJPH_USE_MTL=OFF
    -DTARGET_UNIT_TESTS=OFF -DTARGET_HELLO_WORLD=OFF -DTARGET_PERFORMANCE_TEST=OFF
    -DTARGET_SAMPLES=OFF -DTARGET_VIEWER=OFF
    COMMAND_ERROR_IS_FATAL ANY)
if(NOT DEFINED ENV{CMAKE_BUILD_PARALLEL_LEVEL})
    set(ENV{CMAKE_BUILD_PARALLEL_LEVEL} 4)
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${build}" --config Release --target Jolt
    COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${CMAKE_COMMAND}" --install "${build}" --config Release
    COMMAND_ERROR_IS_FATAL ANY)
message(STATUS "Installed Jolt ${jolt_version} in ${prefix}")
