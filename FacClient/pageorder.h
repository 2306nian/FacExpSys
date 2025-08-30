#ifndef PAGEORDER_H
#define PAGEORDER_H

#include <QWidget>
#include<chatroom.h>

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

private:
    Ui::PageOrder *ui;
    ChatRoom *ch1;
};

#endif // PAGEORDER_H
