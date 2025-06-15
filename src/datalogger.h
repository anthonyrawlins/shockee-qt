#ifndef DATALOGGER_H
#define DATALOGGER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

#include "serialcommunicator.h"

struct Session {
    QString name;
    QString description;
    QDateTime timestamp;
    QVector<SensorData> data;
    
    // Metadata
    QString strut_info;
    double spring_rate;
    double damping_setting;
    QString test_conditions;
    
    Session() : spring_rate(0), damping_setting(0) {}
};

class DataLogger : public QObject
{
    Q_OBJECT

public:
    explicit DataLogger(QObject *parent = nullptr);
    
    // Session management
    void startNewSession(const QString& name = "");
    void endSession();
    void addDataPoint(const SensorData& data);
    void clearCurrentSession();
    
    // File operations
    bool saveSession(const Session& session, const QString& filename = "");
    Session loadSession(const QString& filename);
    QStringList getAvailableSessions();
    QString getSessionsDirectory();
    
    // Export functions
    bool exportToCsv(const Session& session, const QString& filename);
    bool exportToExcel(const Session& session, const QString& filename);
    
    // Analysis functions
    double calculateMaxForce(const Session& session);
    double calculateMaxVelocity(const Session& session);
    double calculateStrokeLength(const Session& session);
    QVector<QPointF> getForceVsPositionCurve(const Session& session);
    QVector<QPointF> getVelocityVsTimeCurve(const Session& session);
    
    // Current session access
    const Session& getCurrentSession() const { return m_currentSession; }
    bool isRecording() const { return m_isRecording; }
    
    // Session metadata
    void setSessionMetadata(const QString& strutInfo, double springRate, 
                           double dampingSetting, const QString& testConditions);

signals:
    void sessionStarted(const QString& sessionName);
    void sessionEnded();
    void dataPointAdded(const SensorData& data);

private:
    QString generateSessionFilename(const QString& baseName = "");
    QJsonObject sessionToJson(const Session& session);
    Session sessionFromJson(const QJsonObject& json);
    QJsonObject sensorDataToJson(const SensorData& data);
    SensorData sensorDataFromJson(const QJsonObject& json);
    
    Session m_currentSession;
    bool m_isRecording;
    QString m_sessionsDir;
};

#endif // DATALOGGER_H