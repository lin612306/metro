#include "tempwidget.h"
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>

// ============================================================================
//  TempCtrlWidget 实现
// ============================================================================
TempCtrlWidget::TempCtrlWidget(const QString &title, const QString &prefix, QWidget *parent)
    : QGroupBox(title, parent), m_prefix(prefix)
{
    QFont font = this->font();
    font.setPointSize(9);
    this->setFont(font);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(10, 20, 10, 15);

    // --- 第一行：当前温度 ---
    QHBoxLayout *currLayout = new QHBoxLayout();
    QLabel *lblTitle = new QLabel("当前温度:");
    lblCurrentTemp = new QLabel("0.00 °C");
    lblCurrentTemp->setStyleSheet("font-size: 26px; font-weight: bold; color: #007ACC; font-family: Arial;");
    currLayout->addStretch();
    currLayout->addWidget(lblTitle);
    currLayout->addSpacing(10);
    currLayout->addWidget(lblCurrentTemp);
    currLayout->addStretch();

    // --- 第二行：目标设置 ---
    QHBoxLayout *targetLayout = new QHBoxLayout();
    spinTargetTemp = new QDoubleSpinBox();
    spinTargetTemp->setRange(0, 100);
    spinTargetTemp->setDecimals(1);
    spinTargetTemp->setValue(prefix == "D0" ? 37.0 : 30.0);
    spinTargetTemp->setSuffix("°C");
    spinTargetTemp->setFixedWidth(100);
    spinTargetTemp->setFixedHeight(24);
    btnSetTemp = new QPushButton("设置");
    btnSetTemp->setFixedWidth(60);
    btnSetTemp->setFixedHeight(24);
    targetLayout->addStretch();
    targetLayout->addWidget(new QLabel("目标:"));
    targetLayout->addSpacing(15);
    targetLayout->addWidget(spinTargetTemp);
    targetLayout->addSpacing(15);
    targetLayout->addWidget(btnSetTemp);
    targetLayout->addStretch();

    // --- 第三行：PID ---
    QHBoxLayout *pidLayout = new QHBoxLayout();
    spinP = new QDoubleSpinBox(); spinP->setRange(0, 1000); spinP->setValue(10.0);
    spinI = new QDoubleSpinBox(); spinI->setRange(0, 1000); spinI->setValue(0.5);
    spinD = new QDoubleSpinBox(); spinD->setRange(0, 1000); spinD->setValue(1.0);
    int pidBoxWidth = 55;
    spinP->setFixedWidth(pidBoxWidth); spinI->setFixedWidth(pidBoxWidth); spinD->setFixedWidth(pidBoxWidth);
    spinP->setFixedHeight(22); spinI->setFixedHeight(22); spinD->setFixedHeight(22);
    btnSetPID = new QPushButton("设置PID");
    btnSetPID->setFixedWidth(70);
    btnSetPID->setFixedHeight(22);
    pidLayout->addStretch();
    pidLayout->addWidget(new QLabel("P")); pidLayout->addWidget(spinP); pidLayout->addSpacing(15);
    pidLayout->addWidget(new QLabel("I")); pidLayout->addWidget(spinI); pidLayout->addSpacing(15);
    pidLayout->addWidget(new QLabel("D")); pidLayout->addWidget(spinD); pidLayout->addSpacing(15);
    pidLayout->addWidget(btnSetPID);
    pidLayout->addStretch();

    QHBoxLayout *ctrlLayout = new QHBoxLayout();
    btnStartCtrl = new QPushButton("启动");
    btnStopCtrl = new QPushButton("停止");

    btnStartCtrl->setStyleSheet("background-color: #4CAF50; color: white;");
    btnStopCtrl->setStyleSheet("background-color: #f44336; color: white;");
    btnStartCtrl->setFixedHeight(24);
    btnStopCtrl->setFixedHeight(24);

    ctrlLayout->addStretch();
    ctrlLayout->addWidget(btnStartCtrl);
    ctrlLayout->addSpacing(15);
    ctrlLayout->addWidget(btnStopCtrl);
    ctrlLayout->addStretch();

    QHBoxLayout *atLayout = new QHBoxLayout();
    btnStartAT = new QPushButton("启动自整定");
    btnStartAT->setFixedHeight(24); // 与其他按钮保持一致的高度
    atLayout->addStretch();
    atLayout->addWidget(btnStartAT);
    atLayout->addStretch();

    // --- 第四行 ：查看曲线按钮 ---
    QHBoxLayout *chartBtnLayout = new QHBoxLayout();
    btnShowChart = new QPushButton(title.contains("内部") ? "查看内部曲线" : "查看外部曲线");
    btnShowChart->setFixedHeight(32);
    btnShowChart->setStyleSheet("font-weight: bold; background-color: #007ACC; color: white;");
    chartBtnLayout->addWidget(btnShowChart);

    // --- 组装主布局 ---
    mainLayout->addLayout(currLayout);
    mainLayout->addLayout(targetLayout);
    mainLayout->addLayout(pidLayout);
    mainLayout->addLayout(atLayout);
    mainLayout->addLayout(ctrlLayout);
    mainLayout->addLayout(chartBtnLayout);

    connect(btnSetTemp, &QPushButton::clicked, this, &TempCtrlWidget::onSetTemp);
    connect(btnSetPID, &QPushButton::clicked, this, &TempCtrlWidget::onSetPID);
    connect(btnStartAT, &QPushButton::clicked, this, &TempCtrlWidget::onStartAutoTune);
    connect(btnStartCtrl, &QPushButton::clicked, this, &TempCtrlWidget::onStartCtrl);
    connect(btnStopCtrl, &QPushButton::clicked, this, &TempCtrlWidget::onStopCtrl);
    connect(btnShowChart, &QPushButton::clicked, this, &TempCtrlWidget::showChartSignal);
}


