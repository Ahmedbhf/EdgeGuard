# EdgeGuard Technical Documentation

## 1. Introduction

### 1.1 Project Overview

EdgeGuard is an edge predictive maintenance monitoring system.

Its goal is to:

- receive live vibration and temperature data from an embedded device
- display machine condition in real time
- show anomaly level clearly to the operator
- keep a short rolling history for analysis and export

### 1.2 Main Purpose

The system is designed for industrial monitoring and anomaly detection.

Typical use cases:

- monitoring a motor, pump, gearbox, or compressor
- visualizing anomaly score in real time
- tracking vibration on X, Y, and Z axes
- storing the last 24 hours of useful data for review

---

## 2. System Architecture

### 2.1 Global Architecture

The system is composed of two main parts:

- STM32 side:
  data acquisition, preprocessing, and score calculation
- Qt desktop application:
  monitoring, visualization, and history storage

### 2.2 Communication Flow

```text
STM32 → UART → Qt App → UI + Storage
```

### 2.3 MVC-Inspired Organization

The project follows a simple MVC-inspired structure:

- Models:
  represent data only
- Services:
  perform logic such as UART communication and file storage
- UI:
  display pages and widgets
- AppController:
  connects services to the UI

### 2.4 Folder Logic

Important backend folders:

- `backend/models`
- `backend/services`
- `backend/controllers`

Important UI folders:

- project root QML pages
- `components`

---

## 3. Data Model

### 3.1 SensorSample

The main data model is `SensorSample`.

File:

- [sensor_sample.h](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/models/sensor_sample.h)

### 3.2 Fields

`SensorSample` contains:

- `timestampUtc`
- `anomalyScore`
- `x`
- `y`
- `z`
- `temp`
- `ambientTemp`
- `deviceId`
- `state`

### 3.3 Role

This structure is the common data unit used between:

- `SerialService`
- `AppController`
- `DataStorageService`

### 3.4 Example Data Row

Example CSV row:

```text
2026-04-09T14:32:10.000Z,80,0.52,0.48,0.50,42.1
```

Meaning:

- timestamp = `2026-04-09T14:32:10.000Z`
- anomaly score = `80`
- X axis = `0.52`
- Y axis = `0.48`
- Z axis = `0.50`
- machine temperature = `42.1`

### 3.5 Example RMS Calculation

`SensorSample` also provides RMS computation:

```cpp
double rms() const
{
    const double energy = ((x * x) + (y * y) + (z * z)) / 3.0;
    return std::sqrt(energy);
}
```

---

## 4. Services (Core Logic)

### 4.1 SerialService

File:

- [serial_service.h](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/services/serial_service.h)
- [serial_service.cpp](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/services/serial_service.cpp)

#### Role

`SerialService` is responsible for UART communication.

It:

- scans available serial ports
- opens and closes the UART connection
- reads incoming bytes
- reconstructs complete text lines
- converts a valid line into `SensorSample`

#### Main Functions

```cpp
refreshPorts()
connectToPort(const QString &portName)
disconnectPort()
onReadyRead()
processLine(const QByteArray &line)
```

Meaning:

- `refreshPorts()`:
  updates the available port list
- `connectToPort(...)`:
  opens the selected serial port
- `disconnectPort()`:
  closes the serial connection
- `onReadyRead()`:
  reads bytes from UART and detects full lines
- `processLine(...)`:
  parses one device message

#### Note About Naming

In some documentation, these functions may be described as:

- `openPort()`
- `closePort()`
- `parseLine()`

In the current implementation, the real names are:

- `connectToPort()`
- `disconnectPort()`
- `processLine()`

#### Example Input

Expected UART frame:

```text
UID123,82,0.51,0.49,0.50,42.1,29.7
```

Parsed meaning:

- field 0: device ID
- field 1: anomaly score
- field 2: X
- field 3: Y
- field 4: Z
- field 5: machine temperature
- field 6: ambient temperature

#### Example Output

Conceptually, the service produces:

```cpp
SensorSample sample;
sample.deviceId = "UID123";
sample.anomalyScore = 82;
sample.x = 0.51;
sample.y = 0.49;
sample.z = 0.50;
sample.temp = 42.1;
sample.ambientTemp = 29.7;
sample.state = "NORMAL";
```

---

### 4.2 DataStorageService

Files:

- [data_storage_service.h](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/services/data_storage_service.h)
- [data_storage_service.cpp](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/services/data_storage_service.cpp)

#### Role

`DataStorageService` handles history persistence.

It:

- stores samples in a local SQLite history database
- keeps only the last 24 hours
- exports the current history as CSV when requested

#### Main Functions

```cpp
appendSample(const SensorSample &sample)
cleanOldData()
loadLast24hSamples(int limit, int offset)
exportCsv(const QString &destinationPath)
```

