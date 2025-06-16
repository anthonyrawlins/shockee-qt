#include "mainwindow.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_serialComm(new SerialCommunicator(this))
    , m_dataLogger(new DataLogger(this))
    , m_displayUpdateTimer(new QTimer(this))
    , m_recordingTimer(new QTimer(this))
    , m_isRecording(false)
    , m_isConnected(false)
    , m_recordingStartTime(0)
{
    setupUI();
    setupMenus();
    setupStatusBar();
    setupConnections();
    
    // Setup timers
    m_displayUpdateTimer->setInterval(DISPLAY_UPDATE_INTERVAL);
    m_recordingTimer->setInterval(1000); // 1 second for recording time display
    
    // Load serial ports
    m_serialPortCombo->addItems(m_serialComm->getAvailablePorts());
    
    // Add common virtual port patterns for testing
    m_serialPortCombo->addItem("/tmp/ttyV1 (Virtual)");
    m_serialPortCombo->lineEdit()->setPlaceholderText("Select or type port (e.g., /tmp/ttyV1)");
    
    resetDisplay();
}

MainWindow::~MainWindow()
{
    if (m_isConnected) {
        m_serialComm->disconnect();
    }
}

void MainWindow::setupUI()
{
    setWindowTitle("Shockee - Motorbike Suspension Dyno v1.0.0");
    setMinimumSize(1200, 800);
    
    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);
    
    // Real-time tab
    m_realTimeTab = new QWidget();
    m_tabWidget->addTab(m_realTimeTab, "Real-time Data");
    
    // Analysis tab  
    m_analysisTab = new QWidget();
    m_tabWidget->addTab(m_analysisTab, "Analysis");
    
    // Comparison tab
    m_comparisonTab = new QWidget();
    m_tabWidget->addTab(m_comparisonTab, "Comparison");
    
    setupRealTimeTab();
    setupAnalysisTab();
    setupComparisonTab();
}

void MainWindow::setupRealTimeTab()
{
    QHBoxLayout* mainLayout = new QHBoxLayout(m_realTimeTab);
    
    // Left panel - controls and sensor displays
    QWidget* leftPanel = new QWidget();
    leftPanel->setMaximumWidth(300);
    leftPanel->setMinimumWidth(250);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    
    // Connection group
    m_connectionGroup = new QGroupBox("Connection");
    QGridLayout* connLayout = new QGridLayout(m_connectionGroup);
    
    connLayout->addWidget(new QLabel("Serial Port:"), 0, 0);
    m_serialPortCombo = new QComboBox();
    m_serialPortCombo->setEditable(true); // Allow manual entry for virtual ports
    connLayout->addWidget(m_serialPortCombo, 0, 1);
    
    m_connectButton = new QPushButton("Connect");
    m_disconnectButton = new QPushButton("Disconnect");
    m_disconnectButton->setEnabled(false);
    connLayout->addWidget(m_connectButton, 1, 0);
    connLayout->addWidget(m_disconnectButton, 1, 1);
    
    m_connectionStatus = new QLabel("Disconnected");
    m_connectionStatus->setStyleSheet("color: red; font-weight: bold;");
    connLayout->addWidget(m_connectionStatus, 2, 0, 1, 2);
    
    leftLayout->addWidget(m_connectionGroup);
    
    // Recording group
    m_recordingGroup = new QGroupBox("Recording");
    QGridLayout* recLayout = new QGridLayout(m_recordingGroup);
    
    m_startRecordButton = new QPushButton("Start Recording");
    m_stopRecordButton = new QPushButton("Stop Recording");
    m_stopRecordButton->setEnabled(false);
    recLayout->addWidget(m_startRecordButton, 0, 0);
    recLayout->addWidget(m_stopRecordButton, 0, 1);
    
    m_saveButton = new QPushButton("Save Session");
    m_loadButton = new QPushButton("Load Session");
    m_saveButton->setEnabled(false);
    recLayout->addWidget(m_saveButton, 1, 0);
    recLayout->addWidget(m_loadButton, 1, 1);
    
    m_recordingTime = new QLabel("00:00");
    m_recordingTime->setStyleSheet("font-size: 16px; font-weight: bold;");
    recLayout->addWidget(m_recordingTime, 2, 0, 1, 2);
    
    m_recordingProgress = new QProgressBar();
    m_recordingProgress->setMaximum(MAX_RECORDING_TIME / 1000);
    recLayout->addWidget(m_recordingProgress, 3, 0, 1, 2);
    
    leftLayout->addWidget(m_recordingGroup);
    
    // Sensor displays group
    m_sensorGroup = new QGroupBox("Live Sensor Data");
    QGridLayout* sensorLayout = new QGridLayout(m_sensorGroup);
    
    sensorLayout->addWidget(new QLabel("Position:"), 0, 0);
    m_positionDisplay = new QLabel("0.00 mm");
    m_positionDisplay->setStyleSheet("font-size: 14px; font-weight: bold; color: blue;");
    sensorLayout->addWidget(m_positionDisplay, 0, 1);
    
    sensorLayout->addWidget(new QLabel("Force:"), 1, 0);
    m_forceDisplay = new QLabel("0.00 kg");
    m_forceDisplay->setStyleSheet("font-size: 14px; font-weight: bold; color: red;");
    sensorLayout->addWidget(m_forceDisplay, 1, 1);
    
    sensorLayout->addWidget(new QLabel("Encoder:"), 2, 0);
    m_encoderDisplay = new QLabel("0 pulses");
    m_encoderDisplay->setStyleSheet("font-size: 14px; font-weight: bold; color: green;");
    sensorLayout->addWidget(m_encoderDisplay, 2, 1);
    
    sensorLayout->addWidget(new QLabel("Velocity:"), 3, 0);
    m_velocityDisplay = new QLabel("0.00 mm/s");
    m_velocityDisplay->setStyleSheet("font-size: 14px; font-weight: bold; color: purple;");
    sensorLayout->addWidget(m_velocityDisplay, 3, 1);
    
    leftLayout->addWidget(m_sensorGroup);
    leftLayout->addStretch();
    
    // Right panel - plots
    QWidget* rightPanel = new QWidget();
    QGridLayout* plotLayout = new QGridLayout(rightPanel);
    
    // Create plot widgets
    m_positionPlot = new PlotWidget(PlotWidget::Position);
    m_positionPlot->setMinimumHeight(200);
    plotLayout->addWidget(m_positionPlot, 0, 0);
    
    m_forcePlot = new PlotWidget(PlotWidget::Force);
    m_forcePlot->setMinimumHeight(200);
    plotLayout->addWidget(m_forcePlot, 0, 1);
    
    m_encoderPlot = new PlotWidget(PlotWidget::Encoder);
    m_encoderPlot->setMinimumHeight(200);
    plotLayout->addWidget(m_encoderPlot, 1, 0);
    
    m_forceVsPositionPlot = new PlotWidget(PlotWidget::ForceVsPosition);
    m_forceVsPositionPlot->setMinimumHeight(200);
    plotLayout->addWidget(m_forceVsPositionPlot, 1, 1);
    
    // Add panels to main layout
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter);
}

