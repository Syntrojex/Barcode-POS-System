cmake_minimum_required(VERSION 3.21)

# Windows + Ninja: Qt Creator sometimes hands CMake Visual Studio's cl.exe together with a MinGW
# Qt kit, which cannot compile. Unless an MSVC Qt is being used, switch to Qt's own MinGW g++.
if(WIN32 AND NOT CMAKE_GENERATOR MATCHES "Visual Studio"
   AND (NOT CMAKE_CXX_COMPILER OR CMAKE_CXX_COMPILER MATCHES "[cC][lL]\\.exe$")
   AND NOT "${CMAKE_PREFIX_PATH}${Qt6_DIR}${QT_DIR}" MATCHES "[mM][sS][vV][cC]")
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/toolchain-qt-mingw.cmake")
endif()

project(BarcodePOS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # Qt 6 needs a correct __cplusplus value; also read sources as UTF-8.
    add_compile_options(/Zc:__cplusplus /utf-8 /permissive-)
endif()

option(BACKEND_ONLY "Build backend and tests without Qt" OFF)

# Camera support uses Qt Multimedia (shipped with Qt; no OpenCV needed):
#   AUTO (default) - camera if the Qt Multimedia component is installed, otherwise built without it
#   ON             - camera required: configuring fails if Qt Multimedia is missing
#   OFF            - no camera (typed barcode lookup only)
set(CAMERA_MODE AUTO CACHE STRING "Camera: AUTO, ON, OFF")
set_property(CACHE CAMERA_MODE PROPERTY STRINGS AUTO ON OFF)
# Which library grabs the pictures (the barcode decoder itself is always the same pure C++ code):
#   AUTO   - OpenCV if a build matching the compiler is found, otherwise Qt Multimedia
#   QT     - Qt Multimedia only
#   OPENCV - OpenCV only (set OpenCV_DIR; it must be built for the SAME compiler as Qt:
#            the official OpenCV Windows download is MSVC-only -> use it with the MSVC kit)
set(CAMERA_BACKEND AUTO CACHE STRING "Camera library: AUTO, QT, OPENCV")
set_property(CACHE CAMERA_BACKEND PROPERTY STRINGS AUTO QT OPENCV)

# ============================================================
# BACKEND LIBRARIES
# ============================================================

# Product management
add_library(product_core STATIC
    Backend/src/product/ProductModel.cpp
    Backend/src/product/ProductStorage.cpp
    Backend/src/product/ProductValidation.cpp
)

target_include_directories(product_core PUBLIC
    Backend/include
    Backend/include/product
)

# Order management
add_library(order_core STATIC
    Backend/src/order/Order.cpp
    Backend/src/order/OrderDetail.cpp
    Backend/src/order/OrderManager.cpp
)

target_include_directories(order_core PUBLIC
    Backend/include
)

# Common utilities
add_library(common_core STATIC
    Backend/src/common/RecordText.cpp
)

target_include_directories(common_core PUBLIC
    Backend/include
)

# Billing
add_library(billing_core STATIC
    Backend/src/billing/BillCalculator.cpp
    Backend/src/billing/BillingService.cpp
    Backend/src/billing/BillRecordCodec.cpp
    Backend/src/billing/BillRepository.cpp
    Backend/src/billing/PaymentMethod.cpp
    Backend/src/billing/SeasonRepository.cpp
)

target_include_directories(billing_core PUBLIC
    Backend/include
)

target_link_libraries(billing_core PUBLIC
    order_core
    common_core
)

# Backend integration
add_library(integration_core STATIC
    Backend/src/integration/ProductAdapter.cpp
)

target_include_directories(integration_core PUBLIC
    Backend/include
)

target_link_libraries(integration_core PUBLIC
    product_core
    order_core
)

# Combined backend
add_library(backend_core INTERFACE)

target_link_libraries(backend_core INTERFACE
    integration_core
    billing_core
)

# ============================================================
# BARCODE DECODER (pure C++17: no OpenCV, no Qt)
# ============================================================

add_library(scanner_core STATIC
    Backend/src/scanner/decoder.cpp
)

target_include_directories(scanner_core PUBLIC
    Backend/include/scanner
)

# ============================================================
# BACKEND TESTS
# ============================================================

include(CTest)

if(BUILD_TESTING)

    add_executable(product_tests
        tests/product_tests.cpp
    )

    target_link_libraries(product_tests PRIVATE product_core)

    add_test(NAME product COMMAND product_tests)

    set_tests_properties(product PROPERTIES
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )

    add_executable(scanner_tests
        tests/scanner_tests.cpp
    )

    target_link_libraries(scanner_tests PRIVATE scanner_core)

    add_test(NAME scanner COMMAND scanner_tests)

    add_executable(order_tests
        tests/OrderManagementSmokeTest.cpp
    )

    target_link_libraries(order_tests PRIVATE order_core)

    add_test(NAME order COMMAND order_tests)

    add_executable(integration_test
        tests/integration_test.cpp
    )

    target_link_libraries(integration_test PRIVATE backend_core)

    add_test(NAME integration COMMAND integration_test)

    set_tests_properties(integration PROPERTIES
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )

    add_executable(checkout_test
        tests/checkout_test.cpp
    )

    target_link_libraries(checkout_test PRIVATE backend_core)

    add_test(NAME checkout COMMAND checkout_test)

    # Editing a paid order in place / on-hold orders kept in Order.txt (Increment 3, requirements 3 and 10)
    add_executable(order_edit_test
        tests/order_edit_test.cpp
    )

    target_link_libraries(order_edit_test PRIVATE backend_core)

    add_test(NAME order_edit COMMAND order_edit_test)

    # Seasons.txt reader/writer
    add_executable(season_test
        tests/season_test.cpp
    )

    target_link_libraries(season_test PRIVATE billing_core)

    add_test(NAME season COMMAND season_test)

    add_executable(billing_tests
        tests/billing_tests.cpp
    )

    target_link_libraries(billing_tests PRIVATE billing_core)

    add_test(NAME billing COMMAND billing_tests)

    set_tests_properties(billing PROPERTIES
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )

endif()

if(BACKEND_ONLY)
    message(STATUS "BACKEND_ONLY enabled: Qt frontend skipped")
    return()
endif()

# ============================================================
# QT CONFIGURATION
# ============================================================

set(QT_INSTALL_ROOT "C:/Qt"
    CACHE PATH "Qt installation root"
)

if(WIN32 AND NOT CMAKE_PREFIX_PATH AND NOT Qt6_DIR)

    file(GLOB _qt_roots
        "${QT_INSTALL_ROOT}/6.*/mingw_64"
    )

    if(_qt_roots)
        list(SORT _qt_roots
            COMPARE NATURAL
            ORDER DESCENDING
        )

        list(GET _qt_roots 0 _qt_root)
        list(PREPEND CMAKE_PREFIX_PATH "${_qt_root}")
    endif()

endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5
    REQUIRED COMPONENTS Widgets
)