void TempCtrlWidget::onStartAutoTune()
{
    int ret = QMessageBox::question(this, "自整定确认",
                                    "即将启动温度自整定，期间加热器将自动启停以计算PID参数。\n确定要继续吗？",
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        emit sendCommandSignal(QString("%1startAT@").arg(m_prefix));
    }
}


void TempCtrlWidget::onStartCtrl()
{
    // 假设硬件接收的启停指令为 startctrl / stopctrl，如 D0startctrl@
    emit sendCommandSignal(QString("%1startctrl@").arg(m_prefix));
}


void TempCtrlWidget::onStopCtrl()
{
    emit sendCommandSignal(QString("%1stopctrl@").arg(m_prefix));
}


void TempCtrlWidget::updateCurrentTempDisplay(double val)
{
    lblCurrentTemp->setText(QString::number(val, 'f', 2) + " °C");
}

void TempCtrlWidget::onSetTemp()
{
    QString cmd = QString("%1settemp:%2@").arg(m_prefix).arg(spinTargetTemp->value());
    emit sendCommandSignal(cmd);
}

void TempCtrlWidget::onSetPID()
{
    emit sendCommandSignal(QString("%1setp:%2@").arg(m_prefix).arg(spinP->value()));
    emit sendCommandSignal(QString("%1seti:%2@").arg(m_prefix).arg(spinI->value()));
    emit sendCommandSignal(QString("%1setd:%2@").arg(m_prefix).arg(spinD->value()));
}


// ============================================================================
//  MainWindow 实现
// ============================================================================

TempWidget::TempWidget(QWidget *parent)
    : QWidget(parent), serial(new QSerialPort(this)), m_pollingState(0)
{
    initUI();

    pollingTimer = new QTimer(this);
    pollingTimer->setInterval(500);

    connect(pollingTimer, &QTimer::timeout, this, &TempWidget::onPollingTimer);
    connect(serial, &QSerialPort::readyRead, this, &TempWidget::readSerialData);
    connect(serial, &QSerialPort::errorOccurred, this, &TempWidget::handleError);
}