void MainWindow::setupAnalysisTab()
{
    QVBoxLayout* layout = new QVBoxLayout(m_analysisTab);
    layout->addWidget(new QLabel("Analysis features will be implemented here"));
}

void MainWindow::setupComparisonTab()
{
    QVBoxLayout* layout = new QVBoxLayout(m_comparisonTab);
    
    // Comparison controls
    QHBoxLayout* controlsLayout = new QHBoxLayout();
    m_overlayCheckbox = new QCheckBox("Overlay Mode");
    m_loadComparisonButton = new QPushButton("Load Comparison Session");
    controlsLayout->addWidget(m_overlayCheckbox);
    controlsLayout->addWidget(m_loadComparisonButton);
    controlsLayout->addStretch();
    
    layout->addLayout(controlsLayout);
    
    // Comparison plot
    m_comparisonPlot = new PlotWidget(PlotWidget::Comparison);
    layout->addWidget(m_comparisonPlot);
}

void MainWindow::setupMenus()
{
    QMenuBar* menuBar = this->menuBar();
    
    // File menu
    QMenu* fileMenu = menuBar->addMenu("File");
    
    QAction* newSessionAction = fileMenu->addAction("New Session");
    connect(newSessionAction, &QAction::triggered, this, &MainWindow::startRecording);
    
    QAction* saveAction = fileMenu->addAction("Save Session");
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveSession);
    
    QAction* loadAction = fileMenu->addAction("Load Session");
    connect(loadAction, &QAction::triggered, this, &MainWindow::loadSession);
    
    fileMenu->addSeparator();
    
    QAction* exportAction = fileMenu->addAction("Export Data...");
    connect(exportAction, &QAction::triggered, this, &MainWindow::exportData);
    
    fileMenu->addSeparator();
    
    QAction* quitAction = fileMenu->addAction("Quit");
    connect(quitAction, &QAction::triggered, this, &QWidget::close);
    
    // Tools menu
    QMenu* toolsMenu = menuBar->addMenu("Tools");
    
    QAction* calibrationAction = toolsMenu->addAction("Calibration...");
    connect(calibrationAction, &QAction::triggered, this, &MainWindow::showCalibration);
}

void MainWindow::setupStatusBar()
{
    statusBar()->showMessage("Ready");
}