find_package(Qt${QT_VERSION_MAJOR}
    REQUIRED COMPONENTS Widgets
)

# A MinGW build of Qt cannot be compiled with MSVC (this is what "Qt requires a C++17 compiler ...
# /Zc:__cplusplus" means: the kit picked cl.exe). Stop early with the fix instead of 100 errors.
if(MSVC AND "${Qt${QT_VERSION_MAJOR}_DIR}" MATCHES "mingw")
    message(FATAL_ERROR
        "Qt is a MinGW build (${Qt${QT_VERSION_MAJOR}_DIR}) but the compiler is MSVC (${CMAKE_CXX_COMPILER}).\n"
        "Fix: Qt Creator > Projects > Build & Run > use the kit 'Desktop Qt 6.x.x MinGW 64-bit' "
        "(its compiler must be 'MinGW ... 64-bit', not 'Microsoft Visual C++'), delete the build folder, "
        "and configure again. Or use the 'Desktop Qt 6.x.x MSVC2022 64-bit' kit instead.")
endif()

set(CAMERA_ENABLED OFF)
set(CAMERA_LIB "")
if(NOT CAMERA_MODE STREQUAL "OFF")
    if(NOT CAMERA_BACKEND STREQUAL "QT")
        # MinGW: first look for an OpenCV config built for MinGW (x64/mingw/lib), then for loose
        # headers + libopencv_*.dll files. (The official Windows download is MSVC-only.)
        if(MINGW)
            file(GLOB_RECURSE _mingw_cfg "$ENV{USERPROFILE}/Downloads/opencv/*mingw*/OpenCVConfig.cmake"
                                         "${OpenCV_DIR}/*mingw*/OpenCVConfig.cmake" "C:/opencv/*mingw*/OpenCVConfig.cmake")
            if(_mingw_cfg AND NOT OpenCV_DIR)
                list(GET _mingw_cfg 0 _first_cfg)
                get_filename_component(_cfg_dir "${_first_cfg}" DIRECTORY)
                set(OpenCV_DIR "${_cfg_dir}" CACHE PATH "OpenCV (MinGW)" FORCE)
            endif()
        endif()
        find_package(OpenCV 4 QUIET COMPONENTS core imgproc videoio)
        if(OpenCV_FOUND)
            set(CAMERA_ENABLED ON)
            set(CAMERA_LIB OPENCV)
            message(STATUS "Camera: OpenCV ${OpenCV_VERSION} (${OpenCV_DIR})")
        elseif(MINGW)
            list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
            include(FindMinGWOpenCV)
            if(MINGW_OPENCV_FOUND)
                set(OpenCV_INCLUDE_DIRS "${MINGW_OPENCV_INCLUDE}")
                set(OpenCV_LIBS ${MINGW_OPENCV_LIBS})
                set(OPENCV_DLL_DIR "${MINGW_OPENCV_DLL_DIR}")
                set(CAMERA_ENABLED ON)
                set(CAMERA_LIB OPENCV)
                message(STATUS "Camera: OpenCV (MinGW files found without a CMake config)")
            endif()
        endif()
    endif()
    if(NOT CAMERA_ENABLED AND NOT CAMERA_BACKEND STREQUAL "OPENCV" AND QT_VERSION_MAJOR EQUAL 6)
        find_package(Qt6 QUIET COMPONENTS Multimedia)
        if(TARGET Qt6::Multimedia)
            set(CAMERA_ENABLED ON)
            set(CAMERA_LIB QT)
            message(STATUS "Camera: Qt Multimedia ${Qt6Multimedia_VERSION}")
        endif()
    endif()
    if(NOT CAMERA_ENABLED)
        set(_why "No usable camera library: install Qt Multimedia (Qt Maintenance Tool > Add or remove components > "
                 "Additional Libraries > Qt Multimedia), or give OpenCV_DIR an OpenCV built for THIS compiler "
                 "(the official OpenCV Windows download only works with the MSVC kit, not MinGW).")
        if(CAMERA_MODE STREQUAL "ON")
            message(FATAL_ERROR ${_why})
        else()
            message(STATUS "Camera disabled. ${_why}")
        endif()
    endif()
