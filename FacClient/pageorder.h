#ifndef PAGEORDER_H
#define PAGEORDER_H

#include <QWidget>
#include<chatroom.h>
#include<QJsonArray>
#include"session.h"
#include"ordersdetail.h"
#include"senddetails.h"

namespace Ui {
class PageOrder;
}

class PageOrder : public QWidget
{
    Q_OBJECT

public:
    explicit PageOrder(QWidget *parent = nullptr);
    ~PageOrder();
    void addOrderRow(const QString& orderNumber, const QString& orderName);
private slots:
    void on_pushButton_clicked();

    void getInitialOrders(const QJsonArray &json_arr);

    void onRowDoubleClicked(int row, int column);


    void on_pushButton_3_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::PageOrder *ui;
    ChatRoom *ch1;
    QJsonArray j_arr;


};

#endif // PAGEORDER_H
