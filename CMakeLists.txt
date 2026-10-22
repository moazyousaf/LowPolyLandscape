cmake_minimum_required(VERSION 3.5...4.0)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "Minimum policy version for all subprojects")

project(LowPolyLandscape LANGUAGES CXX)
set(CMAKE_TLS_VERIFY OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(FetchContent)

############################
### Library dependencies ###
############################

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
)
FetchContent_MakeAvailable(glm)

add_library(common INTERFACE)
target_compile_features(common INTERFACE cxx_std_17)
target_compile_options(common INTERFACE
  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>: -Wall -DGL_SILENCE_DEPRECATION>
  $<$<CXX_COMPILER_ID:MSVC>: /W4>
)

find_package(OpenGL REQUIRED)

# Glad (copiato manualmente nella cartella glad/)
set(GLAD_SRC     ${CMAKE_SOURCE_DIR}/glad/src/glad.c)
set(GLAD_INCLUDE ${CMAKE_SOURCE_DIR}/glad/include)

###############################
### Sources and executables ###
###############################

add_executable(Tappa01  Tappa01/main.cpp  ${GLAD_SRC})
target_include_directories(Tappa01 PRIVATE ${GLAD_INCLUDE})
target_link_libraries(Tappa01 PRIVATE common SFML::Window OpenGL::GL glm::glm-header-only)