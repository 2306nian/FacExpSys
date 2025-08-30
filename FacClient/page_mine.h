#ifndef PAGE_MINE_H
#define PAGE_MINE_H

#include <QWidget>

namespace Ui {
class Page_Mine;
}

class Page_Mine : public QWidget
{
    Q_OBJECT

public:
    explicit Page_Mine(QWidget *parent = nullptr);
    ~Page_Mine();

private:
    Ui::Page_Mine *ui;
};

#endif // PAGE_MINE_H
