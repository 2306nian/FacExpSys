#include "videochat.h"
#include "ui_videochat.h"

VideoChat::VideoChat(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::VideoChat)
{
    ui->setupUi(this);
}

VideoChat::~VideoChat()
{
    delete ui;
}
