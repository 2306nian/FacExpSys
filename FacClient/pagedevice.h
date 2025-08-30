#ifndef PAGEDEVICE_H
#define PAGEDEVICE_H

#include <QWidget>
#include <QtCharts>
#include <QChartView>
#include <QLineSeries>

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

    // 标签控件 - 显示当前值
    QLabel *temperatureLabel;
    QLabel *humidityLabel;
    QLabel *pressureLabel;
    QLabel *temperatureValueLabel;
    QLabel *humidityValueLabel;
    QLabel *pressureValueLabel;

    // 图表控件
    QChartView *temperatureChartView;
    QChartView *pressureChartView;
    QLineSeries *temperatureSeries;
    QLineSeries *pressureSeries;

    // 初始化UI
    void setupDeviceUI();
    // 初始化图表
    void setupCharts();
    // 生成模拟数据
    void generateDemoData();
};

#endif // PAGEDEVICE_H
