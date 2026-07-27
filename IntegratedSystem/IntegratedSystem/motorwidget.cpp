#include "motorwidget.h"
#include "serialmanager.h"
#include <QSerialPortInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>

/*
 * MotorWidget 是 D2 灌流泵控制页面。
 *
 * 上位机负责实验层面的参数输入: 第几路泵、方向、细分和目标流量。
 * STM32 负责硬件层面的 STEP/DIR/EN GPIO 与 PWM 频率输出。
 *
 * 协议对应关系:
 * D2dirN:x@       设置第 N 路方向。
 * D2subN:x@       设置第 N 路细分, 默认推荐 64。
 * D2flowcalN:k,b@ 设置第 N 路流量曲线 Hz = k * ml/h + b。
 * D2flowN:x@      按 ml/h 控制该路泵。
 * D2startall@     四路同步启动。
 * D2stopall@      四路同步停止。
 */
MotorWidget::MotorWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();

    SerialManager *manager = SerialManager::instance();
    connect(manager, &SerialManager::frameReceived,
            this, &MotorWidget::handleSerialFrame);
    connect(manager, &SerialManager::connectionChanged,
            this, &MotorWidget::handleSerialState);
    connect(manager, &SerialManager::errorMessage,
            this, &MotorWidget::handleSerialError);
    connect(manager, &SerialManager::logMessage,
            this, [this](const QString &message) { logConsole->append(message); });
}

MotorWidget::~MotorWidget()
{
}

void MotorWidget::initUI()
{
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QVBoxLayout *rightLayout = new QVBoxLayout();

    // ================= 通信设置 =================
    QGroupBox *serialGroup = new QGroupBox("通信设置");
    QHBoxLayout *serialLayout = new QHBoxLayout();

    portNameCombo = new QComboBox();
    const auto infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        portNameCombo->addItem(info.portName());
    }

    baudRateCombo = new QComboBox();
    baudRateCombo->addItem("115200");
    baudRateCombo->addItem("9600");
    baudRateCombo->setCurrentIndex(0);

    connectBtn = new QPushButton("连接");
    disconnectBtn = new QPushButton("断开");
    disconnectBtn->setEnabled(false);

    serialLayout->addWidget(new QLabel("端口:"));
    serialLayout->addWidget(portNameCombo);
    serialLayout->addWidget(new QLabel("波特率:"));
    serialLayout->addWidget(baudRateCombo);
    serialLayout->addWidget(connectBtn);
    serialLayout->addWidget(disconnectBtn);
    serialGroup->setLayout(serialLayout);

    leftLayout->addWidget(serialGroup);

    // ================= 四路电机控制区域 =================
    QGroupBox *motorAreaGroup = new QGroupBox("四路步进电机控制");
    QVBoxLayout *motorAreaLayout = new QVBoxLayout(); // 改用垂直布局包裹

    // --- 全局控制按钮栏 ---
    QHBoxLayout *globalCtrlLayout = new QHBoxLayout();
    globalStartBtn = new QPushButton("一键全部启动 (Start All)");
    globalStopBtn = new QPushButton("一键全部停止 (Stop All)");

    // 设置显眼的样式
    globalStartBtn->setStyleSheet("background-color: #00796B; color: white; font-weight: bold; height: 35px;");
    globalStopBtn->setStyleSheet("background-color: #C62828; color: white; font-weight: bold; height: 35px;");

    globalCtrlLayout->addWidget(globalStartBtn);
    globalCtrlLayout->addWidget(globalStopBtn);
    motorAreaLayout->addLayout(globalCtrlLayout); // 添加到顶部

    // 具体的电机Grid
    QGridLayout *motorGridLayout = new QGridLayout();
    createMotorPanel(1, motorAreaGroup, motorGridLayout, 0, 0);
    createMotorPanel(2, motorAreaGroup, motorGridLayout, 0, 1);
    createMotorPanel(3, motorAreaGroup, motorGridLayout, 1, 0);
    createMotorPanel(4, motorAreaGroup, motorGridLayout, 1, 1);

    motorAreaLayout->addLayout(motorGridLayout);
    motorAreaGroup->setLayout(motorAreaLayout);

    leftLayout->addWidget(motorAreaGroup);

    // ================= 日志区域 =================
    logConsole = new QTextEdit();
    logConsole->setReadOnly(true);
    logConsole->setMinimumWidth(300);

    clearLogBtn = new QPushButton("清空日志信息");
    clearLogBtn->setStyleSheet("height: 30px; font-weight: bold;");

    rightLayout->addWidget(new QLabel("系统日志:"));
    rightLayout->addWidget(logConsole);
    rightLayout->addWidget(clearLogBtn);

    rootLayout->addLayout(leftLayout);
    rootLayout->addLayout(rightLayout);

    // ================= 信号连接 =================
    connect(connectBtn, &QPushButton::clicked, this, &MotorWidget::openSerialPort);
    connect(disconnectBtn, &QPushButton::clicked, this, &MotorWidget::closeSerialPort);
    connect(clearLogBtn, &QPushButton::clicked, logConsole, &QTextEdit::clear);

    // 连接全局按钮
    connect(globalStartBtn, &QPushButton::clicked, this, &MotorWidget::onGlobalStartClicked);
    connect(globalStopBtn, &QPushButton::clicked, this, &MotorWidget::onGlobalStopClicked);

    setWindowTitle("步进电机上位机");
    resize(1100, 750); // 调大一点以适应新增的剪切力控件
}


