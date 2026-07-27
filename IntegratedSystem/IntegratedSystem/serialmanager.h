#ifndef SERIALMANAGER_H
#define SERIALMANAGER_H

#include <QObject>
#include <QSerialPort>

class SerialManager : public QObject
{
    Q_OBJECT

public:
    static SerialManager *instance();

    bool openPort(const QString &portName, int baudRate);
    void closePort();
    bool isOpen() const;
    QString portName() const;
    QString lastError() const;

    bool sendCommand(const QString &command);

signals:
    void connectionChanged(bool connected, const QString &portName);
    void frameReceived(const QString &prefix, const QString &line);
    void logMessage(const QString &message);
    void errorMessage(const QString &message);

private slots:
    void readSerialData();
    void handleError(QSerialPort::SerialPortError error);

private:
    explicit SerialManager(QObject *parent = nullptr);
    QString detectPrefix(const QString &line) const;

    QSerialPort m_serial;
    QByteArray m_rxBuffer;
};

#endif // SERIALMANAGER_H