TempWidget::~TempWidget()
{
    if (serial->isOpen()) serial->close();
}
void TempWidget::initCharts()
{
    auto configAxis = [](QValueAxis *axis, double min, double max, const QString &title) {
        axis->setRange(min, max);
        axis->setTitleText(title);
        axis->setTickCount(8);
        QFont labelsFont; labelsFont.setPixelSize(10);
        axis->setLabelsFont(labelsFont);
        QFont titleFont; titleFont.setBold(true); titleFont.setPixelSize(11);
        axis->setTitleFont(titleFont);
        axis->setLabelsColor(Qt::white);
        axis->setGridLineColor(QColor("#333333"));
        axis->setTitleBrush(QBrush(Qt::white));
    };

    auto configChartStyle = [](QChart *chart, QLineSeries *series) {
        chart->setMargins(QMargins(0, 0, 0, 0));
        chart->layout()->setContentsMargins(0, 0, 0, 0);
        chart->setTitleFont(QFont("Arial", 10, QFont::Bold));
        chart->legend()->hide();
        chart->setBackgroundBrush(QBrush(QColor("#000000")));
        chart->setTitleBrush(QBrush(Qt::white));

        QPen greenPen(QColor("#a2f92e"));
        greenPen.setWidth(2);
        series->setPen(greenPen);
        chart->addSeries(series);
    };

    auto configXTimeAxis = [](QDateTimeAxis *axis) {
        axis->setFormat("HH:mm:ss");
        axis->setTickCount(7);
        QFont labelsFont; labelsFont.setPixelSize(10);
        axis->setLabelsFont(labelsFont);
        axis->setLabelsColor(Qt::white);
        axis->setGridLineColor(QColor("#333333"));

        QDateTime now = QDateTime::currentDateTime();
        axis->setRange(now, now.addSecs(60)); // 初始显示当前时间到未来60秒
    };

    // 获取当前时间戳用于初始化辅助线
    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    // === 1. 内部温度 (D0) ===
    chartInt = new QChart();
    chartInt->setTitle("内部温度实时曲线");
    seriesInt = new QLineSeries();
    configChartStyle(chartInt, seriesInt);

    axisXInt = new QDateTimeAxis();
    configXTimeAxis(axisXInt);

    axisYInt = new QValueAxis();
    configAxis(axisYInt, 28, 42, "温度（°C）");
    axisYInt->setLabelFormat("%.1f");

    chartInt->addAxis(axisXInt, Qt::AlignBottom);
    chartInt->addAxis(axisYInt, Qt::AlignLeft);
    seriesInt->attachAxis(axisXInt);
    seriesInt->attachAxis(axisYInt);

    seriesTargetInt = new QLineSeries();
    QPen targetPenInt(QColor("#FF9800")); // 橙色，便于和绿色区分
    targetPenInt.setWidth(2);
    targetPenInt.setStyle(Qt::DashLine);  // 设置为虚线
    seriesTargetInt->setPen(targetPenInt);
    chartInt->addSeries(seriesTargetInt);
    seriesTargetInt->attachAxis(axisXInt);
    seriesTargetInt->attachAxis(axisYInt);
    seriesTargetInt->replace(QList<QPointF>() << QPointF(nowMs, 37.0) << QPointF(nowMs + 60000, 37.0));

    labelSeriesInt = new QScatterSeries();
    labelSeriesInt->setMarkerSize(0.1); // 点设到最小
    labelSeriesInt->setPen(QPen(Qt::transparent)); // 边框透明
    labelSeriesInt->setBrush(QBrush(Qt::transparent)); // 填充透明
    labelSeriesInt->setPointLabelsVisible(true);          // 显示标签
    labelSeriesInt->setPointLabelsFormat("         37℃");        // 标签文字
    labelSeriesInt->setPointLabelsColor(QColor("#FF9800"));
    labelSeriesInt->setPointLabelsFont(QFont("Arial", 10, QFont::Bold));
    chartInt->addSeries(labelSeriesInt);
    labelSeriesInt->attachAxis(axisXInt);
    labelSeriesInt->attachAxis(axisYInt);
    labelSeriesInt->replace(QList<QPointF>() << QPointF(nowMs, 37.0));

    chartViewInt = new QChartView(chartInt);
    chartViewInt->setRenderHint(QPainter::Antialiasing);

    // === 2. 外部温度 (D1) ===
    chartExt = new QChart();
    chartExt->setTitle("外部温度实时曲线");
    seriesExt = new QLineSeries();
    configChartStyle(chartExt, seriesExt);

    axisXExt = new QDateTimeAxis();
    configXTimeAxis(axisXExt);

    axisYExt = new QValueAxis();
    configAxis(axisYExt, 20, 34, "温度（°C）");
    axisYExt->setLabelFormat("%.1f");

    chartExt->addAxis(axisXExt, Qt::AlignBottom);
    chartExt->addAxis(axisYExt, Qt::AlignLeft);
    seriesExt->attachAxis(axisXExt);
    seriesExt->attachAxis(axisYExt);

    seriesTargetExt = new QLineSeries();
    QPen targetPenExt(QColor("#FF9800"));
    targetPenExt.setWidth(2);
    targetPenExt.setStyle(Qt::DashLine);
    seriesTargetExt->setPen(targetPenExt);
    chartExt->addSeries(seriesTargetExt);
    seriesTargetExt->attachAxis(axisXExt);
    seriesTargetExt->attachAxis(axisYExt);
    seriesTargetExt->replace(QList<QPointF>() << QPointF(nowMs, 30.0) << QPointF(nowMs + 60000, 30.0));

    labelSeriesExt = new QScatterSeries();
    labelSeriesExt->setMarkerSize(0.1);
    labelSeriesExt->setPen(QPen(Qt::transparent));
    labelSeriesExt->setBrush(QBrush(Qt::transparent));
    labelSeriesExt->setPointLabelsVisible(true);
    labelSeriesExt->setPointLabelsFormat("         30℃");
    labelSeriesExt->setPointLabelsColor(QColor("#FF9800"));
    labelSeriesExt->setPointLabelsFont(QFont("Arial", 10, QFont::Bold));
    chartExt->addSeries(labelSeriesExt);
    labelSeriesExt->attachAxis(axisXExt);
    labelSeriesExt->attachAxis(axisYExt);
    labelSeriesExt->replace(QList<QPointF>() << QPointF(nowMs, 30.0));

    chartViewExt = new QChartView(chartExt);
    chartViewExt->setRenderHint(QPainter::Antialiasing);
}