void MainWindow::setupConnections()
{
    // Serial communication
    connect(m_serialComm, &SerialCommunicator::dataReceived, 
            this, &MainWindow::onNewDataReceived);
    connect(m_serialComm, &SerialCommunicator::connectionStatusChanged,
            this, &MainWindow::onConnectionStatusChanged);
    
    // UI connections
    connect(m_connectButton, &QPushButton::clicked, 
            this, &MainWindow::connectToArduino);
    connect(m_disconnectButton, &QPushButton::clicked,
            this, &MainWindow::disconnectFromArduino);
    connect(m_startRecordButton, &QPushButton::clicked,
            this, &MainWindow::startRecording);
    connect(m_stopRecordButton, &QPushButton::clicked,
            this, &MainWindow::stopRecording);
    connect(m_saveButton, &QPushButton::clicked,
            this, &MainWindow::saveSession);
    connect(m_loadButton, &QPushButton::clicked,
            this, &MainWindow::loadSession);
    connect(m_overlayCheckbox, &QCheckBox::toggled,
            this, &MainWindow::toggleOverlay);
    connect(m_loadComparisonButton, &QPushButton::clicked,
            this, &MainWindow::loadComparisonSession);
    
    // Timers
    connect(m_displayUpdateTimer, &QTimer::timeout,
            this, &MainWindow::updateDisplay);
    connect(m_recordingTimer, &QTimer::timeout,
            this, &MainWindow::updateDisplay);
}

void MainWindow::connectToArduino()
{
    QString portName = m_serialPortCombo->currentText();
    if (portName.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a serial port");
        return;
    }
    
    // Clean up port name if it has description
    if (portName.contains(" (Virtual)")) {
        portName = portName.split(" ").first();
    }
    
    if (m_serialComm->connectToPort(portName)) {
        statusBar()->showMessage("Connected to " + portName);
    } else {
        statusBar()->showMessage("Failed to connect to " + portName);
    }
}

void MainWindow::disconnectFromArduino()
{
    m_serialComm->disconnect();
    statusBar()->showMessage("Disconnected");
}

void MainWindow::startRecording()
{
    if (!m_isConnected) {
        QMessageBox::warning(this, "Error", "Please connect to Arduino first");
        return;
    }
    
    m_isRecording = true;
    m_recordingStartTime = QDateTime::currentMSecsSinceEpoch();
    m_currentSession.clear();
    
    m_startRecordButton->setEnabled(false);
    m_stopRecordButton->setEnabled(true);
    m_saveButton->setEnabled(false);
    
    m_displayUpdateTimer->start();
    m_recordingTimer->start();
    
    // Clear plots
    m_positionPlot->clearData();
    m_forcePlot->clearData();
    m_encoderPlot->clearData();
    m_forceVsPositionPlot->clearData();
    
    statusBar()->showMessage("Recording started");
}

void MainWindow::stopRecording()
{
    m_isRecording = false;
    
    m_startRecordButton->setEnabled(true);
    m_stopRecordButton->setEnabled(false);
    m_saveButton->setEnabled(true);
    
    m_recordingTimer->stop();
    
    statusBar()->showMessage("Recording stopped");
}

void MainWindow::saveSession()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "Save Session", m_dataLogger->getSessionsDirectory(),
        "Shockee Session Files (*.json)");
    
    if (!fileName.isEmpty()) {
        Session session;
        session.name = QFileInfo(fileName).baseName();
        session.timestamp = QDateTime::currentDateTime();
        session.data = QVector<SensorData>(m_currentSession.begin(), m_currentSession.end());
        
        if (m_dataLogger->saveSession(session, fileName)) {
            statusBar()->showMessage("Session saved: " + fileName);
        } else {
            QMessageBox::warning(this, "Error", "Failed to save session");
        }
    }
}

void MainWindow::loadSession()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Load Session", m_dataLogger->getSessionsDirectory(),
        "Shockee Session Files (*.json)");
    
    if (!fileName.isEmpty()) {
        Session session = m_dataLogger->loadSession(fileName);
        if (!session.data.isEmpty()) {
            m_currentSession = QList<SensorData>(session.data.begin(), session.data.end());
            
            // Update plots with loaded data
            m_positionPlot->clearData();
            m_forcePlot->clearData();
            m_encoderPlot->clearData();
            m_forceVsPositionPlot->clearData();
            m_comparisonPlot->clearData();
            
            // Add data to main plots
            QVector<SensorData> dataVector(m_currentSession.begin(), m_currentSession.end());
            m_positionPlot->addDataSeries(dataVector, session.name);
            m_forcePlot->addDataSeries(dataVector, session.name);
            m_encoderPlot->addDataSeries(dataVector, session.name);
            m_forceVsPositionPlot->addDataSeries(dataVector, session.name);
            
            // Add to comparison plot as main dataset
            m_comparisonPlot->addDataSeries(dataVector, session.name);
            
            statusBar()->showMessage("Session loaded: " + fileName);
        } else {
            QMessageBox::warning(this, "Error", "Failed to load session");
        }
    }
}

