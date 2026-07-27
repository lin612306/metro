#include "co2widget.h"
#include <QSerialPortInfo>
#include <QDateTime>
#include <QScrollBar>
#include <QRegularExpression>

// ============================================================================
//  CO2CtrlWidget 实现
// ============================================================================
CO2CtrlWidget::CO2CtrlWidget(const QString &title, QWidget *parent)
    : QGroupBox(title, parent)
{
    QFont font = this->font();
    font.setPointSize(10);
    this->setFont(font);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(15, 20, 15, 15);

    // --- 第一行：当前浓度 ---
    QHBoxLayout *currLayout = new QHBoxLayout();
    QLabel *lblTitle = new QLabel("当前浓度:");
    lblCurrentCO2 = new QLabel("0 ppm");
    lblCurrentCO2->setStyleSheet("font-size: 26px; font-weight: bold; color: #28a745; font-family: Arial;");
    currLayout->addStretch();
    currLayout->addWidget(lblTitle);
    currLayout->addSpacing(10);
    currLayout->addWidget(lblCurrentCO2);
    currLayout->addStretch();

    // --- 第二行：基线校准 ---
    QHBoxLayout *baseLayout = new QHBoxLayout();
    btnSetBase = new QPushButton("启动基线校准");
    btnSetBase->setFixedHeight(30);
    baseLayout->addStretch();
    baseLayout->addWidget(btnSetBase);
    baseLayout->addStretch();

    // --- 第三行：启动自整定 ---
    QHBoxLayout *atLayout = new QHBoxLayout();
    btnStartAT = new QPushButton("启动自整定");
    btnStartAT->setFixedHeight(30);
    atLayout->addStretch();
    atLayout->addWidget(btnStartAT);
    atLayout->addStretch();

    // --- 第四行：启停控制  ---
    QHBoxLayout *btnLayout2 = new QHBoxLayout();
    btnStartCtrl = new QPushButton("启动");
    btnStopCtrl = new QPushButton("停止");
    btnStartCtrl->setFixedHeight(30);
    btnStopCtrl->setFixedHeight(30);
    btnStartCtrl->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");
    btnStopCtrl->setStyleSheet("background-color: #f44336; color: white; font-weight: bold;");
    btnLayout2->addStretch();
    btnLayout2->addWidget(btnStartCtrl);
    btnLayout2->addSpacing(15);
    btnLayout2->addWidget(btnStopCtrl);
    btnLayout2->addStretch();

    // --- 第五行：PI 参数细调 ---
    QHBoxLayout *piLayout = new QHBoxLayout();
    spinP = new QDoubleSpinBox(); spinP->setRange(-100, 100); spinP->setDecimals(2); spinP->setSingleStep(0.1);
    spinI = new QDoubleSpinBox(); spinI->setRange(-100, 100); spinI->setDecimals(2); spinI->setSingleStep(0.01);
    spinP->setFixedWidth(70); spinI->setFixedWidth(70);

    btnSetPI = new QPushButton("设置PI");

    piLayout->addStretch();
    piLayout->addWidget(new QLabel("Kp:"));
    piLayout->addWidget(spinP);
    piLayout->addSpacing(10);
    piLayout->addWidget(new QLabel("Ki:"));
    piLayout->addWidget(spinI);
    piLayout->addSpacing(10);
    piLayout->addWidget(btnSetPI);
    piLayout->addStretch();

    // 加入主布局
    mainLayout->addLayout(currLayout);
    mainLayout->addLayout(baseLayout);  // 基线校准
    mainLayout->addLayout(atLayout);    // 启动自整定
    mainLayout->addLayout(btnLayout2);  // 启停控制
    mainLayout->addLayout(piLayout);    // PI控制


    // 信号槽
    connect(btnSetBase, &QPushButton::clicked, this, &CO2CtrlWidget::onSetBase);
    connect(btnStartAT, &QPushButton::clicked, this, &CO2CtrlWidget::onStartAutoTune);
    connect(btnStartCtrl, &QPushButton::clicked, this, &CO2CtrlWidget::onStartControl);
    connect(btnStopCtrl, &QPushButton::clicked, this, &CO2CtrlWidget::onStopControl);
    connect(btnSetPI, &QPushButton::clicked, this, &CO2CtrlWidget::onSetPI);
}

void CO2CtrlWidget::updateCurrentCO2Display(long val)
{
    lblCurrentCO2->setText(QString::number(val) + " ppm");
}

