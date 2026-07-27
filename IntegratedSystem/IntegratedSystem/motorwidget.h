#ifndef MOTORWIDGET_H
#define MOTORWIDGET_H

#include <QMainWindow>
#include <QGroupBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include <QGridLayout>
#include <QTextEdit>
#include <QTimer>

class MotorWidget : public QWidget
{
    Q_OBJECT

public:
    MotorWidget(QWidget *parent = nullptr);
    ~MotorWidget();

private slots:
    // 串口相关槽函数
    void openSerialPort();
    void closeSerialPort();
    void handleSerialFrame(const QString &prefix, const QString &line);
    void handleSerialState(bool connected, const QString &portName);
    void handleSerialError(const QString &message);

    // 全局控制槽函数
    void onGlobalStartClicked();
    void onGlobalStopClicked();

    // 流量变化处理槽函数
    void onFlowValueChanged(int motorId, double flowVal);

    // 电机控制槽函数
    void onSetDirClicked(int motorId);
    void onSetSubClicked(int motorId);
    void onSetFreqClicked(int motorId);
    void onStartClicked(int motorId);
    void onStopClicked(int motorId);

private:
    void initUI();
    void createMotorPanel(int motorId, QWidget *parent, QGridLayout *mainLayout, int row, int col);
    void sendCommand(const QString &cmd);

    // 频率计算辅助函数
    int calculateFreqFromFlow(int motorId, double flow);

    struct MotorControls {
        QComboBox *dirCombo;
        QComboBox *subCombo;
        QSpinBox *freqSpin;
        QDoubleSpinBox *flowSpin;
        QDoubleSpinBox *shearSpin;
        QDoubleSpinBox *shearKSpin;
        QDoubleSpinBox *flowKSpin;
        QDoubleSpinBox *flowBSpin;
        QPushButton *btnSetFreq;  // 保存引用以便改名或操作
    };
    QMap<int, MotorControls> m_motorWidgets;

    // 串口控件
    QComboBox *portNameCombo;
    QComboBox *baudRateCombo;
    QPushButton *connectBtn;
    QPushButton *disconnectBtn;

    // 全局控制按钮
    QPushButton *globalStartBtn;
    QPushButton *globalStopBtn;

    QTextEdit *logConsole;
    QPushButton *clearLogBtn;
};

#endif // MOTORWIDGET_H
