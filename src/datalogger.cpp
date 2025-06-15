#include "datalogger.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTextStream>
#include <QDebug>
#include <QDateTime>

DataLogger::DataLogger(QObject *parent)
    : QObject(parent)
    , m_isRecording(false)
{
    // Create sessions directory
    m_sessionsDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/sessions";
    QDir dir;
    if (!dir.exists(m_sessionsDir)) {
        dir.mkpath(m_sessionsDir);
    }
}

void DataLogger::startNewSession(const QString& name)
{
    m_currentSession = Session();
    m_currentSession.name = name.isEmpty() ? generateSessionFilename() : name;
    m_currentSession.timestamp = QDateTime::currentDateTime();
    m_isRecording = true;
    
    emit sessionStarted(m_currentSession.name);
}

void DataLogger::endSession()
{
    m_isRecording = false;
    emit sessionEnded();
}

void DataLogger::addDataPoint(const SensorData& data)
{
    if (m_isRecording) {
        m_currentSession.data.append(data);
        emit dataPointAdded(data);
    }
}

void DataLogger::clearCurrentSession()
{
    m_currentSession = Session();
}

bool DataLogger::saveSession(const Session& session, const QString& filename)
{
    QString filepath = filename;
    if (filepath.isEmpty()) {
        filepath = m_sessionsDir + "/" + generateSessionFilename(session.name) + ".json";
    }
    
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filepath;
        return false;
    }
    
    QJsonObject json = sessionToJson(session);
    QJsonDocument doc(json);
    
    file.write(doc.toJson());
    file.close();
    
    return true;
}

Session DataLogger::loadSession(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for reading:" << filename;
        return Session();
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << error.errorString();
        return Session();
    }
    
    return sessionFromJson(doc.object());
}

QStringList DataLogger::getAvailableSessions()
{
    QDir dir(m_sessionsDir);
    QStringList filters;
    filters << "*.json";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);
    QStringList sessions;
    
    for (const QFileInfo& info : files) {
        sessions << info.absoluteFilePath();
    }
    
    return sessions;
}

QString DataLogger::getSessionsDirectory()
{
    return m_sessionsDir;
}

bool DataLogger::exportToCsv(const Session& session, const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open CSV file for writing:" << filename;
        return false;
    }
    
    QTextStream stream(&file);
    
    // Write header
    stream << "timestamp,position_mm,force_kg,encoder_pulses,velocity_mm_s\n";
    
    // Write data
    for (const SensorData& data : session.data) {
        stream << data.timestamp << ","
               << data.position << ","
               << data.force << ","
               << data.encoderPulses << ","
               << data.velocity << "\n";
    }
    
    file.close();
    return true;
}

bool DataLogger::exportToExcel(const Session& session, const QString& filename)
{
    // For now, export as CSV with .xlsx extension
    // A full Excel implementation would require additional libraries
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open Excel file for writing:" << filename;
        return false;
    }
    
    QTextStream stream(&file);
    
    // Write header with tab separation for Excel compatibility
    stream << "Timestamp\tPosition (mm)\tForce (kg)\tEncoder (pulses)\tVelocity (mm/s)\n";
    
    // Write data
    for (const SensorData& data : session.data) {
        stream << data.timestamp << "\t"
               << data.position << "\t"
               << data.force << "\t"
               << data.encoderPulses << "\t"
               << data.velocity << "\n";
    }
    
    file.close();
    return true;
}

double DataLogger::calculateMaxForce(const Session& session)
{
    double maxForce = 0.0;
    for (const SensorData& data : session.data) {
        maxForce = qMax(maxForce, qAbs(data.force));
    }
    return maxForce;
}

double DataLogger::calculateMaxVelocity(const Session& session)
{
    double maxVelocity = 0.0;
    for (const SensorData& data : session.data) {
        maxVelocity = qMax(maxVelocity, qAbs(data.velocity));
    }
    return maxVelocity;
}

double DataLogger::calculateStrokeLength(const Session& session)
{
    if (session.data.isEmpty()) return 0.0;
    
    double minPosition = session.data.first().position;
    double maxPosition = session.data.first().position;
    
    for (const SensorData& data : session.data) {
        minPosition = qMin(minPosition, data.position);
        maxPosition = qMax(maxPosition, data.position);
    }
    
    return maxPosition - minPosition;
}

QVector<QPointF> DataLogger::getForceVsPositionCurve(const Session& session)
{
    QVector<QPointF> curve;
    for (const SensorData& data : session.data) {
        curve.append(QPointF(data.position, data.force));
    }
    return curve;
}

QVector<QPointF> DataLogger::getVelocityVsTimeCurve(const Session& session)
{
    QVector<QPointF> curve;
    for (const SensorData& data : session.data) {
        curve.append(QPointF(data.timestamp / 1000.0, data.velocity));
    }
    return curve;
}

void DataLogger::setSessionMetadata(const QString& strutInfo, double springRate, 
                                   double dampingSetting, const QString& testConditions)
{
    m_currentSession.strut_info = strutInfo;
    m_currentSession.spring_rate = springRate;
    m_currentSession.damping_setting = dampingSetting;
    m_currentSession.test_conditions = testConditions;
}

QString DataLogger::generateSessionFilename(const QString& baseName)
{
    QString base = baseName.isEmpty() ? "session" : baseName;
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    return QString("%1_%2").arg(base).arg(timestamp);
}

QJsonObject DataLogger::sessionToJson(const Session& session)
{
    QJsonObject json;
    json["name"] = session.name;
    json["description"] = session.description;
    json["timestamp"] = session.timestamp.toString(Qt::ISODate);
    json["strut_info"] = session.strut_info;
    json["spring_rate"] = session.spring_rate;
    json["damping_setting"] = session.damping_setting;
    json["test_conditions"] = session.test_conditions;
    
    QJsonArray dataArray;
    for (const SensorData& data : session.data) {
        dataArray.append(sensorDataToJson(data));
    }
    json["data"] = dataArray;
    
    return json;
}

Session DataLogger::sessionFromJson(const QJsonObject& json)
{
    Session session;
    session.name = json["name"].toString();
    session.description = json["description"].toString();
    session.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
    session.strut_info = json["strut_info"].toString();
    session.spring_rate = json["spring_rate"].toDouble();
    session.damping_setting = json["damping_setting"].toDouble();
    session.test_conditions = json["test_conditions"].toString();
    
    QJsonArray dataArray = json["data"].toArray();
    for (const QJsonValue& value : dataArray) {
        session.data.append(sensorDataFromJson(value.toObject()));
    }
    
    return session;
}

QJsonObject DataLogger::sensorDataToJson(const SensorData& data)
{
    QJsonObject json;
    json["timestamp"] = static_cast<qint64>(data.timestamp);
    json["position"] = data.position;
    json["force"] = data.force;
    json["encoder_pulses"] = static_cast<qint64>(data.encoderPulses);
    json["velocity"] = data.velocity;
    return json;
}

SensorData DataLogger::sensorDataFromJson(const QJsonObject& json)
{
    SensorData data;
    data.timestamp = json["timestamp"].toVariant().toLongLong();
    data.position = json["position"].toDouble();
    data.force = json["force"].toDouble();
    data.encoderPulses = json["encoder_pulses"].toVariant().toLongLong();
    data.velocity = json["velocity"].toDouble();
    return data;
}