#ifndef CALIBRATIONDIALOG_H
#define CALIBRATIONDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QTimer>
#include <QTabWidget>

#include "serialcommunicator.h"

class CalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CalibrationDialog(SerialCommunicator* serialComm, QWidget *parent = nullptr);

private slots:
    void onSensorDataReceived(const SensorData& data);
    void calibrateLoadCellZero();
    void calibrateLoadCellScale();
    void calibratePotentiometerMin();
    void calibratePotentiometerMax();
    void resetEncoder();
    void startCalibration();
    void finishCalibration();
    void updateLiveReadings();

private:
    void setupUI();
    void updateDisplays(const SensorData& data);
    void saveCalibrationSettings();
    void loadCalibrationSettings();
    
    SerialCommunicator* m_serialComm;
    
    // UI Components
    QTabWidget* m_tabWidget;
    
    // Load Cell Calibration
    QGroupBox* m_loadCellGroup;
    QLabel* m_loadCellRaw;
    QLabel* m_loadCellCalibrated;
    QPushButton* m_tareButton;
    QPushButton* m_calibrateButton;
    QDoubleSpinBox* m_knownWeight;
    QLabel* m_calibrationFactor;
    
    // Potentiometer Calibration
    QGroupBox* m_potGroup;
    QLabel* m_potRaw;
    QLabel* m_potCalibrated;
    QPushButton* m_potMinButton;
    QPushButton* m_potMaxButton;
    QSpinBox* m_potMinValue;
    QSpinBox* m_potMaxValue;
    QDoubleSpinBox* m_strokeLength;
    
    // Encoder Calibration
    QGroupBox* m_encoderGroup;
    QLabel* m_encoderCount;
    QPushButton* m_encoderResetButton;
    QSpinBox* m_encoderPPR; // Pulses per revolution
    
    // Live readings
    QTimer* m_updateTimer;
    SensorData m_lastData;
    
    // Calibration state
    bool m_isCalibrating;
    int m_calibrationStep;
    
    // Calibration values
    double m_loadCellZero;
    double m_loadCellScale;
    int m_potMin;
    int m_potMax;
    double m_strokeLengthMm;
    int m_encoderPulsesPerRev;
};

#endif // CALIBRATIONDIALOG_H