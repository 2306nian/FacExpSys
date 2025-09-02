#ifndef ORDERSDETAIL_H
#define ORDERSDETAIL_H

#include <QWidget>

namespace Ui {
class ordersdetail;
}

class ordersdetail : public QWidget
{
    Q_OBJECT

public:
    explicit ordersdetail(QString ticket_id,QString status,QString created_at,QWidget *parent = nullptr);
    ~ordersdetail();

private slots:
    void on_pushButton_clicked();

private:
    Ui::ordersdetail *ui;

signals:
    void createChatByOrder();
};

#endif // ORDERSDETAIL_H
