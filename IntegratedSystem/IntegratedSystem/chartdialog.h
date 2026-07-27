#ifndef CHARTDIALOG_H
#define CHARTDIALOG_H

#include <QDialog>
#include <QChart>
#include <QChartView>
#include <QSplineSeries>
#include <QValueAxis>
#include <QDateTimeAxis>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QT_CHARTS_USE_NAMESPACE
#endif

    class ChartDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ChartDialog(const QString &title, const QString &seriesName, QWidget *parent = nullptr);
    void appendData(qint64 msecs, double value);

private:
    QChart *chart;
    QChartView *chartView;
    QSplineSeries *series;
    QDateTimeAxis *axisX;
    QValueAxis *axisY;
};

#endif // CHARTDIALOG_H
