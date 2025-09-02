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
    ui->comboBox->addItem("pending");
    ui->comboBox->addItem("in_progress");
    ui->comboBox->addItem("completed");
    connect(g_session,&Session::sendInitialOrder,this,&PageOrder::getInitialOrders);
    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked, this, &PageOrder::onRowDoubleClicked);
    // 设置表格样式
    ui->tableWidget->setShowGrid(true);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableWidget->setColumnWidth(0, 120);

}

void PageOrder::onRowDoubleClicked(int row, int column){
    QJsonObject js1=j_arr[row].toObject();
    QString t_id=js1["ticket_id"].toString();
    QString status=js1["status"].toString();
    QString time=js1["created_at"].toString();
    QJsonArray dev_arr=js1["device_ids"].toArray();

    QString device_id=dev_arr[0].toString();
    qDebug()<<js1<<js1["device_ids"]<<dev_arr<<dev_arr[0]<<device_id;
    QString desc=js1["description"].toString();
    ordersdetail *or_detail = new ordersdetail(t_id,status,time,device_id,desc);
    or_detail->show();
}

void PageOrder::getInitialOrders(const QJsonArray &json_arr){
    j_arr=json_arr;
    qDebug()<<j_arr[0].toObject()["ticket_id"];
    for(int i=0;i<j_arr.size();i++){
        QJsonObject js1=j_arr[i].toObject();
        QString t_id=js1["ticket_id"].toString();
        QString status=js1["status"].toString();
        addOrderRow(t_id,status);
    }
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
    senddetails *senddtails = new senddetails();
    senddtails->show();
}



void PageOrder::on_pushButton_3_clicked()
{
    QString filterText = ui->comboBox->currentText();  // 获取下拉框选中的文本

    // 遍历所有行，显示或隐藏
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        QTableWidgetItem *item = ui->tableWidget->item(row, 1);  // 第2列（索引1）

        if (item && item->text() == filterText) {
            ui->tableWidget->setRowHidden(row, false);  // 显示匹配的行
        } else {
            ui->tableWidget->setRowHidden(row, true);   // 隐藏不匹配的行
        }
    }
}


void PageOrder::on_pushButton_2_clicked()
{
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        ui->tableWidget->setRowHidden(row, false);  // 显示所有行
    }
}

