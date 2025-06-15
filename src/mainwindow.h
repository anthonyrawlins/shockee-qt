#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QProgressBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QAction>
#include <QTimer>
#include <QGroupBox>
#include <QCheckBox>

#include "serialcommunicator.h"
#include "datalogger.h"
#include "plotwidget.h"
#include "calibrationdialog.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void connectToArduino();
    void disconnectFromArduino();
    void startRecording();
    void stopRecording();
    void saveSession();
    void loadSession();
    void loadComparisonSession();
    void exportData();
    void showCalibration();
    void onNewDataReceived(const SensorData& data);
    void onConnectionStatusChanged(bool connected);
    void updateDisplay();
    void toggleOverlay();

private:
    void setupUI();
    void setupRealTimeTab();
    void setupAnalysisTab();
    void setupComparisonTab();
    void setupMenus();
    void setupStatusBar();
    void setupConnections();
    void updateSensorDisplays(const SensorData& data);
    void resetDisplay();

    // UI Components
    QTabWidget* m_tabWidget;
    QWidget* m_realTimeTab;
    QWidget* m_analysisTab;
    QWidget* m_comparisonTab;
    
    // Control Panel
    QGroupBox* m_connectionGroup;
    QComboBox* m_serialPortCombo;
    QPushButton* m_connectButton;
    QPushButton* m_disconnectButton;
    QLabel* m_connectionStatus;
    
    QGroupBox* m_recordingGroup;
    QPushButton* m_startRecordButton;
    QPushButton* m_stopRecordButton;
    QPushButton* m_saveButton;
    QPushButton* m_loadButton;
    QLabel* m_recordingTime;
    QProgressBar* m_recordingProgress;
    
    // Sensor Displays
    QGroupBox* m_sensorGroup;
    QLabel* m_positionDisplay;
    QLabel* m_forceDisplay;
    QLabel* m_encoderDisplay;
    QLabel* m_velocityDisplay;
    
    // Plot Widgets
    PlotWidget* m_positionPlot;
    PlotWidget* m_forcePlot;
    PlotWidget* m_encoderPlot;
    PlotWidget* m_forceVsPositionPlot;
    
    // Comparison Tools
    QCheckBox* m_overlayCheckbox;
    QPushButton* m_loadComparisonButton;
    PlotWidget* m_comparisonPlot;
    
    // Backend Components
    SerialCommunicator* m_serialComm;
    DataLogger* m_dataLogger;
    
    // Timers
    QTimer* m_displayUpdateTimer;
    QTimer* m_recordingTimer;
    
    // Data
    QList<SensorData> m_currentSession;
    QList<SensorData> m_comparisonSession;
    
    // State
    bool m_isRecording;
    bool m_isConnected;
    qint64 m_recordingStartTime;
    
    // Constants
    static const int MAX_RECORDING_TIME = 120000; // 2 minutes in ms
    static const int DISPLAY_UPDATE_INTERVAL = 50; // 20 FPS
};

#endif // MAINWINDOW_H