cmake_minimum_required(VERSION 3.21)

project(BarcodePOS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BACKEND_ONLY "Build backend and tests without Qt" OFF)

set(CAMERA ON CACHE STRING "OpenCV camera: OFF, ON, AUTO")
set_property(CACHE CAMERA PROPERTY STRINGS OFF ON AUTO)

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
# OPTIONAL OPENCV CAMERA SUPPORT
# ============================================================

set(CAMERA_ENABLED OFF)

if(NOT CAMERA STREQUAL "OFF")

    if(NOT OpenCV_DIR)
        foreach(_candidate
            "${CMAKE_SOURCE_DIR}/build/deps/opencv"
            "${CMAKE_SOURCE_DIR}/build/deps/opencv/x64/mingw/lib"
        )
            if(EXISTS "${_candidate}/OpenCVConfig.cmake")
                set(OpenCV_DIR "${_candidate}"
                    CACHE PATH "OpenCV installation" FORCE)
                break()
            endif()
        endforeach()
    endif()

    if(CAMERA STREQUAL "ON")
        find_package(OpenCV 4 REQUIRED COMPONENTS
            core imgproc imgcodecs videoio)
    else()
        find_package(OpenCV 4 QUIET COMPONENTS
            core imgproc imgcodecs videoio)
    endif()

    if(OpenCV_FOUND)
        set(CAMERA_ENABLED ON)
    endif()

endif()

if(CAMERA_ENABLED)

    # Find the OpenCV header folder ourselves (and print what was found), so the
    # compiler always gets it, whatever OpenCVConfig.cmake reports.
    find_path(OPENCV_HEADER_DIR opencv2/core.hpp
        HINTS ${OpenCV_INCLUDE_DIRS}
              "${OpenCV_DIR}/../../../include"
              "${OpenCV_DIR}/../../include"
              "${OpenCV_DIR}/../include"
              "${OpenCV_DIR}/include"
              "${CMAKE_SOURCE_DIR}/build/deps/opencv/include"
        PATH_SUFFIXES opencv4
        NO_DEFAULT_PATH)
    message(STATUS "OpenCV_DIR     = ${OpenCV_DIR}")
    message(STATUS "OpenCV headers = ${OPENCV_HEADER_DIR}")
    message(STATUS "OpenCV libs    = ${OpenCV_LIBS}")
    if(NOT OPENCV_HEADER_DIR)
        message(FATAL_ERROR
            "opencv2/core.hpp not found near ${OpenCV_DIR}. "
            "Run scripts/build-windows.ps1 to build OpenCV first.")
    endif()

    add_library(scanner_core STATIC
        Backend/src/scanner/cameraScanner.cpp
        Backend/src/scanner/decoder.cpp
    )

    target_include_directories(scanner_core PUBLIC
        Backend/include/scanner
        ${OPENCV_HEADER_DIR}
        ${OpenCV_INCLUDE_DIRS}
    )

    target_link_libraries(scanner_core PUBLIC
        ${OpenCV_LIBS}
    )

    if(WIN32)
        target_link_libraries(scanner_core PUBLIC
            ole32 oleaut32 strmiids)
    endif()

endif()

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

# ============================================================
# FRONTEND
# ============================================================

set(UI_SOURCES
    Frontend/src/ui/AppState.cpp
    Frontend/src/ui/DashboardPage.cpp
    Frontend/src/ui/Dialogs.cpp
    Frontend/src/ui/MainWindow.cpp
    Frontend/src/ui/NavigationModel.cpp
    Frontend/src/ui/OtherPages.cpp
    Frontend/src/ui/RecordOrderPage.cpp
    Frontend/src/ui/Theme.cpp
    Frontend/src/ui/Widgets.cpp
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

if(CAMERA_ENABLED)
    target_link_libraries(barcode_ui PUBLIC scanner_core)
    target_compile_definitions(barcode_ui PUBLIC WITH_CAMERA)
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

# Camera build on Windows: copy the OpenCV runtime DLLs next to BarcodeHub.exe,
# otherwise the exe cannot start outside a shell that has OpenCV on PATH.
if(WIN32 AND CAMERA_ENABLED)
    file(GLOB _opencv_dlls
        "${OpenCV_DIR}/../bin/*.dll"
        "${OpenCV_DIR}/../../bin/*.dll")
    if(_opencv_dlls)
        add_custom_command(TARGET BarcodeHub POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${_opencv_dlls} "$<TARGET_FILE_DIR:BarcodeHub>"
            VERBATIM
            COMMENT "Copying OpenCV runtime DLLs"
        )
    else()
        message(WARNING "No OpenCV DLLs found near ${OpenCV_DIR}; add OpenCV bin to PATH to run the exe.")
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
# OPTIONAL CAMERA DEMO
# ============================================================

if(CAMERA_ENABLED AND EXISTS
    "${CMAKE_SOURCE_DIR}/tests/camera_demo.cpp")

    add_executable(CameraDemo WIN32
        tests/camera_demo.cpp
        Frontend/src/ui/CameraWidget.cpp
        Frontend/include/ui/CameraWidget.h
    )

    target_include_directories(CameraDemo PRIVATE
        Frontend/include/ui
        Backend/include/scanner
    )

    target_link_libraries(CameraDemo PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        scanner_core
    )

endif()

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