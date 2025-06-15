#include "calibrationdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QSettings>
#include <QPushButton>

CalibrationDialog::CalibrationDialog(SerialCommunicator* serialComm, QWidget *parent)
    : QDialog(parent)
    , m_serialComm(serialComm)
    , m_updateTimer(new QTimer(this))
    , m_isCalibrating(false)
    , m_calibrationStep(0)
    , m_loadCellZero(0)
    , m_loadCellScale(1.0)
    , m_potMin(0)
    , m_potMax(1023)
    , m_strokeLengthMm(75.0)
    , m_encoderPulsesPerRev(1000)
{
    setWindowTitle("Sensor Calibration");
    setModal(true);
    resize(600, 400);
    
    setupUI();
    loadCalibrationSettings();
    
    // Connect to sensor data
    connect(m_serialComm, &SerialCommunicator::dataReceived,
            this, &CalibrationDialog::onSensorDataReceived);
    
    // Setup update timer
    m_updateTimer->setInterval(100); // 10 Hz update
    connect(m_updateTimer, &QTimer::timeout, this, &CalibrationDialog::updateLiveReadings);
    
    if (m_serialComm->isConnected()) {
        m_updateTimer->start();
    }
}

void CalibrationDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    m_tabWidget = new QTabWidget();
    mainLayout->addWidget(m_tabWidget);
    
    // Load Cell Tab
    QWidget* loadCellTab = new QWidget();
    m_tabWidget->addTab(loadCellTab, "Load Cell");
    
    QVBoxLayout* loadCellLayout = new QVBoxLayout(loadCellTab);
    
    m_loadCellGroup = new QGroupBox("Load Cell Calibration");
    QGridLayout* loadCellGridLayout = new QGridLayout(m_loadCellGroup);
    
    loadCellGridLayout->addWidget(new QLabel("Raw Reading:"), 0, 0);
    m_loadCellRaw = new QLabel("0");
    m_loadCellRaw->setStyleSheet("font-weight: bold; color: blue;");
    loadCellGridLayout->addWidget(m_loadCellRaw, 0, 1);
    
    loadCellGridLayout->addWidget(new QLabel("Calibrated Reading:"), 1, 0);
    m_loadCellCalibrated = new QLabel("0.00 kg");
    m_loadCellCalibrated->setStyleSheet("font-weight: bold; color: green;");
    loadCellGridLayout->addWidget(m_loadCellCalibrated, 1, 1);
    
    m_tareButton = new QPushButton("Tare (Zero)");
    loadCellGridLayout->addWidget(m_tareButton, 2, 0);
    
    loadCellGridLayout->addWidget(new QLabel("Known Weight (kg):"), 3, 0);
    m_knownWeight = new QDoubleSpinBox();
    m_knownWeight->setRange(0, 1000);
    m_knownWeight->setDecimals(2);
    m_knownWeight->setValue(10.0);
    loadCellGridLayout->addWidget(m_knownWeight, 3, 1);
    
    m_calibrateButton = new QPushButton("Calibrate Scale");
    loadCellGridLayout->addWidget(m_calibrateButton, 4, 0);
    
    loadCellGridLayout->addWidget(new QLabel("Calibration Factor:"), 5, 0);
    m_calibrationFactor = new QLabel("1.0");
    m_calibrationFactor->setStyleSheet("font-weight: bold;");
    loadCellGridLayout->addWidget(m_calibrationFactor, 5, 1);
    
    loadCellLayout->addWidget(m_loadCellGroup);
    loadCellLayout->addStretch();
    
    // Potentiometer Tab
    QWidget* potTab = new QWidget();
    m_tabWidget->addTab(potTab, "Potentiometer");
    
    QVBoxLayout* potLayout = new QVBoxLayout(potTab);
    
    m_potGroup = new QGroupBox("Potentiometer Calibration");
    QGridLayout* potGridLayout = new QGridLayout(m_potGroup);
    
    potGridLayout->addWidget(new QLabel("Raw Reading:"), 0, 0);
    m_potRaw = new QLabel("0");
    m_potRaw->setStyleSheet("font-weight: bold; color: blue;");
    potGridLayout->addWidget(m_potRaw, 0, 1);
    
    potGridLayout->addWidget(new QLabel("Calibrated Position:"), 1, 0);
    m_potCalibrated = new QLabel("0.00 mm");
    m_potCalibrated->setStyleSheet("font-weight: bold; color: green;");
    potGridLayout->addWidget(m_potCalibrated, 1, 1);
    
    potGridLayout->addWidget(new QLabel("Stroke Length (mm):"), 2, 0);
    m_strokeLength = new QDoubleSpinBox();
    m_strokeLength->setRange(1, 500);
    m_strokeLength->setDecimals(1);
    m_strokeLength->setValue(75.0);
    potGridLayout->addWidget(m_strokeLength, 2, 1);
    
    m_potMinButton = new QPushButton("Set Min Position");
    potGridLayout->addWidget(m_potMinButton, 3, 0);
    
    potGridLayout->addWidget(new QLabel("Min Value:"), 3, 1);
    m_potMinValue = new QSpinBox();
    m_potMinValue->setRange(0, 1023);
    m_potMinValue->setValue(0);
    potGridLayout->addWidget(m_potMinValue, 3, 2);
    
    m_potMaxButton = new QPushButton("Set Max Position");
    potGridLayout->addWidget(m_potMaxButton, 4, 0);
    
    potGridLayout->addWidget(new QLabel("Max Value:"), 4, 1);
    m_potMaxValue = new QSpinBox();
    m_potMaxValue->setRange(0, 1023);
    m_potMaxValue->setValue(1023);
    potGridLayout->addWidget(m_potMaxValue, 4, 2);
    
    potLayout->addWidget(m_potGroup);
    potLayout->addStretch();
    
    // Encoder Tab
    QWidget* encoderTab = new QWidget();
    m_tabWidget->addTab(encoderTab, "Encoder");
    
    QVBoxLayout* encoderLayout = new QVBoxLayout(encoderTab);
    
    m_encoderGroup = new QGroupBox("Encoder Calibration");
    QGridLayout* encoderGridLayout = new QGridLayout(m_encoderGroup);
    
    encoderGridLayout->addWidget(new QLabel("Pulse Count:"), 0, 0);
    m_encoderCount = new QLabel("0");
    m_encoderCount->setStyleSheet("font-weight: bold; color: blue;");
    encoderGridLayout->addWidget(m_encoderCount, 0, 1);
    
    m_encoderResetButton = new QPushButton("Reset Count");
    encoderGridLayout->addWidget(m_encoderResetButton, 1, 0);
    
    encoderGridLayout->addWidget(new QLabel("Pulses per Revolution:"), 2, 0);
    m_encoderPPR = new QSpinBox();
    m_encoderPPR->setRange(100, 10000);
    m_encoderPPR->setValue(1000);
    encoderGridLayout->addWidget(m_encoderPPR, 2, 1);
    
    encoderLayout->addWidget(m_encoderGroup);
    encoderLayout->addStretch();
    
    // Button layout
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton* okButton = new QPushButton("OK");
    QPushButton* cancelButton = new QPushButton("Cancel");
    
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(m_tareButton, &QPushButton::clicked, this, &CalibrationDialog::calibrateLoadCellZero);
    connect(m_calibrateButton, &QPushButton::clicked, this, &CalibrationDialog::calibrateLoadCellScale);
    connect(m_potMinButton, &QPushButton::clicked, this, &CalibrationDialog::calibratePotentiometerMin);
    connect(m_potMaxButton, &QPushButton::clicked, this, &CalibrationDialog::calibratePotentiometerMax);
    connect(m_encoderResetButton, &QPushButton::clicked, this, &CalibrationDialog::resetEncoder);
    
    connect(okButton, &QPushButton::clicked, this, &CalibrationDialog::finishCalibration);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void CalibrationDialog::onSensorDataReceived(const SensorData& data)
{
    m_lastData = data;
}

