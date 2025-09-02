#ifndef CONTROLCOUNT_H
#define CONTROLCOUNT_H

#include <QWidget>

namespace Ui {
class controlcount;
}

class controlcount : public QWidget
{
    Q_OBJECT

public:
    explicit controlcount(QWidget *parent = nullptr);
    ~controlcount();

private slots:
    void on_pushButton_clicked();

private:
    Ui::controlcount *ui;
};

#endif // CONTROLCOUNT_H
