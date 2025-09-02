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
    ordersdetail *or_detail = new ordersdetail(t_id,status,time);
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


