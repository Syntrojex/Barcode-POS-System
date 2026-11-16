cmake_minimum_required(VERSION 3.21)
project(BarcodeHub VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)   # Q_OBJECT classes (pages, widgets, MainWindow)

# ---------------------------------------------------------------------------
# FRONTEND-ONLY BUILD
# Only Qt Widgets is required. No OpenCV, no Backend/ sources.
# Every screen runs on in-memory sample data (Frontend/src/ui/AppState.cpp).
# ---------------------------------------------------------------------------
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

file(GLOB_RECURSE FRONTEND_SOURCES CONFIGURE_DEPENDS
    Frontend/src/ui/*.cpp Frontend/include/ui/*.h)
# Old OpenCV-based files (CameraWidget, POSWindow, camera-test main) must never be built here,
# even if they are still lying around from an older copy of the project.
list(FILTER FRONTEND_SOURCES EXCLUDE REGEX "CameraWidget|POSWindow|[/\\\\]main\\.cpp$")
add_library(barcode_ui STATIC ${FRONTEND_SOURCES})
if(MSVC)
    target_compile_options(barcode_ui PUBLIC /utf-8 /Zc:__cplusplus /permissive-)   # sources are UTF-8
    target_compile_definitions(barcode_ui PUBLIC _USE_MATH_DEFINES NOMINMAX)
endif()
target_include_directories(barcode_ui PUBLIC Frontend/include/ui)
target_link_libraries(barcode_ui PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)

add_executable(BarcodeHub WIN32 Frontend/src/main.cpp)
target_link_libraries(BarcodeHub PRIVATE barcode_ui)

# Run from Visual Studio / Qt Creator with the console-less exe as the startup project.
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT BarcodeHub)

# Copy the Qt DLLs next to the .exe so it runs by double-click (MinGW and MSVC), and
# run windeployqt when it is available so the platform plugin is found too.
if(WIN32)
    add_custom_command(TARGET BarcodeHub POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:BarcodeHub> $<TARGET_FILE_DIR:BarcodeHub>
        COMMAND_EXPAND_LISTS VERBATIM)
    find_program(WINDEPLOYQT NAMES windeployqt6 windeployqt
                 HINTS "${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin" "${Qt${QT_VERSION_MAJOR}Core_DIR}/../../../bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET BarcodeHub POST_BUILD
            COMMAND "${WINDEPLOYQT}" --no-translations --no-system-d3d-compiler --no-opengl-sw
                    $<TARGET_FILE:BarcodeHub> VERBATIM)
    endif()
endif()
