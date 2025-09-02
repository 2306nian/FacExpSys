#include "pagedevice.h"
#include "ui_pagedevice.h"
#include <QStandardItemModel>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QDebug>

PageDevice::PageDevice(QWidget *parent)
    : QWidget(parent),
    ui(new Ui::PageDevice)
{
    ui->setupUi(this);
    connect(g_session, &Session::deviceDataArrayReceived, this, &PageDevice::updateDeviceData);
    // 表格模型初始化
    model = new QStandardItemModel(this);
    model->setColumnCount(7);
    model->setHeaderData(0, Qt::Horizontal, "Device ID");
    model->setHeaderData(1, Qt::Horizontal, "Name");
    model->setHeaderData(2, Qt::Horizontal, "Type");
    model->setHeaderData(3, Qt::Horizontal, "Pressure");
    model->setHeaderData(4, Qt::Horizontal, "Online Status");
    model->setHeaderData(5, Qt::Horizontal, "Temperature");
    model->setHeaderData(6,Qt::Horizontal,"Controlcount");
    ui->tableView->setModel(model);

    // 根据实际设备ID设置颜色
    deviceColors["SIM_PLC_1001"] = Qt::red;
    deviceColors["SIM_SENSOR_2002"] = Qt::green;
    deviceColors["SIM_MOTOR_3003"] = Qt::blue;

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

    // 创建三条曲线，分别对应三个设备
    seriesDevice1 = new QLineSeries();
    seriesDevice1->setName("SIM_PLC_1001");
    seriesDevice1->setColor(deviceColors.value("SIM_PLC_1001", Qt::red));

    seriesDevice2 = new QLineSeries();
    seriesDevice2->setName("SIM_SENSOR_2002");
    seriesDevice2->setColor(deviceColors.value("SIM_SENSOR_2002", Qt::green));

    seriesDevice3 = new QLineSeries();
    seriesDevice3->setName("SIM_MOTOR_3003");
    seriesDevice3->setColor(deviceColors.value("SIM_MOTOR_3003", Qt::blue));

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
    // 先保存当前所有设备的历史数据
    QMap<QString, QVector<double>> tempHistory = temperatureHistory;

    model->removeRows(0, model->rowCount());  // 清空旧数据
    qDebug() << "Received devices:" << devices;

    // 遍历设备数组填充表格，并更新温度历史
    for (int i = 0; i < devices.size(); ++i) {
        QJsonObject dev = devices[i].toObject();
        QString deviceId = dev["device_id"].toString();
        double temperature = dev["temperature"].toDouble();
        double pressure=dev["pressure"].toDouble();
        int control_count=dev["control_count"].toInt();

        QString name;
        QString type;
        if(i==0){
            name="主控PLC";
            type="PLC";
        }else if(i==1){
            name="温度传感器";
            type="Sensor";
        }else{
            name="主轴电机";
            type="Motor";
        }
        // 填表 - 修正字段名
        QList<QStandardItem*> rowItems;
        rowItems << new QStandardItem(deviceId)
                 << new QStandardItem(name) // 使用默认值
                 << new QStandardItem(type) // 修正字段名
                 << new QStandardItem(QString::number(pressure,'f',2)) // 修正字段名
                 << new QStandardItem(dev["status"].toString())
                 << new QStandardItem(QString::number(temperature, 'f', 2))
                 << new QStandardItem(QString::number(control_count));
        model->appendRow(rowItems);

        // 更新温度历史，最多保存10个
        if (temperature > 0) { // 只有当温度有效时才更新
            QVector<double> &history = temperatureHistory[deviceId];
            history.append(temperature);
            if (history.size() > 10)
                history.removeFirst();
        }
    }

    // 对于没有收到数据的设备，保持之前的历史数据
    for (const auto &deviceId : deviceColors.keys()) {
        if (!temperatureHistory.contains(deviceId) && tempHistory.contains(deviceId)) {
            temperatureHistory[deviceId] = tempHistory[deviceId];
        }
    }

    qDebug() << "Temperature history:" << temperatureHistory;
    updateChart();
}

void PageDevice::updateChart()
{
    // 使用实际的设备ID列表
    QStringList deviceIds = {"SIM_PLC_1001", "SIM_SENSOR_2002", "SIM_MOTOR_3003"};

    // 更新三条曲线
    auto updateSeries = [&](QtCharts::QLineSeries *series, const QString &devId) {
        series->clear();
        const auto &temps = temperatureHistory.value(devId);
        for (int i = 0; i < temps.size(); ++i) {
            series->append(i, temps[i]);
        }
        qDebug() << "Series" << devId << "points:" << series->points();
    };

    updateSeries(seriesDevice1, deviceIds[0]);
    updateSeries(seriesDevice2, deviceIds[1]);
    updateSeries(seriesDevice3, deviceIds[2]);

    // 自动调整Y轴范围
    double minTemp = 1000, maxTemp = -1000;
    bool hasData = false;

    for(const QString &devId : deviceIds) {
        for(double temp : temperatureHistory.value(devId)){
            if(temp > 0) { // 只考虑有效温度
                hasData = true;
                if(temp < minTemp) minTemp = temp;
                if(temp > maxTemp) maxTemp = temp;
            }
        }
    }

    if(!hasData || minTemp >= maxTemp) {
        minTemp = 0;
        maxTemp = 100;
    } else {
        // 添加一些边距
        double range = maxTemp - minTemp;
        minTemp = qMax(0.0, minTemp - range * 0.1);
        maxTemp = maxTemp + range * 0.1;
    }

    auto axisY = qobject_cast<QtCharts::QValueAxis*>(chart->axisY());
    if(axisY){
        axisY->setRange(minTemp, maxTemp);
    }

    // 刷新图表
    chart->update();
}