// 辅助函数：自动生成单个电机的控制面板
void MotorWidget::createMotorPanel(int motorId, QWidget *parent, QGridLayout *mainLayout, int row, int col)
{
    Q_UNUSED(parent);

    QGroupBox *group = new QGroupBox(QString("电机 #%1").arg(motorId));
    QVBoxLayout *layout = new QVBoxLayout();

    // --- 方向 ---
    QHBoxLayout *dirLayout = new QHBoxLayout();
    QComboBox *dirBox = new QComboBox();
    dirBox->addItem("CCW (逆时针/0)", 0);
    dirBox->addItem("CW (顺时针/1)", 1);
    dirBox->setCurrentIndex(1);
    QPushButton *btnSetDir = new QPushButton("设置方向");
    dirLayout->addWidget(new QLabel("方向:"));
    dirLayout->addWidget(dirBox);
    dirLayout->addWidget(btnSetDir);

    // --- 细分  ---
    QHBoxLayout *subLayout = new QHBoxLayout();
    QComboBox *subBox = new QComboBox();
    subBox->addItem("64");
    subBox->addItem("32");
    subBox->addItem("16");
    subBox->addItem("8");
    subBox->setCurrentText("64");
    QPushButton *btnSetSub = new QPushButton("设置细分");
    subLayout->addWidget(new QLabel("细分:"));
    subLayout->addWidget(subBox);
    subLayout->addWidget(btnSetSub);

    // --- 体积流量控制 ---
    QHBoxLayout *flowLayout = new QHBoxLayout();
    QDoubleSpinBox *flowSpin = new QDoubleSpinBox();

    // 设置基于实验数据的体积流量范围：0.158 ml/h (对应约3000Hz) 到 3.500 ml/h (对应约60000Hz)
    flowSpin->setRange(0.158, 3.500);
    flowSpin->setDecimals(4);
    flowSpin->setSingleStep(0.05);
    flowSpin->setSuffix(" ml/h");
    flowSpin->setValue(0.158);

    QLabel *rangeLabel = new QLabel("(范围: 0.158 ~ 3.500)");
    rangeLabel->setStyleSheet("color: #666666; font-size: 11px; margin-left: 5px;");

    QPushButton *btnSetFlowAndFreq = new QPushButton("应用流量");

    flowLayout->addWidget(new QLabel("体积流量:"));
    flowLayout->addWidget(flowSpin);
    flowLayout->addWidget(rangeLabel);
    flowLayout->addWidget(btnSetFlowAndFreq);

    QDoubleSpinBox *flowKSpin = new QDoubleSpinBox();
    QDoubleSpinBox *flowBSpin = new QDoubleSpinBox();
    flowKSpin->setRange(1.0, 100000.0);
    flowKSpin->setDecimals(3);
    flowKSpin->setValue(18165.304);
    flowKSpin->setFixedWidth(95);
    flowBSpin->setRange(-10000.0, 10000.0);
    flowBSpin->setDecimals(3);
    flowBSpin->setValue(131.301);
    flowBSpin->setFixedWidth(90);
    QDoubleSpinBox *shearKSpin = new QDoubleSpinBox();
    shearKSpin->setRange(0.0001, 100.0);
    shearKSpin->setDecimals(4);
    shearKSpin->setValue(2.1333);
    shearKSpin->setFixedWidth(85);
    /*
     * 标定控件不显示在实验操作界面上。
     * 下位机仍保留每路泵 k/b 标定能力, Qt 这里使用默认值保持协议一致。
     */
    flowKSpin->hide();
    flowBSpin->hide();
    shearKSpin->hide();

    // --- 剪切力计算 (只读) ---
    QHBoxLayout *shearLayout = new QHBoxLayout();
    QDoubleSpinBox *shearSpin = new QDoubleSpinBox();
    shearSpin->setRange(0.0, 100.0);
    shearSpin->setDecimals(4);
    shearSpin->setSuffix(" dyn/cm²");
    shearSpin->setReadOnly(true);
    shearSpin->setStyleSheet("background-color: #f0f0f0; color: #555;");

    shearLayout->addWidget(new QLabel("计算剪切力:"));
    shearLayout->addWidget(shearSpin);

    // --- 频率 (只读) ---
    QHBoxLayout *freqLayout = new QHBoxLayout();
    QSpinBox *freqSpin = new QSpinBox();
    freqSpin->setRange(3000, 60000);
    freqSpin->setValue(3000);
    freqSpin->setSingleStep(100);
    freqSpin->setSuffix(" Hz");
    freqSpin->setReadOnly(true);
    freqSpin->setStyleSheet("background-color: #f0f0f0; color: #555;");

    freqLayout->addWidget(new QLabel("计算频率:"));
    freqLayout->addWidget(freqSpin);

    // --- 启停 ---
    QHBoxLayout *ctrlLayout = new QHBoxLayout();
    QPushButton *btnStart = new QPushButton("启动");
    QPushButton *btnStop = new QPushButton("停止");

    btnStart->setStyleSheet("background-color: #4CAF50; color: white;");
    btnStop->setStyleSheet("background-color: #f44336; color: white;");

    ctrlLayout->addWidget(btnStart);
    ctrlLayout->addWidget(btnStop);

    // 加入布局
    layout->addLayout(dirLayout);
    layout->addLayout(subLayout);
    layout->addLayout(flowLayout);
    layout->addLayout(shearLayout); // 加入新增的剪切力布局
    layout->addLayout(freqLayout);
    layout->addLayout(ctrlLayout);
    group->setLayout(layout);

    mainLayout->addWidget(group, row, col);

    // 保存引用
    MotorControls ctrls;
    ctrls.dirCombo = dirBox;
    ctrls.subCombo = subBox;
    ctrls.freqSpin = freqSpin;
    ctrls.flowSpin = flowSpin;
    ctrls.shearSpin = shearSpin; // 保存剪切力控件引用
    ctrls.shearKSpin = shearKSpin;
    ctrls.flowKSpin = flowKSpin;
    ctrls.flowBSpin = flowBSpin;
    ctrls.btnSetFreq = btnSetFlowAndFreq;
    m_motorWidgets.insert(motorId, ctrls);

    // 绑定事件
    connect(btnSetDir, &QPushButton::clicked, this, [=](){ onSetDirClicked(motorId); });
    connect(btnSetSub, &QPushButton::clicked, this, [=](){ onSetSubClicked(motorId); });

    // 流量改变时，实时更新频率和剪切力显示
    connect(flowSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [=](double val){ onFlowValueChanged(motorId, val); });
    connect(shearKSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [=](double){ onFlowValueChanged(motorId, flowSpin->value()); });

    // 点击应用流量
    connect(btnSetFlowAndFreq, &QPushButton::clicked, this, [=](){ onSetFreqClicked(motorId); });

    connect(btnStart, &QPushButton::clicked, this, [=](){ onStartClicked(motorId); });
    connect(btnStop, &QPushButton::clicked, this, [=](){ onStopClicked(motorId); });

    // 初始化一次计算值
    onFlowValueChanged(motorId, flowSpin->value());
}

