#ifndef SENDDETAILS_H
#define SENDDETAILS_H

#include <QWidget>

namespace Ui {
class senddetails;
}

class senddetails : public QWidget
{
    Q_OBJECT

public:
    explicit senddetails(QWidget *parent = nullptr);
    ~senddetails();

private:
    Ui::senddetails *ui;


private slots:
    void on_pushButton_clicked();
};

#endif // SENDDETAILS_H