void CalibrationDialog::updateLiveReadings()
{
    updateDisplays(m_lastData);
}

void CalibrationDialog::updateDisplays(const SensorData& data)
{
    // Update load cell displays
    m_loadCellRaw->setText(QString::number(data.force, 'f', 0));
    m_loadCellCalibrated->setText(QString("%1 kg").arg(data.force, 0, 'f', 2));
    
    // Update potentiometer displays  
    // Note: We'd need raw ADC value from Arduino for proper calibration
    // For now, show calculated position
    m_potRaw->setText(QString::number((int)(data.position * 1023.0 / 75.0)));
    m_potCalibrated->setText(QString("%1 mm").arg(data.position, 0, 'f', 2));
    
    // Update encoder display
    m_encoderCount->setText(QString::number(data.encoderPulses));
}

void CalibrationDialog::calibrateLoadCellZero()
{
    if (!m_serialComm->isConnected()) {
        QMessageBox::warning(this, "Error", "Not connected to Arduino");
        return;
    }
    
    m_serialComm->tareLoadCell();
    QMessageBox::information(this, "Success", "Load cell zeroed");
}

void CalibrationDialog::calibrateLoadCellScale()
{
    if (!m_serialComm->isConnected()) {
        QMessageBox::warning(this, "Error", "Not connected to Arduino");
        return;
    }
    
    double knownWeight = m_knownWeight->value();
    if (knownWeight <= 0) {
        QMessageBox::warning(this, "Error", "Please enter a valid known weight");
        return;
    }
    
    // Calculate calibration factor based on current reading and known weight
    double currentReading = m_lastData.force;
    if (currentReading > 0) {
        double calibrationFactor = knownWeight / currentReading;
        m_loadCellScale = calibrationFactor;
        m_calibrationFactor->setText(QString::number(calibrationFactor, 'f', 4));
        
        // Send to Arduino
        m_serialComm->setLoadCellCalibration(calibrationFactor);
        
        QMessageBox::information(this, "Success", 
            QString("Load cell calibrated with factor: %1").arg(calibrationFactor, 0, 'f', 4));
    } else {
        QMessageBox::warning(this, "Error", "No load detected. Please apply known weight first.");
    }
}

