#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>

class MainFrame : public QMainWindow
{
    Q_OBJECT

public:
    MainFrame(QWidget *parent = nullptr);
    ~MainFrame();

private:
    QStackedWidget *stackedWidget;
    QPushButton *btnTemp;
    QPushButton *btnCO2;
    QPushButton *btnMotor;
};

#endif // MAINFRAME_H
