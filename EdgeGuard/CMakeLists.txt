cmake_minimum_required(VERSION 3.16)

project(EdgeGuard VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Quick
    SerialPort
    Charts
    Sql
)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appEdgeGuard
    main.cpp
    assets/assets.qrc
)

set_source_files_properties(theme/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(appEdgeGuard
    URI EdgeGuard
    VERSION 1.0

    QML_FILES
        Main.qml
        SetupPage.qml
        Dashboard.qml
        HistoryPage.qml
        components/HistoryChart.qml
        components/DashboardHeaderBar.qml
        components/StatusBadge.qml
        components/ControlButton.qml
        components/HistoryChartPanel.qml
        components/PanelCard.qml
        components/ChartCard.qml
        components/AxisSelectorCombo.qml
        components/StatusTile.qml
        components/ValueCard.qml
        components/GaugeCard.qml
        components/MetricsOverviewPanel.qml
        components/AnomalyStatusPanel.qml
        components/LiveChartsPanel.qml
        components/LiveTrendChart.qml
        theme/Theme.qml

    RESOURCES
        utils/ChartUtils.js
        utils/HistoryChartUtils.js
        assets/motor.jpg
        assets/pump.jpeg
        assets/gearbox.jpeg
        assets/compresor.jpeg
        assets/grid.jpeg
        assets/other.jpeg

    SOURCES
        backend/models/sensor_sample.h
        backend/services/serial_service.h
        backend/services/serial_service.cpp
        backend/services/data_storage_service.h
        backend/services/data_storage_service.cpp
        backend/controllers/app_controller.h
        backend/controllers/app_controller.cpp
)

target_link_libraries(appEdgeGuard
    PRIVATE
        Qt6::Quick
        Qt6::SerialPort
        Qt6::Charts
        Qt6::Sql
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
