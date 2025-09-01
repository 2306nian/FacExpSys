#ifndef VIDEOCHAT_H
#define VIDEOCHAT_H

#include <QWidget>

namespace Ui {
class VideoChat;
}

class VideoChat : public QWidget
{
    Q_OBJECT

public:
    explicit VideoChat(QWidget *parent = nullptr);
    ~VideoChat();

private:
    Ui::VideoChat *ui;
};

#endif // VIDEOCHAT_H
