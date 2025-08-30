#include "pagedevice.h"
#include "ui_pagedevice.h"

PageDevice::PageDevice(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PageDevice)
{
    ui->setupUi(this);
}

PageDevice::~PageDevice()
{
    delete ui;
}
