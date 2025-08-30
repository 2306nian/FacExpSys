#ifndef PAGEDEVICE_H
#define PAGEDEVICE_H

#include <QWidget>

namespace Ui {
class PageDevice;
}

class PageDevice : public QWidget
{
    Q_OBJECT

public:
    explicit PageDevice(QWidget *parent = nullptr);
    ~PageDevice();

private:
    Ui::PageDevice *ui;
};

#endif // PAGEDEVICE_H
