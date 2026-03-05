cmake_minimum_required(VERSION 3.16)

project(EdgeGuard VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Quick
    SerialPort
    Charts
)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appEdgeGuard
    main.cpp
    backend/datamodel.cpp
)

set_source_files_properties(theme/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(appEdgeGuard
    URI EdgeGuard
    VERSION 1.0

    QML_FILES
        Main.qml
        components/AppHeader.qml
        components/Chip.qml
        components/ActionButton.qml
        components/MotorSelector.qml
        components/MetricCard.qml
        components/FeaturesCard.qml
        components/MotorHealthStatus.qml
        components/PersistenceBar.qml
        components/StateSegment.qml
        components/SegmentPill.qml
        components/RawSignals.qml
        components/SignalChart.qml
        theme/Theme.qml

    SOURCES
        backend/datamodel.h
        backend/datamodel.cpp
)

target_link_libraries(appEdgeGuard
    PRIVATE
        Qt6::Quick
        Qt6::SerialPort
        Qt6::Charts
)

set_target_properties(appEdgeGuard PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)

install(TARGETS appEdgeGuard
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