void TempWidget::initUI()
{
    QHBoxLayout *rootLayout = new QHBoxLayout(this);
    QVBoxLayout *leftLayout = new QVBoxLayout();
    QVBoxLayout *rightLayout = new QVBoxLayout();

    // ================= 通信设置 =================
    QGroupBox *serialGroup = new QGroupBox("通信设置");

    serialGroup->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    serialGroup->setMinimumHeight(130); // 强制撑开最低高度

    QHBoxLayout *serialLayout = new QHBoxLayout();
    serialLayout->setContentsMargins(10, 30, 10, 30); // 增加上下边距，保证按钮不变形且留出空白

    portNameCombo = new QComboBox();

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

    // ================= 温度控制面板 =================
    internalTempWidget = new TempCtrlWidget("内部温度控制", "D0", this);
    externalTempWidget = new TempCtrlWidget("外部温度控制", "D1", this);

    leftLayout->addWidget(internalTempWidget);
    leftLayout->addWidget(externalTempWidget);

    // ================= 初始化图表和弹窗 =================
    initCharts();
    chartDialogInt = new QDialog(this);
    chartDialogInt->setWindowTitle("内部温度实时曲线");
    chartDialogInt->resize(700, 450);
    QVBoxLayout *dialogIntLayout = new QVBoxLayout(chartDialogInt);
    dialogIntLayout->addWidget(chartViewInt);

    chartDialogExt = new QDialog(this);
    chartDialogExt->setWindowTitle("外部温度实时曲线");
    chartDialogExt->resize(700, 450);
    QVBoxLayout *dialogExtLayout = new QVBoxLayout(chartDialogExt);
    dialogExtLayout->addWidget(chartViewExt);

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
    connect(connectBtn, &QPushButton::clicked, this, &TempWidget::openSerialPort);
    connect(disconnectBtn, &QPushButton::clicked, this, &TempWidget::closeSerialPort);
    connect(btnClearLog, &QPushButton::clicked, this, &TempWidget::onClearLog);
    connect(internalTempWidget, &TempCtrlWidget::showChartSignal, this, &TempWidget::onShowIntChartClicked);
    connect(externalTempWidget, &TempCtrlWidget::showChartSignal, this, &TempWidget::onShowExtChartClicked);

    setWindowTitle("温度控制系统");
    resize(1100, 700);
}


void TempWidget::appendLog(const QString &text)
{
    QString timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss] ");
    logEditor->appendPlainText(timestamp + text);
    QScrollBar *bar = logEditor->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void TempWidget::onClearLog()
{
    logEditor->clear();
}

