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
# Pure C++17 product module (no Qt): parsing, validation, search, stock rules, description.
file(GLOB PRODUCT_SOURCES CONFIGURE_DEPENDS Backend/src/product/*.cpp)
add_library(barcode_product STATIC ${PRODUCT_SOURCES})
target_include_directories(barcode_product PUBLIC Backend/include)
if(MSVC)
    target_compile_options(barcode_product PUBLIC /utf-8 /permissive-)
endif()

add_library(barcode_ui STATIC ${FRONTEND_SOURCES})
target_compile_definitions(barcode_ui PUBLIC BARCODEHUB_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(barcode_ui PUBLIC barcode_product)
if(MSVC)
    target_compile_options(barcode_ui PUBLIC /utf-8 /Zc:__cplusplus /permissive-)   # sources are UTF-8
    target_compile_definitions(barcode_ui PUBLIC _USE_MATH_DEFINES NOMINMAX)
endif()
target_include_directories(barcode_ui PUBLIC Frontend/include/ui)
target_link_libraries(barcode_ui PUBLIC Qt${QT_VERSION_MAJOR}::Widgets)

enable_testing()
add_executable(product_tests tests/product_tests.cpp)
target_link_libraries(product_tests PRIVATE barcode_product)
add_test(NAME product_tests COMMAND product_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(BarcodeHub WIN32 Frontend/src/main.cpp)
target_link_libraries(BarcodeHub PRIVATE barcode_ui)

# Run from Visual Studio / Qt Creator with the console-less exe as the startup project.
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT BarcodeHub)

# Copy the Qt DLLs next to the .exe so it runs by double-click (MinGW and MSVC), and
# run windeployqt when it is available so the platform plugin is found too.
if(WIN32)
<<<<<<< HEAD
    set(_apps BarcodeHub)
    if(TARGET CameraDemo)
        list(APPEND _apps CameraDemo)
    endif()

    # windeployqt copies the Qt DLLs *and* the plugins (platforms/qwindows.dll ...) next to the exe.
    # Without the plugins the exe says: Could not find the Qt platform plugin "windows".
    if(TARGET Qt${QT_VERSION_MAJOR}::windeployqt)
        get_target_property(WINDEPLOYQT_EXECUTABLE Qt${QT_VERSION_MAJOR}::windeployqt IMPORTED_LOCATION)
    else()
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt
            HINTS "${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin")
    endif()
    if(NOT WINDEPLOYQT_EXECUTABLE)
        message(WARNING "windeployqt not found: BarcodeHub.exe will not start from Explorer/Qt Creator")
    endif()
    get_filename_component(_cxx_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)   # MinGW runtime DLLs live here

    foreach(app ${_apps})
        add_custom_command(TARGET ${app} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${app}> $<TARGET_FILE_DIR:${app}>
            COMMAND_EXPAND_LISTS VERBATIM)
        if(WITH_CAMERA AND EXISTS ${OpenCV_DIR}/../bin)
            add_custom_command(TARGET ${app} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${OpenCV_DIR}/../bin $<TARGET_FILE_DIR:${app}>)
        endif()
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET ${app} POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} --force --no-translations $<TARGET_FILE:${app}>)   # --force: replace stale DLLs of another Qt version
        endif()
        foreach(_dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)   # MinGW runtime
            if(EXISTS "${_cxx_bin}/${_dll}")
                add_custom_command(TARGET ${app} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_cxx_bin}/${_dll}" $<TARGET_FILE_DIR:${app}>)
            endif()
        endforeach()
    endforeach()

    if(EXISTS ${CMAKE_SOURCE_DIR}/data)   # Product.txt, Order.txt ... next to the exe
=======
    add_custom_command(TARGET BarcodeHub POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:BarcodeHub> $<TARGET_FILE_DIR:BarcodeHub>
        COMMAND_EXPAND_LISTS VERBATIM)
    find_program(WINDEPLOYQT NAMES windeployqt6 windeployqt
                 HINTS "${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin" "${Qt${QT_VERSION_MAJOR}Core_DIR}/../../../bin")
    if(WINDEPLOYQT)
>>>>>>> 77c6df988a2022a8476003d52ad1d0547591e8c9
        add_custom_command(TARGET BarcodeHub POST_BUILD
            COMMAND "${WINDEPLOYQT}" --no-translations --no-system-d3d-compiler --no-opengl-sw
                    $<TARGET_FILE:BarcodeHub> VERBATIM)
    endif()
endif()
