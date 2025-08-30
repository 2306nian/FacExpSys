#include "pagedevice.h"
#include "ui_pagedevice.h"
#include <QGridLayout>

PageDevice::PageDevice(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageDevice)
{
    ui->setupUi(this);

    // 设置UI和图表
    setupDeviceUI();
    setupCharts();
    generateDemoData();
}

PageDevice::~PageDevice()
{
    delete ui;
}

void PageDevice::setupDeviceUI()
{
    // 创建标签
    temperatureLabel = new QLabel("温度:", this);
    humidityLabel = new QLabel("湿度:", this);
    pressureLabel = new QLabel("压力:", this);

    // 创建值标签
    temperatureValueLabel = new QLabel("25.3 °C", this);
    humidityValueLabel = new QLabel("45 %", this);
    pressureValueLabel = new QLabel("101.3 kPa", this);

    // 创建图表视图
    temperatureChartView = new QChartView(this);
    pressureChartView = new QChartView(this);

    // 设置图表视图大小策略
    temperatureChartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    pressureChartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 设置防锯齿
    temperatureChartView->setRenderHint(QPainter::Antialiasing);
    pressureChartView->setRenderHint(QPainter::Antialiasing);

    // 创建主布局
    QGridLayout *mainLayout = new QGridLayout(this);

    // 添加控件到布局
    // 第一行：温度标签和值
    mainLayout->addWidget(temperatureLabel, 0, 0);
    mainLayout->addWidget(temperatureValueLabel, 0, 1);

    // 第二行：温度图表
    mainLayout->addWidget(temperatureChartView, 1, 0, 1, 2);

    // 第三行：湿度标签和值
    mainLayout->addWidget(humidityLabel, 2, 0);
    mainLayout->addWidget(humidityValueLabel, 2, 1);

    // 第四行：压力标签和值
    mainLayout->addWidget(pressureLabel, 3, 0);
    mainLayout->addWidget(pressureValueLabel, 3, 1);

    // 第五行：压力图表
    mainLayout->addWidget(pressureChartView, 4, 0, 1, 2);

    // 设置列伸展因子
    mainLayout->setColumnStretch(0, 1);
    mainLayout->setColumnStretch(1, 2);

    // 设置布局
    setLayout(mainLayout);
}

void PageDevice::setupCharts()
{
    // 创建温度数据系列
    temperatureSeries = new QLineSeries();
    temperatureSeries->setName("温度 (°C)");

    // 创建压力数据系列
    pressureSeries = new QLineSeries();
    pressureSeries->setName("压力 (kPa)");

    // 创建温度图表
    QChart *temperatureChart = new QChart();
    temperatureChart->addSeries(temperatureSeries);
    temperatureChart->setTitle("温度历史记录");
    temperatureChart->legend()->setVisible(true);
    temperatureChart->legend()->setAlignment(Qt::AlignBottom);

    // 创建压力图表
    QChart *pressureChart = new QChart();
    pressureChart->addSeries(pressureSeries);
    pressureChart->setTitle("压力历史记录");
    pressureChart->legend()->setVisible(true);
    pressureChart->legend()->setAlignment(Qt::AlignBottom);

    // 设置坐标轴
    QValueAxis *axisX1 = new QValueAxis();
    axisX1->setRange(0, 4);
    axisX1->setTitleText("测量次数");

    QValueAxis *axisY1 = new QValueAxis();
    axisY1->setTitleText("温度 (°C)");

    QValueAxis *axisX2 = new QValueAxis();
    axisX2->setRange(0, 4);
    axisX2->setTitleText("测量次数");

    QValueAxis *axisY2 = new QValueAxis();
    axisY2->setTitleText("压力 (kPa)");

    // 添加坐标轴到图表
    temperatureChart->addAxis(axisX1, Qt::AlignBottom);
    temperatureChart->addAxis(axisY1, Qt::AlignLeft);
    temperatureSeries->attachAxis(axisX1);
    temperatureSeries->attachAxis(axisY1);

    pressureChart->addAxis(axisX2, Qt::AlignBottom);
    pressureChart->addAxis(axisY2, Qt::AlignLeft);
    pressureSeries->attachAxis(axisX2);
    pressureSeries->attachAxis(axisY2);

    // 设置图表视图
    temperatureChartView->setChart(temperatureChart);
    pressureChartView->setChart(pressureChart);
}

void PageDevice::generateDemoData()
{
    // 清除现有数据
    temperatureSeries->clear();
    pressureSeries->clear();

    // 温度模拟数据 (近5次)
    temperatureSeries->append(0, 22.5);
    temperatureSeries->append(1, 23.1);
    temperatureSeries->append(2, 22.8);
    temperatureSeries->append(3, 23.4);
    temperatureSeries->append(4, 25.3); // 当前值

    // 压力模拟数据 (近5次)
    pressureSeries->append(0, 101.3);
    pressureSeries->append(1, 101.5);
    pressureSeries->append(2, 101.4);
    pressureSeries->append(3, 101.2);
    pressureSeries->append(4, 101.3); // 当前值

    // 根据数据设置Y轴范围
    temperatureChartView->chart()->axes(Qt::Vertical).first()->setRange(22, 26);
    pressureChartView->chart()->axes(Qt::Vertical).first()->setRange(101, 102);

    // 更新显示的当前值
    temperatureValueLabel->setText(QString::number(25.3) + " °C");
    humidityValueLabel->setText(QString::number(45) + " %");
    pressureValueLabel->setText(QString::number(101.3) + " kPa");
}

