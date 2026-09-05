cmake_minimum_required(VERSION 3.21)
project(moonbeam VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(ECM REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

set(QT_DEFAULT_MAJOR_VERSION 6)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.5)
find_package(KF6 REQUIRED COMPONENTS
    CoreAddons
    I18n
    Kirigami
    Config
    Notifications
)

qt_add_executable(moonbeam
    src/main.cpp
)

qt_add_qml_module(moonbeam
    URI org.agundur.moonbeam
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/pages/StatusPage.qml
        qml/pages/PairingPage.qml
        qml/pages/MediaCastPage.qml
    SOURCES
        src/sunshinecontroller.h
        src/sunshinecontroller.cpp
    RESOURCES
        assets/icon.svg
)

target_include_directories(moonbeam PRIVATE src)

target_link_libraries(moonbeam PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    KF6::CoreAddons
    KF6::I18n
    KF6::Kirigami
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::Notifications
)

install(TARGETS moonbeam ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.agundur.moonbeam.desktop DESTINATION ${KDE_INSTALL_APPDIR})
