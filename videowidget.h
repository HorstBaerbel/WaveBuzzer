#pragma once

#include <QtCore/QObject>
#include <QtMultimediaWidgets/QVideoWidget>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QMediaPlaylist>


class VideoWidget : public QVideoWidget
{
    Q_OBJECT

public:
    VideoWidget(QWidget * parent = nullptr);
    ~VideoWidget();

    void playFile(const QString & fileName, bool fullScreen = false);
    void stop();

protected slots:
    void playAgain(QMediaPlayer::MediaStatus status);

private:
    double m_repeatPosition = 0.0;
    QMediaPlayer * m_player = nullptr;
    QMediaPlaylist * m_playlist = nullptr;
};