endif()

# Optional low-latency confirmation beep for cashier barcode scans:
# prefers Qt Multimedia (QSoundEffect) when installed, otherwise falls back to standard Windows winmm.
set(SCAN_AUDIO_BACKEND "NONE")
if(QT_VERSION_MAJOR EQUAL 6)
    if(NOT TARGET Qt6::Multimedia)
        find_package(Qt6 QUIET COMPONENTS Multimedia)
    endif()
    if(TARGET Qt6::Multimedia)
        set(SCAN_AUDIO_BACKEND "QT_MULTIMEDIA")
    endif()
endif()
if(SCAN_AUDIO_BACKEND STREQUAL "NONE" AND WIN32)
    set(SCAN_AUDIO_BACKEND "WINMM")
endif()
message(STATUS "Scan audio: ${SCAN_AUDIO_BACKEND}")

# ============================================================
# FRONTEND
# ============================================================

# Every page / dialog lives in its own file under Frontend/src/ui. New files are picked up automatically
# (re-run CMake). main.cpp, CameraWidget.cpp and POSWindow.cpp there are legacy stand-alone files.
file(GLOB UI_SOURCES CONFIGURE_DEPENDS
    Frontend/src/ui/*.cpp
)

# Legacy stand-alone files are skipped. [Mm]ain also matches the old Main.cpp test window,
# which includes CameraWidget.h (needs OpenCV) and has its own main().
list(FILTER UI_SOURCES EXCLUDE REGEX
    "/([Mm]ain|CameraWidget|POSWindow)\\.cpp$"
)

file(GLOB UI_HEADERS CONFIGURE_DEPENDS
    Frontend/include/ui/*.h
)

list(FILTER UI_HEADERS EXCLUDE REGEX
    "CameraWidget.h|POSWindow.h"
)

add_library(barcode_ui STATIC
    ${UI_SOURCES}
    ${UI_HEADERS}
    Frontend/resources/scan_audio.qrc
)

target_include_directories(barcode_ui PUBLIC
    Frontend/include/ui
    Frontend/include
    Backend/include
    Backend/include/product
)

target_link_libraries(barcode_ui PUBLIC
    Qt${QT_VERSION_MAJOR}::Widgets
    backend_core
)

target_compile_definitions(barcode_ui PRIVATE
    BARCODEHUB_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(barcode_ui PUBLIC scanner_core)
if(CAMERA_ENABLED)
    target_compile_definitions(barcode_ui PUBLIC WITH_CAMERA)
    if(CAMERA_LIB STREQUAL "QT")
        target_link_libraries(barcode_ui PUBLIC Qt6::Multimedia)
        target_compile_definitions(barcode_ui PUBLIC WITH_CAMERA_QT)
    else()
        target_include_directories(barcode_ui PUBLIC ${OpenCV_INCLUDE_DIRS})
        target_link_libraries(barcode_ui PUBLIC ${OpenCV_LIBS})
        target_compile_definitions(barcode_ui PUBLIC WITH_CAMERA_OPENCV)
    endif()
endif()

if(SCAN_AUDIO_BACKEND STREQUAL "QT_MULTIMEDIA")
    if(NOT (CAMERA_ENABLED AND CAMERA_LIB STREQUAL "QT"))
        target_link_libraries(barcode_ui PUBLIC Qt6::Multimedia)
    endif()
    target_compile_definitions(barcode_ui PUBLIC WITH_SCAN_AUDIO_QT)
elseif(SCAN_AUDIO_BACKEND STREQUAL "WINMM")
    target_link_libraries(barcode_ui PRIVATE winmm)
    target_compile_definitions(barcode_ui PUBLIC WITH_SCAN_AUDIO_WINMM)
endif()

# ============================================================
# MAIN EXECUTABLE
# ============================================================

add_executable(BarcodeHub WIN32
    Frontend/src/main.cpp
)

target_link_libraries(BarcodeHub PRIVATE
    barcode_ui
)

# OpenCV camera build on Windows: copy the OpenCV runtime DLLs next to BarcodeHub.exe.
if(WIN32 AND CAMERA_LIB STREQUAL "OPENCV")
    file(GLOB _opencv_dlls
        "${OpenCV_DIR}/x64/vc16/bin/*.dll" "${OpenCV_DIR}/x64/vc17/bin/*.dll"
        "${OpenCV_DIR}/../bin/*.dll" "${OpenCV_DIR}/../../bin/*.dll"
        "${OPENCV_DLL_DIR}/*.dll" "${OPENCV_DLL_DIR}/../bin/*.dll")
    if(_opencv_dlls)
        add_custom_command(TARGET BarcodeHub POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_opencv_dlls} "$<TARGET_FILE_DIR:BarcodeHub>"
            VERBATIM COMMENT "Copying OpenCV runtime DLLs")
    else()
        message(WARNING "No OpenCV DLLs found near ${OpenCV_DIR}; add the OpenCV bin folder to PATH to run the exe.")
    endif()
endif()

# Preserve existing records during rebuilding.
# Only copy starter data files when they do not already exist.

add_custom_command(TARGET BarcodeHub POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        "-DSOURCE_DATA=${CMAKE_SOURCE_DIR}/data"
        "-DDEST_DATA=$<TARGET_FILE_DIR:BarcodeHub>/data"
        -P "${CMAKE_SOURCE_DIR}/cmake/seed-data.cmake"
    VERBATIM
)

# ============================================================
# WINDOWS DEPLOYMENT (OPTIONAL)
# ============================================================

# Deployment is not part of the normal build. Qt Creator sets up the
# Qt/MinGW environment when launching an application using its Qt kit.
# To copy Qt DLLs and plugins for standalone distribution, build the
# BarcodeHub_deploy target explicitly instead.

if(WIN32)
    # Prefer the deployment tool belonging to the Qt version used by this kit.
    if(TARGET Qt${QT_VERSION_MAJOR}::windeployqt)
        get_target_property(WINDEPLOYQT_EXECUTABLE
            Qt${QT_VERSION_MAJOR}::windeployqt IMPORTED_LOCATION)
    endif()

    if(NOT WINDEPLOYQT_EXECUTABLE)
        find_program(WINDEPLOYQT_EXECUTABLE
            NAMES windeployqt
            HINTS
                "${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin"
                "${QT_INSTALL_ROOT}/6.11.0/mingw_64/bin"
        )
    endif()

    if(WINDEPLOYQT_EXECUTABLE)
        set(_deploy_apps BarcodeHub)
        if(TARGET CameraDemo)
            list(APPEND _deploy_apps CameraDemo)
        endif()

        foreach(app IN LISTS _deploy_apps)
            add_custom_target(${app}_deploy
                COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                    --no-translations
                    "$<TARGET_FILE:${app}>"
                DEPENDS ${app}
                VERBATIM
                COMMENT "Deploying Qt runtime for ${app}"
            )
        endforeach()
    else()
        message(STATUS
            "windeployqt not found; optional deployment targets disabled")
    endif()
endif()

if(BUILD_TESTING AND QT_VERSION_MAJOR EQUAL 6)
    find_package(Qt6 QUIET COMPONENTS Test)
    if(TARGET Qt6::Test)
        add_executable(scan_audio_tests
            tests/scan_audio_tests.cpp
        )
        target_link_libraries(scan_audio_tests PRIVATE
            barcode_ui
            Qt6::Test
        )
        add_test(NAME scan_audio COMMAND scan_audio_tests)
        if(WIN32)
            get_target_property(_qt_core_loc Qt6::Core IMPORTED_LOCATION_DEBUG)
            if(NOT _qt_core_loc)
                get_target_property(_qt_core_loc Qt6::Core IMPORTED_LOCATION_RELEASE)
            endif()
            if(NOT _qt_core_loc)
                get_target_property(_qt_core_loc Qt6::Core IMPORTED_LOCATION)
            endif()
            if(_qt_core_loc)
                get_filename_component(_qt_bin_dir "${_qt_core_loc}" DIRECTORY)
                get_filename_component(_qt_root_dir "${_qt_bin_dir}" DIRECTORY)
                if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.22")
                    set_tests_properties(scan_audio PROPERTIES
                        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qt_bin_dir};QT_QPA_PLATFORM=set:offscreen;QT_PLUGIN_PATH=set:${_qt_root_dir}/plugins"
                    )
                else()
                    set_tests_properties(scan_audio PROPERTIES
                        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=${_qt_root_dir}/plugins"
                    )
                endif()
            endif()
        else()
            set_tests_properties(scan_audio PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
            )
        endif()
    endif()
endif()