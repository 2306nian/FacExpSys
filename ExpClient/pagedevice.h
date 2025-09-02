#ifndef PAGEDEVICE_H
#define PAGEDEVICE_H

#include <QWidget>
#include <QtCharts>
#include <QChartView>
#include <QLineSeries>
#include"session.h"
#include <QJsonArray>
#include <QStandardItemModel>
#include "pagedevice.h"
#include "ui_pagedevice.h"
#include <QStandardItemModel>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>  // 添加这个
#include "globaldatas.h"     // 假设g_session定义在这里


QT_CHARTS_USE_NAMESPACE

    namespace Ui {
    class PageDevice;
}

class PageDevice : public QWidget
{
    Q_OBJECT

public:
    explicit PageDevice(QWidget *parent = nullptr);
    ~PageDevice();

private:
    Ui::PageDevice *ui;
    QStandardItemModel *model;

    // 温度数据存储，key为device_id，value为最近10个温度，保持时间顺序
    QMap<QString, QVector<double>> temperatureHistory;

    // 设备颜色映射，保证3设备颜色固定
    QMap<QString, QColor> deviceColors;

    // 图表相关
    QtCharts::QChart *chart;
    QtCharts::QLineSeries *seriesDevice1;
    QtCharts::QLineSeries *seriesDevice2;
    QtCharts::QLineSeries *seriesDevice3;

    void setupDeviceUI();
    void setupCharts();
    void updateChart();

public slots:
    void updateDeviceData(const QJsonArray &devices);


};


#endif // PAGEDEVICE_H
