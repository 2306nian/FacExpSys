#include "ordersdetail.h"
#include "ui_ordersdetail.h"
#include "session.h"
#include "globaldatas.h"

ordersdetail::ordersdetail(QString ticket_id,QString status,QString created_at,QString device_id,QString description,QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ordersdetail)
{
    ui->setupUi(this);

    ui->label->setText(ticket_id);
    ui->label_2->setText(status);
    ui->label_3->setText(created_at);
    ui->label_4->setText(device_id);
    ui->label_5->setText(description);

    connect(this,&ordersdetail::createChatByOrder,g_session,&Session::startChatRoom);
}

ordersdetail::~ordersdetail()
{
    delete ui;
}

void ordersdetail::on_pushButton_clicked()
{
    emit createChatByOrder();
}

