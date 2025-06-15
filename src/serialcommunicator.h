#ifndef SERIALCOMMUNICATOR_H
#define SERIALCOMMUNICATOR_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QByteArray>
#include <QStringList>

struct SensorData {
    qint64 timestamp;
    double position;    // mm
    double force;       // kg
    long encoderPulses;
    double velocity;    // mm/s (calculated)
    
    SensorData() : timestamp(0), position(0), force(0), encoderPulses(0), velocity(0) {}
};

class SerialCommunicator : public QObject
{
    Q_OBJECT

public:
    explicit SerialCommunicator(QObject *parent = nullptr);
    ~SerialCommunicator();
    
    QStringList getAvailablePorts();
    bool connectToPort(const QString& portName, int baudRate = 9600);
    void disconnect();
    bool isConnected() const;
    
    void sendCommand(const QString& command);
    void tareLoadCell();
    void resetEncoder();
    void setLoadCellCalibration(double calibration);

signals:
    void dataReceived(const SensorData& data);
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString& error);

private slots:
    void readData();
    void handleError(QSerialPort::SerialPortError error);

private:
    void processDataLine(const QString& line);
    SensorData parseDataLine(const QString& line);
    void calculateVelocity(SensorData& data);
    
    QSerialPort* m_serialPort;
    QByteArray m_dataBuffer;
    
    // For velocity calculation
    double m_lastPosition;
    qint64 m_lastTimestamp;
    bool m_hasLastPosition;
    
    // Moving average filter for velocity
    QList<double> m_velocityHistory;
    static const int VELOCITY_HISTORY_SIZE = 5;
};

#endif // SERIALCOMMUNICATOR_H