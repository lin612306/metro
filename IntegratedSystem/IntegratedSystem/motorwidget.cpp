#include "motorwidget.h"
#include <QSerialPortInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>

MotorWidget::MotorWidget(QWidget *parent)
    : QWidget(parent), serial(new QSerialPort(this))
{
    initUI();

    connect(serial, &QSerialPort::readyRead, this, &MotorWidget::readSerialData);
    connect(serial, &QSerialPort::errorOccurred, this, &MotorWidget::handleError);
}

MotorWidget::~MotorWidget()
{
    if (serial->isOpen())
        serial->close();
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
    ctrls.btnSetFreq = btnSetFlowAndFreq;
    m_motorWidgets.insert(motorId, ctrls);

    // 绑定事件
    connect(btnSetDir, &QPushButton::clicked, this, [=](){ onSetDirClicked(motorId); });

    // 流量改变时，实时更新频率和剪切力显示
    connect(flowSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [=](double val){ onFlowValueChanged(motorId, val); });

    // 点击应用流量
    connect(btnSetFlowAndFreq, &QPushButton::clicked, this, [=](){ onSetFreqClicked(motorId); });

    connect(btnStart, &QPushButton::clicked, this, [=](){ onStartClicked(motorId); });
    connect(btnStop, &QPushButton::clicked, this, [=](){ onStopClicked(motorId); });

    // 初始化一次计算值
    onFlowValueChanged(motorId, flowSpin->value());
}

// --- 核心算法：体积流量转频率 ---
int MotorWidget::calculateFreqFromFlow(double flow)
{
    // 实验平均数据点：
    // P1: (0.1579, 3000)
    // P2: (0.3111, 6000)
    // P3: (0.4882, 9000)
    double freq = 3000;

    if (flow <= 0.3111) {
        // 第一段插值: 0.1579 ~ 0.3111 对应 3000 ~ 6000
        double k1 = (6000.0 - 3000.0) / (0.3111 - 0.1579);
        freq = 3000.0 + (flow - 0.1579) * k1;
    } else {
        // 第二段插值及外推: 大于0.3111 对应 6000 ~ 60000
        double k2 = (9000.0 - 6000.0) / (0.4882 - 0.3111);
        freq = 6000.0 + (flow - 0.3111) * k2;
    }

    // 限制范围
    if (freq < 3000) freq = 3000;
    if (freq > 60000) freq = 60000;

    return static_cast<int>(freq);
}

// --- 流量值变化处理 ---
void MotorWidget::onFlowValueChanged(int motorId, double flowVal)
{
    if (!m_motorWidgets.contains(motorId)) return;

    // 1. 计算转换后的频率
    int calcFreq = calculateFreqFromFlow(flowVal);

    // 2. 计算剪切力 (由淋巴液参数及 250μm 方形流道推导: tau ≈ 2.1333 * Q)
    double calcShear = 2.1333 * flowVal;

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
    // 定义指令之间的时间间隔 (单位：毫秒)
    const int delayMs = 150;

    logConsole->append(">>> 开始执行一键启动序列...");

    // 使用 QTimer::singleShot 制造“排队”效果
    for(int i=1; i<=4; i++) {
        QTimer::singleShot((i-1) * delayMs, this, [=](){
            onStartClicked(i);
        });
    }
}

void MotorWidget::onGlobalStopClicked()
{
    const int delayMs = 150; // 同样使用 150ms 间隔

    logConsole->append(">>> 开始执行一键停止序列...");

    for(int i=1; i<=4; i++) {
        QTimer::singleShot((i-1) * delayMs, this, [=](){
            onStopClicked(i);
        });
    }
}


void MotorWidget::openSerialPort()
{
    serial->setPortName(portNameCombo->currentText());
    serial->setBaudRate(baudRateCombo->currentText().toInt());
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (serial->open(QIODevice::ReadWrite)) {
        serial->setDataTerminalReady(false);
        serial->setRequestToSend(false);

        connectBtn->setEnabled(false);
        disconnectBtn->setEnabled(true);
        portNameCombo->setEnabled(false);

        //  连接时可以启用全局按钮
        globalStartBtn->setEnabled(true);
        globalStopBtn->setEnabled(true);

        logConsole->append(QString("[%1] 已连接到 %2").arg(QTime::currentTime().toString("HH:mm:ss")).arg(serial->portName()));
    } else {
        QMessageBox::critical(this, "连接错误", serial->errorString());
    }
}

void MotorWidget::closeSerialPort()
{
    if (serial->isOpen())
        serial->close();
    connectBtn->setEnabled(true);
    disconnectBtn->setEnabled(false);
    portNameCombo->setEnabled(true);

    // 断开时禁用全局按钮
    globalStartBtn->setEnabled(false);
    globalStopBtn->setEnabled(false);

    logConsole->append(QString("[%1] 已断开连接").arg(QTime::currentTime().toString("HH:mm:ss")));
}

void MotorWidget::sendCommand(const QString &cmd)
{
    if (serial->isOpen()) {
        QByteArray data = cmd.toLocal8Bit();
        serial->write(data);
        logConsole->append(QString("发送 >> %1").arg(cmd.trimmed()));
    } else {
        QMessageBox::warning(this, "警告", "串口未连接，请先连接！");
    }
}

void MotorWidget::readSerialData()
{
    // 1. 把新收到的数据追加到缓冲区屁股后面
    QByteArray data = serial->readAll();
    m_buffer.append(data);

    // 2. 检查缓冲区里有没有“换行符” (\n)
    while (m_buffer.contains('\n')) {

        // 找到换行符的位置
        int index = m_buffer.indexOf('\n');

        // 把这一句从缓冲区里“挖”出来 (left取左边部分)
        QByteArray line = m_buffer.left(index);

        // 这一句已经被挖出来了，从缓冲区里彻底删掉 (包括那个换行符 index+1)
        m_buffer.remove(0, index + 1);

        // 处理一下：去掉可能存在的“回车符” (\r)
        if (line.endsWith('\r')) {
            line.chop(1);
        }

        // 如果这一行不是空的，就显示出来
        if (!line.isEmpty()) {
            QString msg = QString::fromLocal8Bit(line);
            logConsole->append(QString("接收 << %1").arg(msg));
        }
    }
}

void MotorWidget::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, "严重错误", serial->errorString());
        closeSerialPort();
    }
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
    // 下发计算后的频率给硬件
    int freqVal = m_motorWidgets[motorId].freqSpin->value();
    QString cmd = QString("D2freq%1:%2@").arg(motorId).arg(freqVal);
    sendCommand(cmd);
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
