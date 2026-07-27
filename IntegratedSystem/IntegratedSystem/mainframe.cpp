#include "mainframe.h"
#include "co2widget.h"
#include "tempwidget.h"
#include "motorwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

MainFrame::MainFrame(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("细胞培养系统");
    resize(1200, 800);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    // 导航栏
    QHBoxLayout *navLayout = new QHBoxLayout();
    btnTemp = new QPushButton("温度控制");
    btnCO2 = new QPushButton("二氧化碳控制");
    btnMotor = new QPushButton("流速控制");

    QString btnStyle = "QPushButton { padding: 8px 20px; font-size: 14px; font-weight: bold; }";
    btnTemp->setStyleSheet(btnStyle);
    btnCO2->setStyleSheet(btnStyle);
    btnMotor->setStyleSheet(btnStyle);

    navLayout->addWidget(btnTemp);
    navLayout->addWidget(btnCO2);
    navLayout->addWidget(btnMotor);
    navLayout->addStretch();

    // 堆叠界面区
    stackedWidget = new QStackedWidget(this);
    stackedWidget->addWidget(new TempWidget(this));
    stackedWidget->addWidget(new CO2Widget(this));
    stackedWidget->addWidget(new MotorWidget(this));

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(stackedWidget);

    // 绑定切换事件
    connect(btnTemp, &QPushButton::clicked, [=]() { stackedWidget->setCurrentIndex(0); });
    connect(btnCO2, &QPushButton::clicked, [=]() { stackedWidget->setCurrentIndex(1); });
    connect(btnMotor, &QPushButton::clicked, [=]() { stackedWidget->setCurrentIndex(2); });
}

MainFrame::~MainFrame() {}