// --- 核心算法：体积流量转频率 ---
int MotorWidget::calculateFreqFromFlow(int motorId, double flow)
{
    // 每一路泵使用自己的 k,b 标定。Qt 先在界面上预览频率,
    // STM32 收到 D2flow 后还会用同一组 k,b 再计算一次, 保证上下位机语义一致。
    double k = 18165.304;
    double b = 131.301;
    double freq;

    if (m_motorWidgets.contains(motorId)) {
        k = m_motorWidgets[motorId].flowKSpin->value();
        b = m_motorWidgets[motorId].flowBSpin->value();
    }

    freq = k * flow + b;
    if (freq < 3000) freq = 3000;
    if (freq > 60000) freq = 60000;

    return static_cast<int>(freq + 0.5);
}

// --- 流量值变化处理 ---
void MotorWidget::onFlowValueChanged(int motorId, double flowVal)
{
    if (!m_motorWidgets.contains(motorId)) return;

    // 1. 计算转换后的频率
    int calcFreq = calculateFreqFromFlow(motorId, flowVal);

    // 2. 计算剪切力: 系数由真实微流道尺寸和流体黏度重新标定。
    double shearK = m_motorWidgets[motorId].shearKSpin->value();
    double calcShear = shearK * flowVal;

    // 更新界面上的频率显示
    m_motorWidgets[motorId].freqSpin->blockSignals(true);
    m_motorWidgets[motorId].freqSpin->setValue(calcFreq);
    m_motorWidgets[motorId].freqSpin->blockSignals(false);

    // 更新界面上的剪切力显示
    m_motorWidgets[motorId].shearSpin->blockSignals(true);
    m_motorWidgets[motorId].shearSpin->setValue(calcShear);
    m_motorWidgets[motorId].shearSpin->blockSignals(false);
}