#### Export CSV Format

Exported format:

```text
timestamp,anomaly,x,y,z,temp
```

Example:

```text
2026-04-09T14:32:10.000Z,80,0.52,0.48,0.50,42.1
```

#### 24h Rolling Logic

The service uses a simple strategy:

1. insert new rows into the local history table
2. periodically delete old rows
3. keep only rows newer than 24 hours
4. export filtered rows to CSV when the user asks

This approach is simple and easy to explain.

#### Example

Append one sample:

```cpp
SensorSample sample;
sample.timestampUtc = QDateTime::currentDateTimeUtc();
sample.anomalyScore = 80;
sample.x = 0.52;
sample.y = 0.48;
sample.z = 0.50;
sample.temp = 42.1;

storageService.appendSample(sample);
```

---

### 4.3 HistoryService

At the moment, there is no separate `HistoryService` class.

Current behavior:

- history file access is handled by `DataStorageService`
- history preparation for charts is handled inside `AppController`

This is acceptable for a small project because:

- it avoids unnecessary files
- it keeps architecture simple
- it is still easy to follow

If the project grows, a dedicated `HistoryService` can be introduced later.

---

## 5. AppController

Files:

- [app_controller.h](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/controllers/app_controller.h)
- [app_controller.cpp](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/controllers/app_controller.cpp)

### 5.1 Role

`AppController` is the central coordinator of the application.

It connects:

- `SerialService`
- `DataStorageService`
- QML pages and widgets

### 5.2 Main Flow

```text
SerialService → AppController → UI
                            → Storage
```

### 5.3 Responsibilities

`AppController` is responsible for:

- receiving live `SensorSample` objects
- updating current values for the dashboard
- aggregating live chart values
- storing downsampled samples
- preparing history data for the History page
- exposing QML-friendly properties

### 5.4 Important Public Functions

```cpp
connectToPort(const QString &portName)
connectPreferredPort(const QString &preferredPort)
disconnectPort()
disconnectAndReset()
refreshPorts()
setSelectedPort(const QString &portName)
refreshHistoryData()
exportHistoryCsv(const QUrl &fileUrl)
```

### 5.5 Important Internal Functions

```cpp
processSample(const SensorSample &sample)
flushLiveData()
storeHistorySample(double anomalyScore, double x, double y, double z, double temp)
parseHistorySamples(const QVector<SensorSample> &samples)
updateHistoryData(const ParsedHistory &parsedHistory)
```

### 5.6 Example Flow

When one UART frame arrives:

1. `SerialService` parses it into `SensorSample`
2. `AppController::onSampleReceived()` receives the sample
3. `processSample()` updates live values
4. `flushLiveData()` updates chart arrays
5. `storeHistorySample()` sends a downsampled sample to storage

---

## 6. UI Structure

### 6.1 Main Pages

#### SetupPage

File:

- [SetupPage.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/SetupPage.qml)

Role:

- detects and connects to the serial device
- shows detected machine type
- shows device ID
- allows entering the dashboard

#### Dashboard

File:

- [Dashboard.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/Dashboard.qml)

Role:

- displays real-time gauges
- displays real-time charts
- gives access to History view

#### HistoryPage

File:

- [HistoryPage.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/HistoryPage.qml)

Role:

- shows last 24h stored data
- allows refresh
- allows CSV export

### 6.2 Main Widgets

#### GaugeCard

File:

- [GaugeCard.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/components/GaugeCard.qml)

Role:

- shows a gauge with zones and needle
- used for anomaly score and temperature

#### ChartCard

File:

- [ChartCard.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/components/ChartCard.qml)

Role:

- reusable container for chart widgets
- standardizes chart presentation

#### DashboardHeaderBar

File:

- [DashboardHeaderBar.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/components/DashboardHeaderBar.qml)

Role:

- top control bar
- connect/disconnect
- theme toggle
- history navigation

#### AxisSelectorCombo

File:

- [AxisSelectorCombo.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/components/AxisSelectorCombo.qml)

Role:

- selects the displayed acceleration axis
- values: `X`, `Y`, `Z`

---

## 7. Chart System

### 7.1 Dashboard Charts

The dashboard contains:

- anomaly score vs time
- acceleration vs time

### 7.2 Axis Switching

The acceleration chart is reused for three axes:

- X
- Y
- Z

The selected axis is controlled by:

- `selectedAxis`

The chart data is then selected from:

- `xAxisValues`
- `yAxisValues`
- `zAxisValues`

### 7.3 Reuse Principle

Only one chart component is reused.

This is better than creating three separate charts because:

- less code duplication
- simpler maintenance
- cleaner UI

### 7.4 History Charts

The History page uses:

- anomaly score chart
- RMS chart
- temperature chart

These charts share:

- the same time window
- the same pan/zoom interaction

---

## 8. Gauge System

### 8.1 Main Gauges

The dashboard uses:

- anomaly gauge
- machine temperature gauge

### 8.2 Gauge Features

Each gauge includes:

- semi-circular arc
- colored zones
- center needle
- large numeric value
- label text

### 8.3 Zone Logic

Typical zone mapping:

- green:
  healthy range
- yellow:
  warning range
- red:
  critical range

### 8.4 Needle Behavior

The needle angle is derived from:

- `value`
- `min`
- `max`

This makes the gauge easy to read instantly.

---

## 9. Data Storage Strategy

### 9.1 Storage Choice

The project uses a local SQLite database for rolling history, with CSV export available from the History page.

Why:

- keeps history queries fast
- avoids loading the whole history file for every view
- still allows portable CSV export
- supports the 24-hour rolling window cleanly

### 9.2 Strategy

The storage strategy is:

- append-only writes
- periodic cleanup
- keep last 24 hours only
- export current file when needed

### 9.3 Downsampling

The application does not store every raw UART sample.

Instead, it stores a reduced rate sample approximately every:

- `250 ms`

Benefits:

- smaller file size
- enough detail for charting

### 9.4 Why SQLite

SQLite is used because:

- it is bundled through Qt's SQL support
- it is reliable for local rolling history
- it keeps the app independent from external training assets

---

## 10. Testing Strategy

### 10.1 UART Simulation

A practical way to test the project is to simulate UART input from a PC script.

Typical approach:

- generate fake CSV-like sensor frames
- send them over a virtual or real serial port
- observe the Qt application behavior

Example simulated frame:

```text
UID123,78,0.40,0.45,0.42,41.7,29.5
```

### 10.2 Short Test Window

For validation, a short rolling window can be used temporarily.

Example:

- 1 minute instead of 24 hours

This makes it easier to:

- test cleanup logic
- verify export behavior
- inspect chart refresh quickly

### 10.3 Real-Time Validation

Important checks:

- UART connection opens correctly
- samples appear in dashboard
- anomaly score updates correctly
- axis switching updates the acceleration chart
- history refresh loads the rolling file
- CSV export creates the expected file

---

## 11. Key Design Decisions

### 11.1 Why C++ and Qt

Reasons:

- strong performance
- native desktop UI
- good serial communication support
- good fit for embedded and industrial workflows

### 11.2 Why CSV Export

Reasons:

- easy to debug
- easy to export
- human-readable
- useful for reports without keeping training data inside the project

### 11.3 Why MVC-Inspired Architecture

Reasons:

- separates data, logic, and presentation
- makes the code easier to explain
- reduces duplication
- improves maintainability

### 11.4 Why AppController

Using one central controller helps:

- keep UI simple
- centralize application flow
- avoid business logic in QML pages

---

## 12. Conclusion

EdgeGuard is a simple, efficient, and maintainable monitoring application.

Main strengths:

- clear architecture
- reusable UI components
- simple UART communication pipeline
- practical 24h rolling storage
- easy-to-read industrial dashboard

This design is suitable for:

- predictive maintenance demonstration
- industrial monitoring prototypes
- PFE presentation and explanation

It is also scalable enough for future additions such as:

- more advanced analytics
- extra machine types
- separate history preparation service
- cloud synchronization

---

## Appendix A - Important Files

### Backend

- [main.cpp](D:/Dev/Qt/EdgeGuard/EdgeGuard/main.cpp)
- [sensor_sample.h](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/models/sensor_sample.h)
- [serial_service.h](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/services/serial_service.h)
- [serial_service.cpp](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/services/serial_service.cpp)
- [data_storage_service.h](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/services/data_storage_service.h)
- [data_storage_service.cpp](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/services/data_storage_service.cpp)
- [app_controller.h](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/controllers/app_controller.h)
- [app_controller.cpp](D:/Dev/Qt/EdgeGuard/EdgeGuard/backend/controllers/app_controller.cpp)

### UI

- [SetupPage.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/SetupPage.qml)
- [Dashboard.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/Dashboard.qml)
- [HistoryPage.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/HistoryPage.qml)
- [GaugeCard.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/components/GaugeCard.qml)
- [ChartCard.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/components/ChartCard.qml)
- [LiveTrendChart.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/components/LiveTrendChart.qml)
- [HistoryChart.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/components/HistoryChart.qml)
- [HistoryChartPanel.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/components/HistoryChartPanel.qml)
- [DashboardHeaderBar.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/components/DashboardHeaderBar.qml)
- [AxisSelectorCombo.qml](D:/Dev/Qt/EdgeGuard/EdgeGuard/components/AxisSelectorCombo.qml)