void CO2CtrlWidget::updatePIDisplay(double p, double i)
{
    spinP->setValue(p);
    spinI->setValue(i);
}

void CO2CtrlWidget::onSetBase()
{
    int ret = QMessageBox::warning(this, "基线校准确认",
                                   "确定要执行基线校准吗？\n请确保传感器当前处于标准环境。",
                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        emit sendCommandSignal("D3setbase@");
    }
}

void CO2CtrlWidget::onStartAutoTune()
{
    int ret = QMessageBox::question(this, "自整定确认",
                                    "即将启动自整定，期间阀门将自动开合计算参数。\n确定要继续吗？",
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        emit sendCommandSignal("D3startAT@");
    }
}

void CO2CtrlWidget::onStartControl()
{
    emit sendCommandSignal("D3startctrl@");
}

void CO2CtrlWidget::onStopControl()
{
    emit sendCommandSignal("D3stopctrl@");
}


void CO2CtrlWidget::onSetPI()
{
    emit sendCommandSignal(QString("D3setp:%1@").arg(spinP->value()));
    emit sendCommandSignal(QString("D3seti:%1@").arg(spinI->value()));
}

// ============================================================================
//  MainWindow 实现
// ============================================================================
CO2Widget::CO2Widget(QWidget *parent)
    : QWidget(parent), serial(new QSerialPort(this))
{
    initUI();
    pollingTimer = new QTimer(this);
    pollingTimer->setInterval(1000);

    connect(pollingTimer, &QTimer::timeout, this, &CO2Widget::onPollingTimer);
    connect(serial, &QSerialPort::readyRead, this, &CO2Widget::readSerialData);
    connect(serial, &QSerialPort::errorOccurred, this, &CO2Widget::handleError);
    connect(co2Widget, &CO2CtrlWidget::sendCommandSignal, this, &CO2Widget::handleSendCommand);
}

CO2Widget::~CO2Widget()
{
    if (serial->isOpen()) serial->close();
}


void CO2Widget::initChart()
{
    chartCO2 = new QChart();
    chartCO2->setTitle("CO2 浓度实时曲线");
    chartCO2->setMargins(QMargins(10, 10, 10, 10));
    chartCO2->setTitleFont(QFont("Arial", 12, QFont::Bold));
    chartCO2->legend()->hide();

    // 暗黑主题与绿色线条
    chartCO2->setBackgroundBrush(QBrush(QColor("#000000")));
    chartCO2->setTitleBrush(QBrush(Qt::white));

    seriesCO2 = new QLineSeries();
    QPen greenPen(QColor("#a2f92e"));
    greenPen.setWidth(2);
    seriesCO2->setPen(greenPen);
    chartCO2->addSeries(seriesCO2);

    // === 新增：目标浓度虚线 (50000 ppm) ===
    seriesTargetCO2 = new QLineSeries();
    QPen targetPen(QColor("#FF9800")); // 橙色
    targetPen.setWidth(2);
    targetPen.setStyle(Qt::DashLine);
    seriesTargetCO2->setPen(targetPen);
    chartCO2->addSeries(seriesTargetCO2);

    // === 新增：目标浓度文字标签 ===
    labelSeriesCO2 = new QScatterSeries();
    labelSeriesCO2->setMarkerSize(0.1);
    labelSeriesCO2->setPen(QPen(Qt::transparent));
    labelSeriesCO2->setBrush(QBrush(Qt::transparent));
    labelSeriesCO2->setPointLabelsVisible(true);
    labelSeriesCO2->setPointLabelsFormat("                  50000ppm"); // 留出空格防止贴边
    labelSeriesCO2->setPointLabelsColor(QColor("#FF9800"));
    labelSeriesCO2->setPointLabelsFont(QFont("Arial", 10, QFont::Bold));
    chartCO2->addSeries(labelSeriesCO2);

    axisXCO2 = new QDateTimeAxis();
    axisXCO2->setFormat("HH:mm:ss");
    axisXCO2->setTickCount(7);
    axisXCO2->setLabelsColor(Qt::white);
    axisXCO2->setGridLineColor(QColor("#333333"));

    // 获取当前时间戳初始化范围
    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    axisXCO2->setRange(QDateTime::fromMSecsSinceEpoch(nowMs), QDateTime::fromMSecsSinceEpoch(nowMs + 60000));

    axisYCO2 = new QValueAxis();
    axisYCO2->setRange(0, 60000);
    axisYCO2->setTitleText("浓度 (ppm)");
    axisYCO2->setLabelFormat("%d");
    axisYCO2->setLabelsColor(Qt::white);
    axisYCO2->setGridLineColor(QColor("#333333"));
    axisYCO2->setTitleBrush(QBrush(Qt::white));

    chartCO2->addAxis(axisXCO2, Qt::AlignBottom);
    chartCO2->addAxis(axisYCO2, Qt::AlignLeft);

    seriesCO2->attachAxis(axisXCO2);
    seriesCO2->attachAxis(axisYCO2);

    seriesTargetCO2->attachAxis(axisXCO2);
    seriesTargetCO2->attachAxis(axisYCO2);
    labelSeriesCO2->attachAxis(axisXCO2);
    labelSeriesCO2->attachAxis(axisYCO2);

    // 初始化虚线和标签位置为真实时间戳
    seriesTargetCO2->replace(QList<QPointF>() << QPointF(nowMs, 50000) << QPointF(nowMs + 60000, 50000));
    labelSeriesCO2->replace(QList<QPointF>() << QPointF(nowMs, 50000));

    chartViewCO2 = new QChartView(chartCO2);
    chartViewCO2->setRenderHint(QPainter::Antialiasing);
}