void MainWindow::loadComparisonSession()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        "Load Comparison Session", m_dataLogger->getSessionsDirectory(),
        "Shockee Session Files (*.json)");
    
    if (!fileName.isEmpty()) {
        Session session = m_dataLogger->loadSession(fileName);
        if (!session.data.isEmpty()) {
            m_comparisonSession = QList<SensorData>(session.data.begin(), session.data.end());
            
            // Add to comparison plot as overlay
            QVector<SensorData> dataVector(m_comparisonSession.begin(), m_comparisonSession.end());
            m_comparisonPlot->addOverlayData(dataVector, session.name);
            
            // Enable overlay mode automatically
            m_overlayCheckbox->setChecked(true);
            m_comparisonPlot->setOverlayMode(true);
            
            statusBar()->showMessage("Comparison session loaded: " + fileName);
        } else {
            QMessageBox::warning(this, "Error", "Failed to load comparison session");
        }
    }
}

void MainWindow::exportData()
{
    if (m_currentSession.isEmpty()) {
        QMessageBox::information(this, "Info", "No data to export");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this,
        "Export Data", "", "CSV Files (*.csv);;Excel Files (*.xlsx)");
    
    if (!fileName.isEmpty()) {
        Session session;
        session.data = QVector<SensorData>(m_currentSession.begin(), m_currentSession.end());
        
        bool success = false;
        if (fileName.endsWith(".csv")) {
            success = m_dataLogger->exportToCsv(session, fileName);
        } else if (fileName.endsWith(".xlsx")) {
            success = m_dataLogger->exportToExcel(session, fileName);
        }
        
        if (success) {
            statusBar()->showMessage("Data exported: " + fileName);
        } else {
            QMessageBox::warning(this, "Error", "Failed to export data");
        }
    }
}

void MainWindow::showCalibration()
{
    CalibrationDialog dialog(m_serialComm, this);
    dialog.exec();
}

void MainWindow::onNewDataReceived(const SensorData& data)
{
    if (m_isRecording) {
        m_currentSession.append(data);
        
        // Add to plots
        m_positionPlot->addDataPoint(data);
        m_forcePlot->addDataPoint(data);
        m_encoderPlot->addDataPoint(data);
        m_forceVsPositionPlot->addDataPoint(data);
    }
    
    // Update sensor displays
    updateSensorDisplays(data);
}

void MainWindow::onConnectionStatusChanged(bool connected)
{
    m_isConnected = connected;
    
    m_connectButton->setEnabled(!connected);
    m_disconnectButton->setEnabled(connected);
    m_startRecordButton->setEnabled(connected && !m_isRecording);
    
    if (connected) {
        m_connectionStatus->setText("Connected");
        m_connectionStatus->setStyleSheet("color: green; font-weight: bold;");
        m_displayUpdateTimer->start();
    } else {
        m_connectionStatus->setText("Disconnected");
        m_connectionStatus->setStyleSheet("color: red; font-weight: bold;");
        m_displayUpdateTimer->stop();
        
        if (m_isRecording) {
            stopRecording();
        }
    }
}

void MainWindow::updateDisplay()
{
    if (m_isRecording) {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_recordingStartTime;
        int seconds = elapsed / 1000;
        int minutes = seconds / 60;
        seconds = seconds % 60;
        
        m_recordingTime->setText(QString("%1:%2").arg(minutes, 2, 10, QChar('0'))
                                                  .arg(seconds, 2, 10, QChar('0')));
        m_recordingProgress->setValue(seconds + minutes * 60);
        
        // Auto-stop at maximum recording time
        if (elapsed >= MAX_RECORDING_TIME) {
            stopRecording();
        }
    }
}

void MainWindow::updateSensorDisplays(const SensorData& data)
{
    m_positionDisplay->setText(QString("%1 mm").arg(data.position, 0, 'f', 2));
    m_forceDisplay->setText(QString("%1 kg").arg(data.force, 0, 'f', 2));
    m_encoderDisplay->setText(QString("%1 pulses").arg(data.encoderPulses));
    m_velocityDisplay->setText(QString("%1 mm/s").arg(data.velocity, 0, 'f', 2));
}

void MainWindow::resetDisplay()
{
    m_positionDisplay->setText("0.00 mm");
    m_forceDisplay->setText("0.00 kg");
    m_encoderDisplay->setText("0 pulses");
    m_velocityDisplay->setText("0.00 mm/s");
    m_recordingTime->setText("00:00");
    m_recordingProgress->setValue(0);
}

void MainWindow::toggleOverlay()
{
    bool overlayMode = m_overlayCheckbox->isChecked();
    m_comparisonPlot->setOverlayMode(overlayMode);
}