#include "page_mine.h"
#include "ui_page_mine.h"

Page_Mine::Page_Mine(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Page_Mine)
{
    ui->setupUi(this);
}

Page_Mine::~Page_Mine()
{
    delete ui;
}