void TempWidget::openSerialPort()
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

        m_startTimeMs = QDateTime::currentMSecsSinceEpoch();
        seriesInt->clear();
        seriesExt->clear();
        QDateTime startTime = QDateTime::fromMSecsSinceEpoch(m_startTimeMs);
        QDateTime endTime = startTime.addSecs(60);
        axisXInt->setRange(startTime, endTime);
        axisXExt->setRange(startTime, endTime);

        logEditor->clear();
        appendLog("连接成功: " + serial->portName());

        m_pollingState = 0;
        pollingTimer->start();
    } else {
        QMessageBox::critical(this, "错误", serial->errorString());
        appendLog("连接失败: " + serial->errorString());
    }
}

void TempWidget::closeSerialPort()
{
    pollingTimer->stop();
    if (serial->isOpen()) serial->close();

    connectBtn->setEnabled(true);
    disconnectBtn->setEnabled(false);
    portNameCombo->setEnabled(true);
    appendLog("连接已断开");
}

void TempWidget::handleSendCommand(QString cmd)
{
    if (!serial->isOpen()) return;
    serial->write(cmd.toLocal8Bit());
    appendLog("发送 -> " + cmd.trimmed());
}

void TempWidget::onPollingTimer()
{
    if (!serial->isOpen()) return;
    if (m_pollingState == 0) {
        serial->write("D0gettemp@");
        m_pollingState = 1;
    } else if (m_pollingState == 1) {
        serial->write("D1gettemp@");
        m_pollingState = 0;
    }
}
void TempWidget::readSerialData()
{
    while (serial->canReadLine()) {
        QByteArray lineData = serial->readLine();
        QString line = QString::fromLocal8Bit(lineData).trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith("CMD") || line.contains("Error")) {
            appendLog("系统: " + line);
            continue;
        }

        bool ok;
        double tempVal = line.toDouble(&ok);
        if (ok) {
            // 直接获取当前的毫秒时间戳
            qint64 currentMs = QDateTime::currentMSecsSinceEpoch();

            // --- 动态计算 X 轴范围 (最多显示30分钟 = 1800000 毫秒) ---
            qint64 minXMs = currentMs - 1800000;
            if (minXMs < m_startTimeMs) {
                minXMs = m_startTimeMs; // 如果运行时间不足30分钟，起点固定为连接串口的时间
            }
            // 保证右侧有一点余量（刚开始时强制撑满60秒，之后跟随当前时间推移）
            qint64 maxXMs = qMax(m_startTimeMs + 60000, currentMs + 10000);

            QDateTime minTime = QDateTime::fromMSecsSinceEpoch(minXMs);
            QDateTime maxTime = QDateTime::fromMSecsSinceEpoch(maxXMs);

            if (m_pollingState == 1) {
                internalTempWidget->updateCurrentTempDisplay(tempVal);

                // 传入时间戳
                seriesInt->append(currentMs, tempVal);
                // 应用真实时间范围
                axisXInt->setRange(minTime, maxTime);
                //更新虚线和标签的时间坐标
                seriesTargetInt->replace(QList<QPointF>() << QPointF(minXMs, 37.0) << QPointF(maxXMs, 37.0));
                labelSeriesInt->replace(QList<QPointF>() << QPointF(minXMs, 37.0));

                if (seriesInt->count() > 4000) seriesInt->removePoints(0, 100);
            } else {
                externalTempWidget->updateCurrentTempDisplay(tempVal);
                // 传入时间戳
                seriesExt->append(currentMs, tempVal);
                // 应用真实时间范围
                axisXExt->setRange(minTime, maxTime);
                // 更新虚线和标签的时间坐标
                seriesTargetExt->replace(QList<QPointF>() << QPointF(minXMs, 30.0) << QPointF(maxXMs, 30.0));
                labelSeriesExt->replace(QList<QPointF>() << QPointF(minXMs, 30.0));

                if (seriesExt->count() > 4000) seriesExt->removePoints(0, 100);
            }
        }
    }
}

void TempWidget::handleError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        QMessageBox::critical(this, "错误", serial->errorString());
        closeSerialPort();
    }
}

void TempWidget::onShowIntChartClicked()
{
    if (chartDialogInt) {
        chartDialogInt->show();
        chartDialogInt->raise();
        chartDialogInt->activateWindow();
    }
}

void TempWidget::onShowExtChartClicked()
{
    if (chartDialogExt) {
        chartDialogExt->show();
        chartDialogExt->raise();
        chartDialogExt->activateWindow();
    }
}
