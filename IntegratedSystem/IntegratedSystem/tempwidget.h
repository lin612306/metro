#ifndef TEMPWIDGET_H
#define TEMPWIDGET_H


#include <QMainWindow>
#include <QGroupBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QLayout>
#include <QTimer>
#include <QComboBox>
#include <QDebug>
#include <QTime>
#include <QPlainTextEdit>
#include <QtCharts>
#include <QDialog>
#include <QScatterSeries>
#include <QDateTimeAxis>
#include <QValueAxis>


// ==========================================
// 自定义控件类：温度控制面板 (保持不变)
// ==========================================
class TempCtrlWidget : public QGroupBox
{
    Q_OBJECT
public:
    explicit TempCtrlWidget(const QString &title, const QString &prefix, QWidget *parent = nullptr);
    double getP() const;
    double getI() const;
    double getD() const;
    double getTargetTemp() const;
    void updateCurrentTempDisplay(double val);

signals:
    void sendCommandSignal(QString cmd);
    void showChartSignal();

private slots:
    void onSetPID();
    void onSetTemp();
    void onStartCtrl();
    void onStopCtrl();
    void onStartAutoTune();

private:
    QString m_prefix;
    QDoubleSpinBox *spinP;
    QDoubleSpinBox *spinI;
    QDoubleSpinBox *spinD;
    QDoubleSpinBox *spinTargetTemp;
    QLabel *lblCurrentTemp;
    QPushButton *btnSetPID;
    QPushButton *btnSetTemp;
    QPushButton *btnShowChart;
    QPushButton *btnStartCtrl;
    QPushButton *btnStopCtrl;
    QPushButton *btnStartAT;
};

// ==========================================
// 主窗口类
// ==========================================
class TempWidget : public QWidget
{
    Q_OBJECT

public:
    TempWidget(QWidget *parent = nullptr);
    ~TempWidget();

private slots:
    void openSerialPort();
    void closeSerialPort();
    void handleSerialFrame(const QString &prefix, const QString &line);
    void handleSerialState(bool connected, const QString &portName);
    void handleSerialError(const QString &message);
    void onPollingTimer();
    void handleSendCommand(QString cmd);
    void onClearLog();
    void onShowIntChartClicked();
    void onShowExtChartClicked();

private:
    void initUI();
    void initCharts();
    void appendLog(const QString &text);

    QTimer *pollingTimer;
    TempCtrlWidget *internalTempWidget;
    TempCtrlWidget *externalTempWidget;
    QComboBox *portNameCombo;
    QComboBox *baudRateCombo;
    QPushButton *connectBtn;
    QPushButton *disconnectBtn;
    int m_pollingState;
    bool m_requestedOpen;

    //  日志显示框
    QPlainTextEdit *logEditor;

    // --- 图表 1: 内部温度 (D0) ---
    QChartView *chartViewInt;
    QChart *chartInt;
    QLineSeries *seriesInt;
    QDateTimeAxis *axisXInt;
    QValueAxis *axisYInt;

    // --- 图表 2: 外部温度 (D1) ---
    QChartView *chartViewExt;
    QChart *chartExt;
    QLineSeries *seriesExt;
    QDateTimeAxis *axisXExt;
    QValueAxis *axisYExt;

    qint64 m_startTimeMs;
    QDialog *chartDialogInt;
    QDialog *chartDialogExt;

    QLineSeries *seriesTargetInt;
    QLineSeries *seriesTargetExt;

    QScatterSeries *labelSeriesInt;
    QScatterSeries *labelSeriesExt;
};

#endif // TEMPWIDGET_H
