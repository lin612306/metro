#include "serialmanager.h"

#include <QDateTime>

/*
 * SerialManager 是上位机唯一的串口入口。
 *
 * 论文硬件结构中, Qt 只通过一根串口线连接 STM32 的 USART1。温度、电机、
 * CO2 页面虽然是不同界面, 但不能各自抢占同一个串口。因此这里集中处理:
 * 1. 打开和关闭串口。
 * 2. 发送完整命令帧, 例如 D2flow1:0.5000@。
 * 3. 按换行拆分 STM32 返回文本。
 * 4. 根据 D0/D1/D2/D3 前缀把数据分发给对应页面。
 */
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
    // 如果串口已经按相同参数打开, 直接复用, 避免重复打开导致端口状态抖动。
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
    // 所有页面都通过这里发送命令, 这样 TX 日志和错误处理是统一的。
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
    // STM32 使用 \r\n 作为一帧文本结束。这里先缓存半包, 读到换行后再分发。
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
    // 兼容旧版返回文本: 有些温控器或 CO2 函数没有严格以 D0/D1/D3 开头。
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