void CalibrationDialog::calibratePotentiometerMin()
{
    // In a real implementation, we'd get the raw ADC value from Arduino
    // For now, we'll calculate based on current position
    int rawValue = (int)(m_lastData.position * 1023.0 / m_strokeLength->value());
    m_potMin = rawValue;
    m_potMinValue->setValue(rawValue);
    
    QMessageBox::information(this, "Success", 
        QString("Minimum position set: %1").arg(rawValue));
}

void CalibrationDialog::calibratePotentiometerMax()
{
    // In a real implementation, we'd get the raw ADC value from Arduino
    // For now, we'll calculate based on current position
    int rawValue = (int)(m_lastData.position * 1023.0 / m_strokeLength->value());
    m_potMax = rawValue;
    m_potMaxValue->setValue(rawValue);
    
    QMessageBox::information(this, "Success", 
        QString("Maximum position set: %1").arg(rawValue));
}

void CalibrationDialog::resetEncoder()
{
    if (!m_serialComm->isConnected()) {
        QMessageBox::warning(this, "Error", "Not connected to Arduino");
        return;
    }
    
    m_serialComm->resetEncoder();
    QMessageBox::information(this, "Success", "Encoder reset to zero");
}

void CalibrationDialog::startCalibration()
{
    m_isCalibrating = true;
    m_calibrationStep = 0;
}

void CalibrationDialog::finishCalibration()
{
    saveCalibrationSettings();
    accept();
}

void CalibrationDialog::saveCalibrationSettings()
{
    QSettings settings;
    settings.beginGroup("Calibration");
    
    settings.setValue("loadCellScale", m_loadCellScale);
    settings.setValue("potMin", m_potMin);
    settings.setValue("potMax", m_potMax);
    settings.setValue("strokeLength", m_strokeLength->value());
    settings.setValue("encoderPPR", m_encoderPPR->value());
    
    settings.endGroup();
}

void CalibrationDialog::loadCalibrationSettings()
{
    QSettings settings;
    settings.beginGroup("Calibration");
    
    m_loadCellScale = settings.value("loadCellScale", 1.0).toDouble();
    m_potMin = settings.value("potMin", 0).toInt();
    m_potMax = settings.value("potMax", 1023).toInt();
    m_strokeLengthMm = settings.value("strokeLength", 75.0).toDouble();
    m_encoderPulsesPerRev = settings.value("encoderPPR", 1000).toInt();
    
    // Update UI
    m_calibrationFactor->setText(QString::number(m_loadCellScale, 'f', 4));
    m_potMinValue->setValue(m_potMin);
    m_potMaxValue->setValue(m_potMax);
    m_strokeLength->setValue(m_strokeLengthMm);
    m_encoderPPR->setValue(m_encoderPulsesPerRev);
    
    settings.endGroup();
}