#include "controlcount.h"
#include "ui_controlcount.h"
#include "session.h"
#include <QJsonObject>
#include <QByteArray>
#include<QJsonDocument>

controlcount::controlcount(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::controlcount)
{
    ui->setupUi(this);

    ui->comboBox->addItem("SIM_PLC_1001");
    ui->comboBox->addItem("SIM_SENSOR_2002");
    ui->comboBox->addItem("SIM_MOTOR_3003");
}

controlcount::~controlcount()
{
    delete ui;
}

void controlcount::on_pushButton_clicked()
{
    QString device_id=ui->comboBox->currentText();
    QJsonObject json{
        {"type","control_command"},
        {"data",QJsonObject{
                {"device_id",device_id},
                {"action","control"}
            }
    }
    };
    QJsonDocument doc(json);

    QByteArray byteArray = doc.toJson();
    g_session->sendMessage(byteArray);
}

