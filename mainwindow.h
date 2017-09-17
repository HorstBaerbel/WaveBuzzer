#pragma once

#include "joystick.h"
#include "videowidget.h"

#include <QtWidgets/QMainWindow>
#include <QtCore/QSignalMapper>
#include <QtCore/QTimer>
#include <QtCore/QTime>
#include <QtGui/QColor>
#include <QtGui/QMovie>
#include <QtMultimedia/QSoundEffect>

#include <memory>
#include <mutex>


namespace Ui
{
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

    enum ApplicationState { WAITING_FOR_START, WAITING_FOR_STOP, SHOW_VIDEO };

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

protected:
	virtual void keyReleaseEvent(QKeyEvent * event);

protected Q_SLOTS:
	void selectJoystick(int index);
	void showMenu(const QPoint & position);
    void incdecScore(unsigned int index, int value);
	void startCounter();
	void stopCounter();
	void counterUpdate();
    void setTargetTime(int timeInS);

private:
	void resetToDefaultState();
    void updateScores();
	void updateTeams();

    ApplicationState m_applicationState = WAITING_FOR_START;
    std::mutex m_applicationStateMutex;
    bool m_wantFullscreen = true;

    std::unique_ptr<Joystick> m_joystick;
    QSignalMapper * m_actionMapper = nullptr;

    struct Team {
        int score;
        bool visible;
    };
    std::vector<Team> m_teams = {{0, true}, {0, true}, {0, true}, {0, true}};

	QTimer counterTimer;
	QTime counterTime;
    int m_timerDurationInS = 10;
	QString m_progressStyle;

    QMovie m_lorbeerMovie;
	QSoundEffect m_buzzerSound;
	QSoundEffect m_clockSound;
	QSoundEffect m_hirschSound1;
	QSoundEffect m_hirschSound2;
    QSoundEffect m_hirschSound3;
	
    VideoWidget * m_videoWidget = nullptr;
	Ui::MainWindow *m_ui = nullptr;
};