// --- 全局启动 ---
void MotorWidget::onGlobalStartClicked()
{
    sendCommand("D2startall@");
}

void MotorWidget::onGlobalStopClicked()
{
    sendCommand("D2stopall@");
}


void MotorWidget::openSerialPort()
{
    SerialManager *manager = SerialManager::instance();
    if (!manager->openPort(portNameCombo->currentText(), baudRateCombo->currentText().toInt())) {
        QMessageBox::critical(this, "Connection Error", manager->lastError());
    }
}

void MotorWidget::closeSerialPort()
{
    SerialManager::instance()->closePort();
}

void MotorWidget::sendCommand(const QString &cmd)
{
    if (!SerialManager::instance()->sendCommand(cmd)) {
        QMessageBox::warning(this, "Warning", "Serial port is not open.");
    }
}

void MotorWidget::handleSerialFrame(const QString &prefix, const QString &line)
{
    // 当前电机页主要靠统一日志显示 D2OK/D2ERR。
    // 后续若要把 D2STATUS 回填到界面, 应在这里解析 line 并更新四路控件。
    Q_UNUSED(prefix);
    Q_UNUSED(line);
}

void MotorWidget::handleSerialState(bool connected, const QString &portName)
{
    connectBtn->setEnabled(!connected);
    disconnectBtn->setEnabled(connected);
    portNameCombo->setEnabled(!connected);
    globalStartBtn->setEnabled(connected);
    globalStopBtn->setEnabled(connected);

    if (connected) {
        logConsole->append(QString("[%1] Connected to %2")
                           .arg(QTime::currentTime().toString("HH:mm:ss"))
                           .arg(portName));
    } else {
        logConsole->append(QString("[%1] Disconnected")
                           .arg(QTime::currentTime().toString("HH:mm:ss")));
    }
}

void MotorWidget::handleSerialError(const QString &message)
{
    logConsole->append(QString("Error: %1").arg(message));
}

void MotorWidget::onSetDirClicked(int motorId)
{
    if(!m_motorWidgets.contains(motorId)) return;
    int dirVal = m_motorWidgets[motorId].dirCombo->currentData().toInt();
    QString cmd = QString("D2dir%1:%2@").arg(motorId).arg(dirVal);
    sendCommand(cmd);
}

void MotorWidget::onSetSubClicked(int motorId)
{
    if(!m_motorWidgets.contains(motorId)) return;
    QString subVal = m_motorWidgets[motorId].subCombo->currentText();
    QString cmd = QString("D2sub%1:%2@").arg(motorId).arg(subVal);
    sendCommand(cmd);
}

void MotorWidget::onSetFreqClicked(int motorId)
{
    if(!m_motorWidgets.contains(motorId)) return;

    // 先发标定曲线, 再发目标流量。
    // 这样 STM32 即使刚上电, 也能按当前界面参数进行流量到频率的换算。
    const MotorControls &ctrls = m_motorWidgets[motorId];
    QString calCmd = QString("D2flowcal%1:%2,%3@").arg(motorId).arg(ctrls.flowKSpin->value()).arg(ctrls.flowBSpin->value());
    QString flowCmd = QString("D2flow%1:%2@").arg(motorId).arg(ctrls.flowSpin->value(), 0, 'f', 4);
    sendCommand(calCmd);
    sendCommand(flowCmd);
}

void MotorWidget::onStartClicked(int motorId)
{
    QString cmd = QString("D2start%1@").arg(motorId);
    sendCommand(cmd);
}

void MotorWidget::onStopClicked(int motorId)
{
    QString cmd = QString("D2stop%1@").arg(motorId);
    sendCommand(cmd);
}
