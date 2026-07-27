#ifndef CO2WIDGET_H
#define CO2WIDGET_H


#include <QMainWindow>
#include <QGroupBox>
#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QGridLayout>
#include <QTimer>
#include <QComboBox>
#include <QDebug>
#include <QTime>
#include <QPlainTextEdit>
#include <QMessageBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
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
    void onSetTarget();
    void onSetAdvanced();
    void onSetCohenCoon();

private:
    QLabel *lblCurrentCO2;
    QDoubleSpinBox *spinTargetCO2;
    QPushButton *btnSetTarget;
    QPushButton *btnSetBase;
    QPushButton *btnStartAT;
    QPushButton *btnStartCtrl;
    QPushButton *btnStopCtrl;

    QDoubleSpinBox *spinP;
    QDoubleSpinBox *spinI;
    QPushButton *btnSetPI;
    QSpinBox *spinDeadband;
    QDoubleSpinBox *spinMinDuty;
    QSpinBox *spinPwmPeriod;
    QPushButton *btnSetAdvanced;
    QDoubleSpinBox *spinCCK;
    QDoubleSpinBox *spinCCL;
    QDoubleSpinBox *spinCCT;
    QPushButton *btnSetCC;
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
    void handleSerialFrame(const QString &prefix, const QString &line);
    void handleSerialState(bool connected, const QString &portName);
    void handleSerialError(const QString &message);
    void onPollingTimer();
    void handleSendCommand(QString cmd);
    void onClearLog();
    void onShowChartClicked();

private:
    void initUI();
    void initChart();
    void appendLog(const QString &text);

    QTimer *pollingTimer;
    CO2CtrlWidget *co2Widget;
    QComboBox *portNameCombo;
    QComboBox *baudRateCombo;
    QPushButton *connectBtn;
    QPushButton *disconnectBtn;
    QPlainTextEdit *logEditor;
    bool m_requestedOpen;
    double m_targetCO2;

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
