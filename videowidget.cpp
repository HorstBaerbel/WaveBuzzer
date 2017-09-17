#include "videowidget.h"

#include <QtCore/QFileInfo>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget>


VideoWidget::VideoWidget(QWidget * parent)
    : QVideoWidget(parent)
{
    setAspectRatioMode(Qt::KeepAspectRatio);
    setAttribute(Qt::WA_OpaquePaintEvent);
    m_player = new QMediaPlayer(this);
    m_player->setVideoOutput(this);
}

VideoWidget::~VideoWidget()
{
    stop();
}

void VideoWidget::playAgain(QMediaPlayer::MediaStatus status)
{
    if (QMediaPlayer::EndOfMedia == status)
    {
        m_player->play();
    }
}

void VideoWidget::playFile(const QString & fileName, bool fullScreen)
{
    m_player->stop();
    m_player->setMedia(QUrl::fromLocalFile(QFileInfo(fileName).absoluteFilePath()));
    if (fullScreen)
    {
        QDesktopWidget * desktopWidget = QApplication::desktop();
        if (desktopWidget)
        {
            auto screenNr = desktopWidget->screenNumber(parentWidget());
            QRect screenRect = desktopWidget->screenGeometry(screenNr);
            setGeometry(screenRect);
            setFixedSize(screenRect.size());
        }
        showFullScreen();
    }
    else
    {
        show();
    }
    m_player->play();
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &VideoWidget::playAgain);
}

void VideoWidget::stop()
{
    disconnect(m_player, &QMediaPlayer::mediaStatusChanged, this, &VideoWidget::playAgain);
    hide();
    m_player->stop();
}
