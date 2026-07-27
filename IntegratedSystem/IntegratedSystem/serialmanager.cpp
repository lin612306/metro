#include "serialmanager.h"

#include <QDateTime>

SerialManager *SerialManager::instance()
{
    static SerialManager manager;
    return &manager;
}

SerialManager::SerialManager(QObject *parent)
    : QObject(parent)
{
    connect(&m_serial, &QSerialPort::readyRead,
            this, &SerialManager::readSerialData);
    connect(&m_serial, &QSerialPort::errorOccurred,
            this, &SerialManager::handleError);
}

bool SerialManager::openPort(const QString &portName, int baudRate)
{
    if (m_serial.isOpen()) {
        if (m_serial.portName() == portName &&
            m_serial.baudRate() == baudRate) {
            emit connectionChanged(true, m_serial.portName());
            return true;
        }
        closePort();
    }

    m_serial.setPortName(portName);
    m_serial.setBaudRate(baudRate);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial.open(QIODevice::ReadWrite)) {
        emit errorMessage(m_serial.errorString());
        return false;
    }

    m_serial.setDataTerminalReady(false);
    m_serial.setRequestToSend(false);
    m_rxBuffer.clear();

    emit logMessage(QString("Serial opened: %1, %2 baud")
                    .arg(m_serial.portName())
                    .arg(baudRate));
    emit connectionChanged(true, m_serial.portName());
    return true;
}

void SerialManager::closePort()
{
    if (m_serial.isOpen()) {
        const QString closedPort = m_serial.portName();
        m_serial.close();
        m_rxBuffer.clear();
        emit logMessage(QString("Serial closed: %1").arg(closedPort));
    }
    emit connectionChanged(false, QString());
}

bool SerialManager::isOpen() const
{
    return m_serial.isOpen();
}

QString SerialManager::portName() const
{
    return m_serial.portName();
}

QString SerialManager::lastError() const
{
    return m_serial.errorString();
}

bool SerialManager::sendCommand(const QString &command)
{
    if (!m_serial.isOpen()) {
        emit errorMessage("Serial port is not open");
        return false;
    }

    QByteArray data = command.toLocal8Bit();
    if (!data.endsWith('@')) {
        data.append('@');
    }

    const qint64 written = m_serial.write(data);
    if (written != data.size()) {
        emit errorMessage(m_serial.errorString());
        return false;
    }

    emit logMessage(QString("TX -> %1").arg(QString::fromLocal8Bit(data).trimmed()));
    return true;
}

void SerialManager::readSerialData()
{
    m_rxBuffer.append(m_serial.readAll());

    while (m_rxBuffer.contains('\n')) {
        const int index = m_rxBuffer.indexOf('\n');
        QByteArray lineData = m_rxBuffer.left(index);
        m_rxBuffer.remove(0, index + 1);

        if (lineData.endsWith('\r')) {
            lineData.chop(1);
        }
        if (lineData.isEmpty()) {
            continue;
        }

        const QString line = QString::fromLocal8Bit(lineData).trimmed();
        emit logMessage(QString("RX <- %1").arg(line));
        emit frameReceived(detectPrefix(line), line);
    }
}

void SerialManager::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    if (error == QSerialPort::ResourceError) {
        emit errorMessage(m_serial.errorString());
        closePort();
        return;
    }

    emit errorMessage(m_serial.errorString());
}

QString SerialManager::detectPrefix(const QString &line) const
{
    if (line.startsWith("D0") || line.startsWith("Temp") ||
        line.startsWith("Temperature") || line.startsWith("TC1")) {
        return "D0";
    }
    if (line.startsWith("D1") || line.startsWith("TF") ||
        line.startsWith("Data")) {
        return "D1";
    }
    if (line.startsWith("D2")) {
        return "D2";
    }
    if (line.startsWith("D3") || line.startsWith("CO2") ||
        line.startsWith("CurrentPI") || line.startsWith("GetCO2") ||
        line.startsWith("SetBase")) {
        return "D3";
    }
    if (line.startsWith("InvalidCommand") || line.contains("Error")) {
        return "SYS";
    }
    return QString();
}
