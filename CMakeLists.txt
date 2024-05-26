cmake_minimum_required(VERSION 3.5)

project(artifacts-deploy VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add an option to specify the Qt version
option(QT_VERSION_MAJOR "Specify the major version of Qt to use (5 or 6)" 5)

# Find the specified Qt version
if(QT_VERSION_MAJOR EQUAL 5)
    find_package(QT NAMES Qt5 REQUIRED COMPONENTS Widgets Network UiTools)
    find_package(Qt5 REQUIRED COMPONENTS Widgets Network UiTools)
elseif(QT_VERSION_MAJOR EQUAL 6)
    find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets Network UiTools)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Network UiTools)
else()
    message(FATAL_ERROR "Unsupported Qt version: ${QT_VERSION_MAJOR}. Please specify 5 or 6.")
endif()

if (NOT WIN32)
    message(STATUS "Using system libraries")
    find_package(libssh REQUIRED)
    find_package(libzip REQUIRED)
endif()

file(GLOB PROJECT_SOURCES src/*.cpp src/*.hpp src/ui/*.ui src/resources/*.qrc)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(artifacts-deploy MANUAL_FINALIZATION ${PROJECT_SOURCES})
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET artifacts-deploy APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

else()
    if(ANDROID)
        add_library(artifacts-deploy SHARED ${PROJECT_SOURCES})
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(artifacts-deploy ${PROJECT_SOURCES})
    endif()

endif()


# Conditionally link static libraries for Windows
if(WIN32)
    # Include directories for header files
    message(STATUS "Using Windows libraries")
    include_directories(win-libs/include)
    # Specify the directories containing the static libraries
    link_directories(win-libs/lib)
endif()

target_link_libraries(artifacts-deploy PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::UiTools
    ssh zip)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION_MAJOR} VERSION_LESS 6.1)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.artifacts-deploy)
endif()
set_target_properties(artifacts-deploy PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS artifacts-deploy
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(artifacts-deploy)
endif()