void CO2Widget::initUI()
{
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QVBoxLayout *rightLayout = new QVBoxLayout();

    // ================= 左侧控制区 =================
    QGroupBox *serialGroup = new QGroupBox("通信设置");

    serialGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    serialGroup->setMinimumHeight(130); // 强制撑开最低高度

    QHBoxLayout *serialLayout = new QHBoxLayout();
    serialLayout->setContentsMargins(10, 30, 10, 30); // 增加上下边距，保证按钮不变形且留出空白

    portNameCombo = new QComboBox();
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) portNameCombo->addItem(info.portName());
    baudRateCombo = new QComboBox();
    baudRateCombo->addItems({"115200", "9600"});

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

    co2Widget = new CO2CtrlWidget("二氧化碳控制面板", this);

    btnShowChart = new QPushButton("查看二氧化碳曲线");
    btnShowChart->setFixedHeight(40); // 稍微加高一点以填充空间
    btnShowChart->setStyleSheet("font-weight: bold; font-size: 14px; background-color: #28a745; color: white;");

    leftLayout->addWidget(serialGroup);
    leftLayout->addWidget(co2Widget);
    leftLayout->addWidget(btnShowChart);

    // 初始化图表和弹窗
    initChart();
    chartDialog = new QDialog(this);
    chartDialog->setWindowTitle("CO2 浓度实时曲线");
    chartDialog->resize(700, 450);
    QVBoxLayout *dialogLayout = new QVBoxLayout(chartDialog);
    dialogLayout->addWidget(chartViewCO2);

    // ================= 右侧日志区 =================
    logEditor = new QPlainTextEdit();
    logEditor->setReadOnly(true);
    logEditor->setStyleSheet("font-family: Consolas; font-size: 9pt; background-color: #f8f9fa;");
    logEditor->setMinimumWidth(300);

    QPushButton *btnClearLog = new QPushButton("清空日志信息");
    btnClearLog->setStyleSheet("height: 30px; font-weight: bold;");

    rightLayout->addWidget(new QLabel("系统日志:"));
    rightLayout->addWidget(logEditor);
    rightLayout->addWidget(btnClearLog);

    rootLayout->addLayout(leftLayout);
    rootLayout->addLayout(rightLayout);

    // ================= 信号连接 =================
    connect(connectBtn, &QPushButton::clicked, this, &CO2Widget::openSerialPort);
    connect(disconnectBtn, &QPushButton::clicked, this, &CO2Widget::closeSerialPort);
    connect(btnClearLog, &QPushButton::clicked, this, &CO2Widget::onClearLog);
    connect(btnShowChart, &QPushButton::clicked, this, &CO2Widget::onShowChartClicked);

    setWindowTitle("二氧化碳控制系统");
    resize(1100, 700);
}



void CO2Widget::onShowChartClicked()
{
    if (chartDialog) {
        chartDialog->show();
        chartDialog->raise();
        chartDialog->activateWindow();
    }
}

void CO2Widget::appendLog(const QString &text)
{
    QString timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss] ");
    logEditor->appendPlainText(timestamp + text);
    QScrollBar *bar = logEditor->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void CO2Widget::onClearLog()
{
    logEditor->clear();
}

