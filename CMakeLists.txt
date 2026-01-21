cmake_minimum_required(VERSION 4.0)

# Generate compile commands for anyone using our libraries.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)

project(metal-cpp LANGUAGES CXX)

if(${CMAKE_CXX_CLANG_TIDY})
    message(STATUS "${CMAKE_CXX_CLANG_TIDY} found!")
endif()

static_library(
    ENABLE_TESTS OFF
)

set_packages(

    PACKAGES
    metal-cpp

    LINK_PACKAGES
    metal-cpp::metal-cpp
    "-framework Metal"
    "-framework Foundation"
    "-framework Cocoa"
    "-framework CoreGraphics"
    "-framework MetalKit" 
)


add_executable(sandbox sandbox/application.cpp)

target_link_libraries(sandbox PUBLIC metal-cpp)

target_include_directories(sandbox PUBLIC sandbox)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

# if(${CMAKE_CXX_CLANG_TIDY})
#     # Define a custom command that runs clang-tidy
#     # ${CHECK_FILES} is a variable we will pass from the terminal
#     add_custom_target(tidy
#         COMMAND ${CMAKE_CXX_CLANG_TIDY} -p ${CMAKE_BINARY_DIR} ${CHECK_FILES}
#         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#         COMMENT "Running clang-tidy on selected files..."
#     )
# endif()

target_sources(${PROJECT_NAME} PUBLIC
    FILE_SET CXX_MODULES
    TYPE CXX_MODULES
    FILES
    metal-cpp/metal_cpp.cppm
)


install(
    TARGETS ${PROJECT_NAME}
    EXPORT metal-cpp_targets
    FILE_SET CXX_MODULES DESTINATION "."
    LIBRARY DESTINATION "lib"
    ARCHIVE DESTINATION "lib"
    CXX_MODULES_BMI DESTINATION "bmi"
)

install(
    EXPORT metal-cpp_targets
    FILE "metal-cpp-config.cmake"
    DESTINATION "lib/cmake"
    CXX_MODULES_DIRECTORY "cxx-modules"
)

# Always run this custom target by making it depend on ALL
add_custom_target(copy_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
    DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
)
