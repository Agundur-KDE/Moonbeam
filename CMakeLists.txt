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
find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Qml Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.5)
find_package(KF6 REQUIRED COMPONENTS
    CoreAddons
    I18n
    Kirigami
    Config
    Notifications
    Wallet
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
    SOURCES
        src/sunshinecontroller.h
        src/sunshinecontroller.cpp
        src/sunshinecredentials.h
        src/sunshinecredentials.cpp
        src/sunshineidentity.h
        src/sunshineidentity.cpp
        src/sunshinecredentialsstate.h
        src/sunshinecredentialsstate.cpp
        src/sunshineremotecontrolconfig.h
        src/sunshineremotecontrolconfig.cpp
        src/sunshineportconfig.h
        src/sunshineportconfig.cpp
        src/isunshineprocess.h
        src/isunshineprocess.cpp
        src/isunshinenetworkprobe.h
        src/isunshinenetworkprobe.cpp
        src/isunshinewallet.h
        src/isunshinewallet.cpp
        src/isunshinecredentialsprocess.h
        src/isunshinecredentialsprocess.cpp
        src/sunshinepairingresponse.h
        src/sunshinepairingresponse.cpp
)

target_include_directories(moonbeam PRIVATE src)

target_link_libraries(moonbeam PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    KF6::CoreAddons
    KF6::I18n
    KF6::I18nQml
    KF6::Kirigami
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::Notifications
    KF6::Wallet
)

enable_testing()
find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(sunshineidentitytest
    tests/sunshineidentitytest.cpp
    src/sunshineidentity.cpp
)
target_include_directories(sunshineidentitytest PRIVATE src)
target_compile_definitions(sunshineidentitytest PRIVATE
    MOONBEAM_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
)
target_link_libraries(sunshineidentitytest PRIVATE Qt6::Test Qt6::Network)
add_test(NAME sunshineidentitytest COMMAND sunshineidentitytest)

add_executable(sunshinepinningtest
    tests/sunshinepinningtest.cpp
    src/sunshineidentity.cpp
)
target_include_directories(sunshinepinningtest PRIVATE src)
target_compile_definitions(sunshinepinningtest PRIVATE
    MOONBEAM_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
)
target_link_libraries(sunshinepinningtest PRIVATE Qt6::Test Qt6::Network)
add_test(NAME sunshinepinningtest COMMAND sunshinepinningtest)

add_executable(sunshinecredentialsstatetest
    tests/sunshinecredentialsstatetest.cpp
    src/sunshinecredentialsstate.cpp
)
target_include_directories(sunshinecredentialsstatetest PRIVATE src)
target_link_libraries(sunshinecredentialsstatetest PRIVATE Qt6::Test)
add_test(NAME sunshinecredentialsstatetest COMMAND sunshinecredentialsstatetest)

add_executable(sunshineremotecontrolconfigtest
    tests/sunshineremotecontrolconfigtest.cpp
    src/sunshineremotecontrolconfig.cpp
)
target_include_directories(sunshineremotecontrolconfigtest PRIVATE src)
target_link_libraries(sunshineremotecontrolconfigtest PRIVATE Qt6::Test)
add_test(NAME sunshineremotecontrolconfigtest COMMAND sunshineremotecontrolconfigtest)

add_executable(sunshineportconfigtest
    tests/sunshineportconfigtest.cpp
    src/sunshineportconfig.cpp
)
target_include_directories(sunshineportconfigtest PRIVATE src)
target_link_libraries(sunshineportconfigtest PRIVATE Qt6::Test)
add_test(NAME sunshineportconfigtest COMMAND sunshineportconfigtest)

add_executable(sunshinepairingresponsetest
    tests/sunshinepairingresponsetest.cpp
    src/sunshinepairingresponse.cpp
)
target_include_directories(sunshinepairingresponsetest PRIVATE src)
target_link_libraries(sunshinepairingresponsetest PRIVATE Qt6::Test)
add_test(NAME sunshinepairingresponsetest COMMAND sunshinepairingresponsetest)

add_executable(sunshinecontrollertest
    tests/sunshinecontrollertest.cpp
    tests/fakes/fakesunshineprocess.h
    src/sunshinecontroller.cpp
    src/sunshineidentity.cpp
    src/sunshineportconfig.cpp
    src/sunshineremotecontrolconfig.cpp
    src/sunshinepairingresponse.cpp
    src/isunshineprocess.cpp
    src/isunshinenetworkprobe.cpp
)
target_include_directories(sunshinecontrollertest PRIVATE src tests)
target_compile_definitions(sunshinecontrollertest PRIVATE
    MOONBEAM_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures"
)
target_link_libraries(sunshinecontrollertest PRIVATE Qt6::Test Qt6::Network Qt6::Qml)
add_test(NAME sunshinecontrollertest COMMAND sunshinecontrollertest)

add_executable(sunshinecredentialstest
    tests/sunshinecredentialstest.cpp
    src/sunshinecredentials.cpp
    src/sunshinecredentialsstate.cpp
    src/isunshinewallet.cpp
    src/isunshinecredentialsprocess.cpp
)
target_include_directories(sunshinecredentialstest PRIVATE src tests)
target_link_libraries(sunshinecredentialstest PRIVATE Qt6::Test Qt6::Qml KF6::Wallet)
add_test(NAME sunshinecredentialstest COMMAND sunshinecredentialstest)

install(TARGETS moonbeam ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.agundur.moonbeam.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES assets/icons/org.agundur.moonbeam.png
    DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/512x512/apps
)
