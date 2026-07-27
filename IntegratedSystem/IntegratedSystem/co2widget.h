#ifndef CO2WIDGET_H
#define CO2WIDGET_H


#include <QMainWindow>
#include <QSerialPort>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QTimer>
#include <QComboBox>
#include <QDebug>
#include <QTime>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QDoubleSpinBox>
#include <QtCharts>
#include <QDialog>
#include <QScatterSeries>

class CO2CtrlWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit CO2CtrlWidget(const QString &title, QWidget *parent = nullptr);
    void updateCurrentCO2Display(long val);
    void updatePIDisplay(double p, double i);

signals:
    void sendCommandSignal(QString cmd);

private slots:
    void onSetBase();
    void onStartAutoTune();
    void onStartControl();
    void onStopControl();
    void onSetPI();

private:
    QLabel *lblCurrentCO2;
    QPushButton *btnSetBase;
    QPushButton *btnStartAT;
    QPushButton *btnStartCtrl;
    QPushButton *btnStopCtrl;

    QDoubleSpinBox *spinP;
    QDoubleSpinBox *spinI;
    QPushButton *btnSetPI;
};

class CO2Widget : public QWidget
{
    Q_OBJECT

public:
    CO2Widget(QWidget *parent = nullptr);
    ~CO2Widget();

private slots:
    void openSerialPort();
    void closeSerialPort();
    void readSerialData();
    void handleError(QSerialPort::SerialPortError error);
    void onPollingTimer();
    void handleSendCommand(QString cmd);
    void onClearLog();
    void onShowChartClicked();

private:
    void initUI();
    void initChart();
    void appendLog(const QString &text);

    QSerialPort *serial;
    QTimer *pollingTimer;
    CO2CtrlWidget *co2Widget;
    QComboBox *portNameCombo;
    QComboBox *baudRateCombo;
    QPushButton *connectBtn;
    QPushButton *disconnectBtn;
    QPlainTextEdit *logEditor;

    QChartView *chartViewCO2;
    QChart *chartCO2;
    QLineSeries *seriesCO2;
    QDateTimeAxis *axisXCO2;
    QValueAxis *axisYCO2;

    qint64 m_startTimeMs;
    QPushButton *btnShowChart;
    QDialog *chartDialog;
    QScatterSeries *labelSeriesCO2;
    QLineSeries *seriesTargetCO2;
};


#endif // CO2WIDGET_H
