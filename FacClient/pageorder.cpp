#include "pageorder.h"
#include "ui_pageorder.h"

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
    ch1=new ChatRoom(this);
    ch1->show();
    ch1->resize(1300,900);
    this->hide();
}