void CO2Widget::openSerialPort()
{
    serial->setPortName(portNameCombo->currentText());
    serial->setBaudRate(baudRateCombo->currentText().toInt());
    serial->setDataBits(QSerialPort::Data8);
    serial->setParity(QSerialPort::NoParity);
    serial->setStopBits(QSerialPort::OneStop);
    serial->setFlowControl(QSerialPort::NoFlowControl);

    if (serial->open(QIODevice::ReadWrite)) {
        connectBtn->setEnabled(false);
        disconnectBtn->setEnabled(true);
        portNameCombo->setEnabled(false);

        m_startTimeMs = QDateTime::currentMSecsSinceEpoch();
        seriesCO2->clear();
        QDateTime startTime = QDateTime::fromMSecsSinceEpoch(m_startTimeMs);
        QDateTime endTime = startTime.addSecs(60);
        axisXCO2->setRange(startTime, endTime);

        logEditor->clear();
        appendLog("✅ 连接成功: " + serial->portName());

        pollingTimer->start();
        // 连接成功后，延时 500ms 自动获取一次 PI 参数
        QTimer::singleShot(500, this, [this]() {
            if (serial->isOpen()) {
                serial->write("D3getpi@");
                appendLog("TX -> D3getpi@ (上电自动读取)");
            }
        });
    } else {
        QMessageBox::critical(this, "错误", serial->errorString());
        appendLog("❌ 连接失败: " + serial->errorString());
    }
}

void CO2Widget::closeSerialPort()
{
    pollingTimer->stop();
    if (serial->isOpen()) serial->close();

    connectBtn->setEnabled(true);
    disconnectBtn->setEnabled(false);
    portNameCombo->setEnabled(true);
    appendLog("🔴 连接已断开");
}

void CO2Widget::handleSendCommand(QString cmd)
{
    if (!serial->isOpen()) return;
    serial->write(cmd.toLocal8Bit());
    appendLog("TX -> " + cmd.trimmed());
}

void CO2Widget::onPollingTimer()
{
    if (!serial->isOpen()) return;
    serial->write("D3getco2@");
}



void CO2Widget::readSerialData()
{
    while (serial->canReadLine()) {
        QByteArray lineData = serial->readLine();
        QString line = QString::fromLocal8Bit(lineData).trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith("CO2Concentration:")) {
            QRegularExpression rx("(\\d+)");
            QRegularExpressionMatch match = rx.match(line);
            if (match.hasMatch()) {
                long co2Val = match.captured(1).toLong();
                qint64 currentMs = QDateTime::currentMSecsSinceEpoch(); // 获取当前毫秒时间戳

                co2Widget->updateCurrentCO2Display(co2Val);
                seriesCO2->append(currentMs, co2Val); // 传入真实的毫秒时间戳

                // --- 动态计算 X 轴范围 (最多显示30分钟 = 1800000 毫秒) ---
                qint64 minXMs = currentMs - 1800000;
                if (minXMs < m_startTimeMs) {
                    minXMs = m_startTimeMs; // 运行不足30分钟时，起点固定为连接时间
                }
                qint64 maxXMs = qMax(m_startTimeMs + 60000, currentMs + 10000); // 留出一点余量

                QDateTime minTime = QDateTime::fromMSecsSinceEpoch(minXMs);
                QDateTime maxTime = QDateTime::fromMSecsSinceEpoch(maxXMs);

                // 应用动态范围
                axisXCO2->setRange(minTime, maxTime);

                // 更新虚线和标签的时间坐标
                seriesTargetCO2->replace(QList<QPointF>() << QPointF(minXMs, 50000.0) << QPointF(maxXMs, 50000.0));
                labelSeriesCO2->replace(QList<QPointF>() << QPointF(minXMs, 50000.0));

                // 1000ms轮询一次，30分钟大约 1800 个点，保留 2000 个防溢出
                if (seriesCO2->count() > 2000) {
                    seriesCO2->removePoints(0, 100);
                }
            }
        }
        else if (line.startsWith("CurrentPI:")) {
            QRegularExpression rx("Kp=([-+]?\\d*\\.?\\d+),\\s*Ki=([-+]?\\d*\\.?\\d+)");
            QRegularExpressionMatch match = rx.match(line);
            if (match.hasMatch()) {
                double p = match.captured(1).toDouble();
                double i = match.captured(2).toDouble();
                co2Widget->updatePIDisplay(p, i);
                appendLog("✅ PI参数已更新");
            }
        }
        else {
            appendLog("RX <- " + line);
        }
    }
}



void CO2Widget::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, "错误", serial->errorString());
        closeSerialPort();
    }
}
