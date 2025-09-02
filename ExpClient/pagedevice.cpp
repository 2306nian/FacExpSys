#include "pagedevice.h"
#include "ui_pagedevice.h"
#include <QStandardItemModel>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

PageDevice::PageDevice(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::PageDevice)
{
    ui->setupUi(this);
    // 在创建PageDevice实例的代码处（如主窗口等）
    connect(g_session, &Session::deviceDataArrayReceived, this, &PageDevice::updateDeviceData);

    // 表格模型初始化
    model = new QStandardItemModel(this);
    model->setColumnCount(6);
    model->setHeaderData(0, Qt::Horizontal, "Device ID");
    model->setHeaderData(1, Qt::Horizontal, "Name");
    model->setHeaderData(2, Qt::Horizontal, "Type");
    model->setHeaderData(3, Qt::Horizontal, "Location");
    model->setHeaderData(4, Qt::Horizontal, "Online Status");
    model->setHeaderData(5, Qt::Horizontal, "Temperature");
    ui->tableView->setModel(model);

    // 设备颜色固定，假设三个设备ID分别为"dev1","dev2","dev3"，若不同根据实际替换
    deviceColors["dev1"] = Qt::red;
    deviceColors["dev2"] = Qt::green;
    deviceColors["dev3"] = Qt::blue;

    // 初始化chart和series
    setupCharts();
}

PageDevice::~PageDevice(){
    delete ui;
}

void PageDevice::setupCharts()
{
    using namespace QtCharts;

    chart = new QChart();
    chart->legend()->setVisible(true);
    chart->setTitle("设备温度历史");

    seriesDevice1 = new QLineSeries();
    seriesDevice1->setName("Device 1");
    seriesDevice1->setColor(deviceColors.value("dev1", Qt::red));

    seriesDevice2 = new QLineSeries();
    seriesDevice2->setName("Device 2");
    seriesDevice2->setColor(deviceColors.value("dev2", Qt::green));

    seriesDevice3 = new QLineSeries();
    seriesDevice3->setName("Device 3");
    seriesDevice3->setColor(deviceColors.value("dev3", Qt::blue));

    chart->addSeries(seriesDevice1);
    chart->addSeries(seriesDevice2);
    chart->addSeries(seriesDevice3);

    // 创建坐标轴
    QValueAxis *axisX = new QValueAxis;
    axisX->setRange(0, 9); // 最多显示10个点，索引0~9
    axisX->setLabelFormat("%d");
    axisX->setTitleText("采样序号");

    QValueAxis *axisY = new QValueAxis;
    axisY->setRange(0, 100); // 根据实际温度范围调整
    axisY->setTitleText("温度");

    chart->setAxisX(axisX, seriesDevice1);
    chart->setAxisY(axisY, seriesDevice1);
    chart->setAxisX(axisX, seriesDevice2);
    chart->setAxisY(axisY, seriesDevice2);
    chart->setAxisX(axisX, seriesDevice3);
    chart->setAxisY(axisY, seriesDevice3);

    // 将chart放入widget中
    QChartView *chartView = new QChartView(chart, ui->widget);
    chartView->setRenderHint(QPainter::Antialiasing);

    // 布局
    QVBoxLayout *layout = new QVBoxLayout(ui->widget);
    layout->addWidget(chartView);
    ui->widget->setLayout(layout);
}
void PageDevice::updateDeviceData(const QJsonArray &devices)
{
    model->removeRows(0, model->rowCount());  // 清空旧数据
    qDebug()<<devices;
    // 遍历设备数组填充表格，并更新温度历史
    for (int i = 0; i < devices.size(); ++i) {
        QJsonObject dev = devices[i].toObject();
        QString deviceId = dev["device_id"].toString();
        QString name = dev["name"].toString();
        QString type = dev["timestamp"].toString();
        QString location = dev["pressure"].toString();
        QString onlineStatus = dev["status"].toString();
        double temperature = dev["temperature"].toDouble();

        // 填表
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(deviceId)
                 << new QStandardItem(name)
                 << new QStandardItem(type)
                 << new QStandardItem(location)
                 << new QStandardItem(onlineStatus)
                 << new QStandardItem(QString::number(temperature, 'f', 2));
        model->appendRow(rowItems);

        // 更新温度历史，最多保存10个
        QVector<double> &history = temperatureHistory[deviceId];
        history.append(temperature);
        if (history.size() > 10)
            history.removeFirst();
    }

    updateChart();
}

void PageDevice::updateChart()
{
    // 设备列表，顺序与series匹配，需要保证deviceColors里顺序或键值稳定
    QStringList keys = deviceColors.keys();

    // Helper lambda更新单条曲线
    auto updateSeries = [&](QtCharts::QLineSeries *series, const QString &devId) {
        series->clear();
        const auto &temps = temperatureHistory.value(devId);
        for (int i = 0; i < temps.size(); ++i) {
            series->append(i, temps[i]);
        }
    };

    if(keys.size() >= 3) {
        updateSeries(seriesDevice1, keys[0]);
        updateSeries(seriesDevice2, keys[1]);
        updateSeries(seriesDevice3, keys[2]);
    }
    // 自动调整Y轴范围以包容所有温度数据
    double minTemp = 1000, maxTemp = -1000;
    for(const QString &devId : keys.mid(0,3)) {
        for(double temp : temperatureHistory[devId]){
            if(temp < minTemp) minTemp = temp;
            if(temp > maxTemp) maxTemp = temp;
        }
    }
    if(minTemp >= maxTemp) {
        minTemp = 0;
        maxTemp = 100;
    }
    auto axisY = qobject_cast<QtCharts::QValueAxis*>(chart->axisY());
    if(axisY){
        axisY->setRange(minTemp - 5, maxTemp + 5);
    }
}




