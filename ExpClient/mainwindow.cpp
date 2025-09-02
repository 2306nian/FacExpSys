#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "session.h"  // 确保能访问 Session
#include <QMessageBox>
#include"clientcore.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    currentTicket="";//初始化
    connect(g_session, &Session::tableInitial,
            this, &MainWindow::onTableInitial);
    connect(g_session, &Session::addTicket,
            this, &MainWindow::onAddTicket);
    connect(this,&MainWindow::acceptTicketSend,
            g_session,&Session::handleAcceptTicket
            );
    connect(this,&MainWindow::completeTicketSend,g_session,&Session::handleCompleteTicket);
    connect(g_session, &Session::confirmCompleted,
            this, &MainWindow::onConfirmCompleted);
    connect(this,&MainWindow::create_chatroom,g_session,&Session::create_ChatRoom);
}


MainWindow::~MainWindow()
{
    delete ui;
}
void Session::create_ChatRoom(){
    emit createChatRoom();
}

void MainWindow::on_joinTicket_clicked()//点击按钮触发
{
    emit create_chatroom();
}
void MainWindow::on_acceptTicket_clicked()
{
    // 1. 获取选中行
    QModelIndexList selection = ui->tableWidget->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择一条工单！");
        return;
    }
    int row = selection.first().row();
    QTableWidgetItem *item = ui->tableWidget->item(row, 0); // 假设工单号在第0列
    if (!item || item->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, "提示", "选中的工单号无效！");
        return;
    }
    QString ticketId = item->text().trimmed();
    g_session->setTickedId(ticketId);
    currentTicket=ticketId;//设置当前工单号

    // 2. 构造 JSON 数据
    QJsonObject data{
        {"type", "accept_ticket"},
        {"data", QJsonObject{{"ticket_id", ticketId},
                             {"username",g_username}}}
    };
    qDebug()<<data;
    // 4. 发出信号
    emit acceptTicketSend(data);
}

void MainWindow::onTableInitial(const Session *,QJsonArray &data)
{
 qDebug() << "onTableInitial 被调用，data size:" << data.size(); // 添加这行

    // 清空现有数据（通过ui指针访问表格）
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);

    // 设置表头
    ui->tableWidget->setColumnCount(4);
    ui->tableWidget->setHorizontalHeaderLabels({"工单号", "用户名", "时间", "设备号"});

    // 填充数据
    for (int i = 0; i < data.size(); ++i) {
        QJsonObject obj = data[i].toObject();
        if (obj.isEmpty()) continue;

        // 插入新行
        ui->tableWidget->insertRow(i);

        // 工单号（第0列）
        QTableWidgetItem *idItem = new QTableWidgetItem(obj["ticket_id"].toString());
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, 0, idItem);

        // 用户名（第1列）
        QTableWidgetItem *userItem = new QTableWidgetItem(obj["username"].toString());
        userItem->setFlags(userItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, 1, userItem);

        // 时间（第2列）
        QTableWidgetItem *timeItem = new QTableWidgetItem(obj["created_at"].toString());
        timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, 2, timeItem);

        // 设备号（第3列）—— 正确处理 JSON 数组
        QJsonValue deviceVal = obj["device_ids"];
        QString deviceText;

        if (deviceVal.isArray()) {
            QJsonArray deviceArray = deviceVal.toArray();
            QStringList deviceList;
            for (const QJsonValue &v : deviceArray) {
                deviceList << v.toString();
            }
            deviceText = deviceList.join(", ");  // 拼接成 "D001, D002, D003"
        } else {
            deviceText = "";  // 安全兜底
        }

        QTableWidgetItem *deviceItem = new QTableWidgetItem(deviceText);
        deviceItem->setFlags(deviceItem->flags() & ~Qt::ItemIsEditable);
        deviceItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        ui->tableWidget->setItem(i, 3, deviceItem);
    }

    // 列宽设置：工单名占比最大
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);

    // 表格样式优化
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->setShowGrid(true);
    ui->tableWidget->verticalHeader()->setVisible(false);
}
void MainWindow::onAddTicket(const Session *,const QJsonObject &data)
{
    if (data.isEmpty()) return;
    qDebug()<<"增量更新";
    // 获取工单号，确保数据有效性
    QString ticketId = data["ticket_id"].toString();
    if (ticketId.isEmpty()) return;

    // 在表格开头添加新行
    int newRow = 0;
    ui->tableWidget->insertRow(newRow);

    // 工单号（第0列）
    QTableWidgetItem *idItem = new QTableWidgetItem(ticketId);
    idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
    ui->tableWidget->setItem(newRow, 0, idItem);

    // 用户名（第1列）
    QTableWidgetItem *userItem = new QTableWidgetItem(data["username"].toString());
    userItem->setFlags(userItem->flags() & ~Qt::ItemIsEditable);
    ui->tableWidget->setItem(newRow, 1, userItem);

    // 时间（第2列）
    QTableWidgetItem *timeItem = new QTableWidgetItem(data["created_at"].toString());
    timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
    ui->tableWidget->setItem(newRow, 2, timeItem);

    // 设备ID列表（第3列）
    QJsonArray deviceIds = data["device_ids"].toArray();
    QString deviceText;
    for (int i = 0; i < deviceIds.size(); ++i) {
        if (i > 0) deviceText += ", ";
        deviceText += deviceIds[i].toString();
    }
    QTableWidgetItem *deviceItem = new QTableWidgetItem(deviceText);
    deviceItem->setFlags(deviceItem->flags() & ~Qt::ItemIsEditable);
    deviceItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    deviceItem->setToolTip(deviceText); // 鼠标悬停显示完整内容
    ui->tableWidget->setItem(newRow, 3, deviceItem);

    // 自动滚动到新添加的行
    ui->tableWidget->scrollToItem(ui->tableWidget->item(newRow, 0));
    ui->tableWidget->viewport()->update();
    QApplication::processEvents(); // 处理待处理的事件
}
void MainWindow::on_completeTicket_clicked()
{
    if (currentTicket.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先承接工单！");
        return;
    }

    // 构造 JSON 数据
    QJsonObject dataObj;
    dataObj["ticket_id"] = currentTicket;
    dataObj["description"] = "This is a default description.";   // 固定文本
    dataObj["solution"] = "This is a default solution.";         // 固定文本

    QJsonObject request;
    request["type"] = "complete_ticket";
    request["data"] = dataObj;

    // 发出信号，通知 Session 发送
    emit completeTicketSend(request);
}
void MainWindow::onConfirmCompleted(const QString &ticketId)
{
    QTableWidget *table = ui->tableWidget;  // 替换为你的表格指针

    // 从最后一行开始遍历（避免删除时索引错乱）
    for (int row = table->rowCount() - 1; row >= 0; --row) {
        QTableWidgetItem *item = table->item(row, 0);  // 假设 ticketId 在第0列
        if (item && item->text() == ticketId) {
            table->removeRow(row);
            qDebug() << "已删除工单：" << ticketId;
            return;
        }
    }

    // 可选：未找到时输出日志
    qDebug() << "未找到要删除的工单：" << ticketId;
}

