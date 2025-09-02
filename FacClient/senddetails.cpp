#include "senddetails.h"
#include "ui_senddetails.h"
#include "globaldatas.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <qjsonobject.h>
#include "session.h"

senddetails::senddetails(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::senddetails)
{
    ui->setupUi(this);

    ui->comboBox->addItem("SIM_PLC_1001");
    ui->comboBox->addItem("SIM_SENSOR_2002");
    ui->comboBox->addItem("SIM_MOTOR_3003");

}

senddetails::~senddetails()
{
    delete ui;
}



void senddetails::on_pushButton_clicked()
{
    if(!ui->lineEdit->text().isEmpty()&&!ui->comboBox->currentText().isEmpty()){
        QString desc=ui->lineEdit->text();
        QString device_id=ui->comboBox->currentText();
        QJsonObject json{
            {"type", "create_ticket"},
            {"data", QJsonObject{
                         {"device_ids", QJsonArray{device_id}},
                         {"username", g_username},
                         {"description",desc}
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

}

