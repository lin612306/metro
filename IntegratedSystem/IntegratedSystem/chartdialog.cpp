#include "chartdialog.h"
#include <QVBoxLayout>
#include <QDateTime>
#include <QGraphicsLayout>

ChartDialog::ChartDialog(const QString &title, const QString &seriesName, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(title);
    resize(900, 500);

    chart = new QChart();
    series = new QSplineSeries();
    series->setName(seriesName);

    // 样式：暗黑背景，亮绿色线条
    QPen greenPen(QColor("#a2f92e"));
    greenPen.setWidth(2);
    series->setPen(greenPen);
    chart->addSeries(series);
    chart->setBackgroundBrush(QBrush(QColor("#000000")));
    chart->setTitleBrush(QBrush(Qt::white));
    chart->legend()->setLabelColor(Qt::white);
    chart->legend()->setAlignment(Qt::AlignTop | Qt::AlignRight);

    axisX = new QDateTimeAxis();
    axisX->setFormat("HH:mm:ss");
    axisX->setLabelsColor(Qt::white);
    axisX->setGridLineColor(QColor("#333333"));
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    axisY = new QValueAxis();
    axisY->setLabelFormat("%.1f");
    axisY->setLabelsColor(Qt::white);
    axisY->setGridLineColor(QColor("#333333"));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->layout()->setContentsMargins(0, 0, 0, 0);
    chart->setBackgroundRoundness(0);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(chartView);
}

void ChartDialog::appendData(qint64 msecs, double value)
{
    series->append(msecs, value);
    // 动态调整X轴显示最近3分钟的数据
    axisX->setMin(QDateTime::fromMSecsSinceEpoch(msecs).addSecs(-180));
    axisX->setMax(QDateTime::fromMSecsSinceEpoch(msecs));

    // Y轴可以根据传入的 value 动态扩展范围，这里预设一个基础范围
    if (value > axisY->max() - 2) axisY->setMax(value + 5);
    if (value < axisY->min() + 2) axisY->setMin(value - 5);
}
