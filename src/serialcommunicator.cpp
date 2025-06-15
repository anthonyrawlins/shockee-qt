#include "serialcommunicator.h"
#include <QDebug>

SerialCommunicator::SerialCommunicator(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
    , m_lastPosition(0)
    , m_lastTimestamp(0)
    , m_hasLastPosition(false)
{
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialCommunicator::readData);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialCommunicator::handleError);
}

SerialCommunicator::~SerialCommunicator()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }
}

QStringList SerialCommunicator::getAvailablePorts()
{
    QStringList ports;
    const auto serialPorts = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : serialPorts) {
        ports << info.portName();
    }
    return ports;
}

bool SerialCommunicator::connectToPort(const QString& portName, int baudRate)
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
    }
    
    m_serialPort->setPortName(portName);
    m_serialPort->setBaudRate(baudRate);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
    
    if (m_serialPort->open(QIODevice::ReadWrite)) {
        m_dataBuffer.clear();
        m_hasLastPosition = false;
        m_velocityHistory.clear();
        emit connectionStatusChanged(true);
        return true;
    }
    
    emit errorOccurred("Failed to open serial port: " + m_serialPort->errorString());
    return false;
}

void SerialCommunicator::disconnect()
{
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        emit connectionStatusChanged(false);
    }
}

bool SerialCommunicator::isConnected() const
{
    return m_serialPort->isOpen();
}

void SerialCommunicator::sendCommand(const QString& command)
{
    if (m_serialPort->isOpen()) {
        m_serialPort->write(command.toUtf8() + "\n");
    }
}

void SerialCommunicator::tareLoadCell()
{
    sendCommand("TARE");
}

void SerialCommunicator::resetEncoder()
{
    sendCommand("RESET_ENCODER");
}

void SerialCommunicator::setLoadCellCalibration(double calibration)
{
    sendCommand(QString("CAL_LOAD:%1").arg(calibration));
}

void SerialCommunicator::readData()
{
    QByteArray data = m_serialPort->readAll();
    m_dataBuffer.append(data);
    
    // Process complete lines
    while (m_dataBuffer.contains('\n')) {
        int newlineIndex = m_dataBuffer.indexOf('\n');
        QByteArray line = m_dataBuffer.left(newlineIndex);
        m_dataBuffer.remove(0, newlineIndex + 1);
        
        QString lineStr = QString::fromUtf8(line).trimmed();
        if (!lineStr.isEmpty() && !lineStr.startsWith('#')) {
            processDataLine(lineStr);
        }
    }
}

void SerialCommunicator::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError) {
        emit errorOccurred("Serial port error: " + m_serialPort->errorString());
        emit connectionStatusChanged(false);
    }
}

void SerialCommunicator::processDataLine(const QString& line)
{
    SensorData data = parseDataLine(line);
    if (data.timestamp > 0) {
        calculateVelocity(data);
        emit dataReceived(data);
    }
}

SensorData SerialCommunicator::parseDataLine(const QString& line)
{
    SensorData data;
    QStringList parts = line.split(',');
    
    if (parts.size() >= 4) {
        bool ok;
        data.timestamp = parts[0].toLongLong(&ok);
        if (!ok) return SensorData();
        
        data.position = parts[1].toDouble(&ok);
        if (!ok) return SensorData();
        
        data.force = parts[2].toDouble(&ok);
        if (!ok) return SensorData();
        
        data.encoderPulses = parts[3].toLong(&ok);
        if (!ok) return SensorData();
    }
    
    return data;
}

void SerialCommunicator::calculateVelocity(SensorData& data)
{
    if (m_hasLastPosition && data.timestamp > m_lastTimestamp) {
        double deltaTime = (data.timestamp - m_lastTimestamp) / 1000.0; // Convert to seconds
        double deltaPosition = data.position - m_lastPosition;
        double instantVelocity = deltaPosition / deltaTime;
        
        // Apply moving average filter
        m_velocityHistory.append(instantVelocity);
        if (m_velocityHistory.size() > VELOCITY_HISTORY_SIZE) {
            m_velocityHistory.removeFirst();
        }
        
        // Calculate average velocity
        double sum = 0;
        for (double v : m_velocityHistory) {
            sum += v;
        }
        data.velocity = sum / m_velocityHistory.size();
    }
    
    m_lastPosition = data.position;
    m_lastTimestamp = data.timestamp;
    m_hasLastPosition = true;
}