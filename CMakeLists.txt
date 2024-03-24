cmake_minimum_required(VERSION 3.5)

project(artifacts-deploy-v2 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets LinguistTools Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools Network)

set(TS_FILES artifacts-deploy-v2_ru_RU.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.hpp
        mainwindow.ui
        api_handler.hpp api_handler.cpp
        ssh_connection.hpp ssh_connection.cpp
        cooler_list.hpp cooler_list.cpp
        token_dialog.hpp token_dialog.cpp token_dialog.ui
        ssh_dialog.hpp ssh_dialog.cpp ssh_dialog.ui
        deploy_dialog.hpp deploy_dialog.cpp deploy_dialog.ui
        data_manager.hpp data_manager.cpp
        cooler_dialog.hpp cooler_dialog.cpp
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(artifacts-deploy-v2
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        download_manager.hpp download_manager.cpp
        deploy_object.hpp deploy_object.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET artifacts-deploy-v2 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(artifacts-deploy-v2 SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(artifacts-deploy-v2 ${PROJECT_SOURCES})
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

find_package(libssh REQUIRED)
if (NOT TARGET ssh)
     add_library(ssh SHARED IMPORTED)
     set_target_properties(ssh
                           PROPERTIES
                 ​          IMPORTED_LOCATION "${LIBSSH_LIBRARIES}"
                           INTERFACE_INCLUDE_DIRECTORIES "${LIBSSH_INCLUDE_DIR}")
endif()

target_link_libraries(artifacts-deploy-v2 PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network ssh)



# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.artifacts-deploy-v2)
endif()
set_target_properties(artifacts-deploy-v2 PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS artifacts-deploy-v2
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(artifacts-deploy-v2)
endif()
