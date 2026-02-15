cmake_minimum_required(VERSION 3.30)

project(UltrasoundProcessor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

include(cmake/ProjectOptions.cmake)
include(CMakeDependentOption)
include(CTest)

option(ULTRASOUND_BUILD_TESTS "Build unit tests" ON)
option(ULTRASOUND_ENABLE_COVERAGE "Enable coverage reporting target" OFF)
cmake_dependent_option(ULTRASOUND_WITH_VISUALIZER
    "Build ImGui-based ultrasound visualizer"
    OFF
    "NOT CMAKE_CROSSCOMPILING"
    OFF
)

add_library(ultrasound_core
    src/core/processor.cpp
)

target_include_directories(ultrasound_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(ultrasound_core PUBLIC cxx_std_20)

add_library(ultrasound_io
    src/io/replay_source.cpp
    src/io/runtime_stub.cpp
    src/io/config_loader.cpp
)

target_include_directories(ultrasound_io
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(ultrasound_io PUBLIC cxx_std_20)

target_link_libraries(ultrasound_io PUBLIC ultrasound_core)

add_executable(uss_replay_runner
    apps/replay_runner.cpp
)

target_link_libraries(uss_replay_runner PRIVATE ultrasound_core ultrasound_io)

add_executable(uss_legacy_capture_convert
    apps/legacy_capture_convert.cpp
)
target_link_libraries(uss_legacy_capture_convert PRIVATE ultrasound_io ultrasound_core)

if (ULTRASOUND_WITH_VISUALIZER)
    find_package(glfw3 QUIET)
    find_package(glew QUIET)
    find_package(OpenGL REQUIRED)
    find_package(imgui QUIET)

    set(ULTRASOUND_GLFW_TARGET "")
    if(TARGET CONAN_PKG::glfw)
        set(ULTRASOUND_GLFW_TARGET CONAN_PKG::glfw)
    elseif(TARGET glfw::glfw)
        set(ULTRASOUND_GLFW_TARGET glfw::glfw)
    elseif(TARGET glfw)
        set(ULTRASOUND_GLFW_TARGET glfw)
    endif()

    if (ULTRASOUND_GLFW_TARGET STREQUAL "")
        if (TARGET glfw)
            set(ULTRASOUND_GLFW_TARGET glfw)
        endif()
    endif()

    set(ULTRASOUND_GLEW_TARGET "")
    if(TARGET CONAN_PKG::glew)
        set(ULTRASOUND_GLEW_TARGET CONAN_PKG::glew)
    elseif(TARGET glew::glew)
        set(ULTRASOUND_GLEW_TARGET glew::glew)
    elseif(TARGET GLEW::GLEW)
        set(ULTRASOUND_GLEW_TARGET GLEW::GLEW)
    endif()

    set(ULTRASOUND_IMGUI_TARGET "")
    if(TARGET CONAN_PKG::imgui)
        set(ULTRASOUND_IMGUI_TARGET CONAN_PKG::imgui)
    elseif(TARGET imgui::imgui)
        set(ULTRASOUND_IMGUI_TARGET imgui::imgui)
    endif()

    if (ULTRASOUND_GLFW_TARGET STREQUAL "" OR ULTRASOUND_IMGUI_TARGET STREQUAL "")
        message(FATAL_ERROR "Unable to resolve visualizer dependencies. Install dependencies via Conan (glfw, glew, imgui, opengl).")
    endif()

    set(ULTRASOUND_VISUALIZER_SOURCES
        src/visualization/imgui_visualizer.cpp
        bindings/imgui_impl_glfw.cpp
        bindings/imgui_impl_opengl3.cpp
    )
    set(ULTRASOUND_VISUALIZER_INCLUDE_DIRS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        ${CMAKE_CURRENT_SOURCE_DIR}/bindings
    )

    add_library(ultrasound_visualizer ${ULTRASOUND_VISUALIZER_SOURCES})

    target_include_directories(ultrasound_visualizer
        PUBLIC
            ${ULTRASOUND_VISUALIZER_INCLUDE_DIRS}
    )

    target_compile_features(ultrasound_visualizer PUBLIC cxx_std_20)
    target_link_libraries(ultrasound_visualizer
        PUBLIC
            ultrasound_core
        PRIVATE
            ${ULTRASOUND_GLFW_TARGET}
            ${ULTRASOUND_IMGUI_TARGET}
            OpenGL::GL
    )
    if (NOT ULTRASOUND_GLEW_TARGET STREQUAL "")
        target_link_libraries(ultrasound_visualizer PRIVATE ${ULTRASOUND_GLEW_TARGET})
    endif()

    add_executable(uss_imgui_visualizer
        apps/imgui_visualizer.cpp
    )
    target_link_libraries(uss_imgui_visualizer PRIVATE ultrasound_core ultrasound_io ultrasound_visualizer)

    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/visualization/imgui.ini DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

if (ULTRASOUND_BUILD_TESTS)
    find_package(GTest REQUIRED)
    include(GoogleTest)

    add_executable(ultrasound_tests
        tests/test_processor.cpp
        tests/test_config_loader.cpp
        tests/test_replay_source.cpp
        tests/test_runtime_stub.cpp
    )
    target_link_libraries(ultrasound_tests
        PRIVATE
            ultrasound_core
            ultrasound_io
            GTest::gtest
            GTest::gtest_main
    )
    gtest_discover_tests(ultrasound_tests)

    if (ULTRASOUND_ENABLE_COVERAGE)
        if (MSVC)
            find_program(ULTRASOUND_OPENCOVERAGE_EXECUTABLE
                NAMES OpenCppCoverage
                PATHS "C:/Program Files/OpenCppCoverage"
                PATH_SUFFIXES ""
            )
            if (NOT ULTRASOUND_OPENCOVERAGE_EXECUTABLE)
                message(FATAL_ERROR "ULTRASOUND_ENABLE_COVERAGE=ON but OpenCppCoverage was not found.")
            endif()

            find_package(Python3 REQUIRED COMPONENTS Interpreter)
            set(ULTRASOUND_COVERAGE_XML "${CMAKE_BINARY_DIR}/coverage/coverage.xml")
            file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/src/core" ULTRASOUND_COVERAGE_SRC_CORE_NATIVE)
            file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}/src/io" ULTRASOUND_COVERAGE_SRC_IO_NATIVE)
            file(TO_NATIVE_PATH "${ULTRASOUND_COVERAGE_XML}" ULTRASOUND_COVERAGE_XML_NATIVE)

            add_custom_target(coverage
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/coverage"
                COMMAND "${ULTRASOUND_OPENCOVERAGE_EXECUTABLE}"
                        --quiet
                        --sources "${ULTRASOUND_COVERAGE_SRC_CORE_NATIVE}"
                        --sources "${ULTRASOUND_COVERAGE_SRC_IO_NATIVE}"
                        --export_type "cobertura:${ULTRASOUND_COVERAGE_XML_NATIVE}"
                        -- "$<TARGET_FILE:ultrasound_tests>" --gtest_color=no
                COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/check_coverage.py" "${ULTRASOUND_COVERAGE_XML}" "80.0"
                DEPENDS ultrasound_tests
                USES_TERMINAL
                COMMENT "Running unit tests with OpenCppCoverage and enforcing >=80% line coverage"
            )
        else()
            message(FATAL_ERROR "ULTRASOUND_ENABLE_COVERAGE=ON is currently supported only on MSVC.")
        endif()
    endif()
endif()
