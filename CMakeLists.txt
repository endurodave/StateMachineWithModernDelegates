# Example CMake command line to create project build files:
#
# *** Windows ***
# cmake -G "Visual Studio 17 2022" -A Win32 -B Build -S .
# cmake -G "Visual Studio 17 2022" -A x64 -B Build -S .
#
# *** Linux ***
# cmake -G "Unix Makefiles" -B Build -S .
# cmake -G "Unix Makefiles" -B Build -S . -DENABLE_UNIT_TESTS=ON

# Specify the minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name and language (C or C++)
project(Delegate VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Collect all .cpp and *.h source files in the current directory
file(GLOB SOURCES "${CMAKE_SOURCE_DIR}/*.cpp" "${CMAKE_SOURCE_DIR}/*.h")

# Collect all header files in the Delegate directory
file(GLOB Delegate_HEADERS "${CMAKE_SOURCE_DIR}/Delegate/*.h")

# Organize Delegate headers into a "Delegate Files" folder in Visual Studio
source_group("Delegate Files" FILES ${Delegate_HEADERS})

# Add subdirectories to include path
include_directories( 
    ${CMAKE_SOURCE_DIR}/Delegate
    ${CMAKE_SOURCE_DIR}/SelfTest
    ${CMAKE_SOURCE_DIR}/StateMachine
    ${CMAKE_SOURCE_DIR}/Port
)

# Add an executable target
add_executable(DelegateApp ${SOURCES} ${Delegate_HEADERS})

# Define the DELEGATE_UNIT_TEST macro for the DelegateApp target
if (ENABLE_UNIT_TESTS)
    add_compile_definitions(DELEGATE_UNIT_TESTS)
endif()

# Add subdirectories to build
add_subdirectory(SelfTest)
add_subdirectory(StateMachine)
add_subdirectory(Port)

target_link_libraries(DelegateApp PRIVATE 
    SelfTestLib
    StateMachineLib
    PortLib
)

