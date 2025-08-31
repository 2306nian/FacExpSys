#include "pageorder.h"
#include "ui_pageorder.h"
#include <QJsonObject>       // 添加这行
#include <QJsonArray>        // 添加这行
#include <QJsonDocument>     // 添加这行
#include <QDebug>            // 如果需要的话

PageOrder::PageOrder(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageOrder)
{
    ui->setupUi(this);
    ui->tableWidget->setColumnCount(2);
    QStringList headers;
    headers << "工单号" << "工单名";
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    // 设置表格样式
    ui->tableWidget->setShowGrid(true);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableWidget->setColumnWidth(0, 120);

    // 添加预设的历史工单数据
    addOrderRow("WO-2023001", "发动机装配工单");
    addOrderRow("WO-2023002", "液压系统检测");
    addOrderRow("WO-2023003", "电路板焊接任务");
    addOrderRow("WO-2023004", "钢结构喷涂处理");
    addOrderRow("WO-2023005", "精密零件加工");

}

void PageOrder::addOrderRow(const QString& orderNumber, const QString& orderName)
{
    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(orderNumber));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(orderName));
}

PageOrder::~PageOrder()
{
    delete ui;
}

void PageOrder::on_pushButton_clicked()
{
    QJsonObject json{
        {"type", "create_ticket"},
        {"data", QJsonObject{
                     {"device_ids", QJsonArray{"SIM_PLC_1001", "SIM_SENSOR_2002"}},
                     {"username", g_username}
                 }}
    };

    // 将JSON对象转换为JSON文档并发送
    QByteArray jsonData = QJsonDocument(json).toJson(QJsonDocument::Compact);

    // 使用全局Session发送消息
    if (g_session) {
        g_session->sendMessage(jsonData);
        qDebug() << "工单创建请求已发送";
    } else {
        qDebug() << "无法发送消息：Session未初始化";
    }
}


