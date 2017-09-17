#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtCore/QString>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QWidgetAction>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QMenu>
#include <QtMultimedia/QAudioDeviceInfo>


MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
    , m_actionMapper(new QSignalMapper(this))
    , m_ui(new Ui::MainWindow)
{
    // set up UI
    m_ui->setupUi(this);
    setWindowState(Qt::WindowFullScreen);
    // set up progress bar style
	m_progressStyle = QString("QProgressBar { background: #333; border-radius: 20px; } QProgressBar::chunk{ background: QLinearGradient(x1 : 0, y1 : 0, x2 : 0, y2 : 1, stop : 0 %1, stop: 0.4 %2, stop: 1 %3); border-radius: 20px;}");
    // load gif "movie"
    m_lorbeerMovie.setFileName(":/lorbeer.gif");
    m_lorbeerMovie.start();
    // set up all sound effects
	m_buzzerSound.setSource(QUrl::fromLocalFile(":/sounds/BUZZER_short.wav"));
	m_buzzerSound.setVolume(0.5);
	m_clockSound.setSource(QUrl::fromLocalFile(":/sounds/clock_ticking.wav"));
    m_clockSound.setLoopCount(QSoundEffect::Infinite);
    m_hirschSound1.setSource(QUrl::fromLocalFile(":/sounds/hirsch_haha1.wav"));
    m_hirschSound2.setSource(QUrl::fromLocalFile(":/sounds/hirsch1.wav"));
    m_hirschSound3.setSource(QUrl::fromLocalFile(":/sounds/hirsch3.wav"));
    // build video widget
    //m_videoWidget = new VideoWidget(this);
    // reset to default and activate first connected joystick
    resetToDefaultState();
    selectJoystick(0);
    // connect context menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QMainWindow::customContextMenuRequested, this, &MainWindow::showMenu);
}

MainWindow::~MainWindow()
{
    m_lorbeerMovie.stop();
    delete m_ui;
}

void MainWindow::showMenu(const QPoint & /*position*/)
{
    std::lock_guard<std::mutex> locker(m_applicationStateMutex);
    if (m_applicationState == WAITING_FOR_START)
    {
        // check if joystick connected to system
        const auto joysticks = Joystick::enumerateJoysticks();
        QMenu menu;
        if (joysticks.size() == 0)
        {
            // no. add message
            QAction * labelAction = menu.addAction(tr("Keine Joysticks gefunden"));
            labelAction->setEnabled(false);
        }
        else
        {
            // yes. add list of joysticks
            m_actionMapper->disconnect();
            QAction * labelAction = menu.addAction(tr("Joysticks:"));
            labelAction->setEnabled(false);
            for (const auto & info : joysticks)
            {
                const unsigned int index = std::get<0>(info);
                const bool connected = std::get<2>(info);
                QAction * action = menu.addAction(std::get<1>(info));
                action->setEnabled(connected);
                action->setCheckable(true);
                action->setChecked(m_joystick && m_joystick->index() == index);
                connect(action, QOverload<bool>::of(&QAction::triggered), m_actionMapper, QOverload<>::of(&QSignalMapper::map));
                m_actionMapper->setMapping(action, (int)index);
            }
            connect(m_actionMapper, QOverload<int>::of(&QSignalMapper::mapped), this, &MainWindow::selectJoystick);
        }
        // add spinbox to change timer duration
        QAction * labelAction = menu.addAction(tr("Timerdauer:"));
        labelAction->setEnabled(false);
        auto wa = new QWidgetAction(&menu);
        auto sb = new QSpinBox(&menu);
        sb->setRange(1, 600);
        sb->setValue(m_timerDurationInS);
        sb->setSuffix("s");
        connect(sb, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &MainWindow::setTargetTime);
        wa->setDefaultWidget(sb);
        menu.addAction(wa);
        menu.exec(QCursor::pos());
    }
}

void MainWindow::setTargetTime(int timeInS)
{
    m_timerDurationInS = timeInS;
}

void MainWindow::selectJoystick(int index)
{
    // only change joystick if in default state and if joystick changed
    if (m_applicationState == WAITING_FOR_START)
    {
        if (m_joystick == nullptr || m_joystick->index() != (unsigned int)index)
        {
            resetToDefaultState();
            m_joystick.reset(new Joystick(index));
			m_joystick->open();
            connect(m_joystick.get(), &Joystick::buttonsPressed, this, &MainWindow::startCounter);
        }
    }
}

void MainWindow::incdecScore(unsigned int index, int value)
{
    m_teams[index].score += value;
}

void MainWindow::keyReleaseEvent(QKeyEvent * event)
{
    const bool isEditingATeam = focusWidget() != nullptr && qobject_cast<QLineEdit*>(focusWidget()) != nullptr;
    const bool shiftPressed = QApplication::keyboardModifiers() & Qt::ShiftModifier;
    const auto key = event->key();
    // now check which key was pressed
    if (key == Qt::Key_Return)
	{
        m_ui->lineEditTeamA->clearFocus();
        m_ui->lineEditTeamB->clearFocus();
        m_ui->lineEditTeamC->clearFocus();
        m_ui->lineEditTeamD->clearFocus();
	}
    else if (key == Qt::Key_F1)
    {
        m_teams[0].visible = !m_teams[0].visible;
        updateTeams();
    }
    else if (key == Qt::Key_F2)
    {
        m_teams[1].visible = !m_teams[1].visible;
		updateTeams();
    }
    else if (key == Qt::Key_F3)
    {
        m_teams[2].visible = !m_teams[2].visible;
		updateTeams();
    }
    else if (key == Qt::Key_F4)
    {
        m_teams[3].visible = !m_teams[3].visible;
		updateTeams();
    }
    else if (key == Qt::Key_F11)
    {   m_wantFullscreen = !m_wantFullscreen;
        auto currentState = windowState();
        currentState.setFlag(Qt::WindowFullScreen, m_wantFullscreen);
        setWindowState(currentState);
	}
    else if (!isEditingATeam)
    {
        if (key == Qt::Key_V)
        {
            m_applicationStateMutex.lock();
            /*if (m_applicationState == WAITING_FOR_START)
            {
                m_applicationState = SHOW_VIDEO;
                auto currentState = windowState();
                currentState.setFlag(Qt::WindowFullScreen, false);
                setWindowState(currentState);
                m_videoWidget->playFile("lorbeer.mp4", true);
                m_applicationStateMutex.unlock();
            }
            else*/ if (m_applicationState == SHOW_VIDEO)
            {
                m_applicationState = WAITING_FOR_START;
                m_videoWidget->stop();
                auto currentState = windowState();
                currentState.setFlag(Qt::WindowFullScreen, m_wantFullscreen);
                setWindowState(currentState);
                m_applicationStateMutex.unlock();
                resetToDefaultState();
                selectJoystick(0);
            }
            else
            {
                m_applicationStateMutex.unlock();
            }
        }
        else if (key == Qt::Key_Space)
        {
            m_applicationStateMutex.lock();
            if (m_applicationState == WAITING_FOR_START)
            {
                m_applicationStateMutex.unlock();
                // replacement for first joystick press
                startCounter();
            }
            else if (m_applicationState == WAITING_FOR_STOP)
            {
                m_applicationStateMutex.unlock();
                // replacement for second joystick press
                stopCounter();
            }
            else
            {
                m_applicationStateMutex.unlock();
            }
        }
        else if (key == Qt::Key_0)
        {
            for (auto & team : m_teams)
            {
                team.score = 0;
            }
            updateScores();
        }
        else if (key == Qt::Key_1 || (shiftPressed && key == Qt::Key_Exclam))
        {
            incdecScore(0, shiftPressed ? -1 : 1);
            updateScores();
        }
        else if (key == Qt::Key_2 || (shiftPressed && key == Qt::Key_QuoteDbl))
        {
            incdecScore(1, shiftPressed ? -1 : 1);
            updateScores();
        }
        else if (key == Qt::Key_3 || (shiftPressed && key == Qt::Key_section))
        {
            incdecScore(2, shiftPressed ? -1 : 1);
            updateScores();
        }
        else if (key == Qt::Key_4 || (shiftPressed && key == Qt::Key_Dollar))
        {
            incdecScore(3, shiftPressed ? -1 : 1);
            updateScores();
        }
        else if (key == Qt::Key_H)
        {
            m_hirschSound1.play();
        }
        else if (key == Qt::Key_J)
        {
            m_hirschSound2.play();
        }
        else if (key == Qt::Key_K)
        {
            m_hirschSound3.play();
        }
    }
}

void MainWindow::resetToDefaultState()
{
    // stop video if it is still playing
    if (m_applicationState == SHOW_VIDEO)
    {
        m_videoWidget->stop();
    }
    m_applicationState = WAITING_FOR_START;
    // stop timer and disconnect it from slots
    counterTimer.stop();
    counterTimer.disconnect();
    // disconnect from stop method and connect to slot starting counting
    if (m_joystick)
    {
        disconnect(m_joystick.get(), &Joystick::buttonsPressed, this, &MainWindow::stopCounter);
        connect(m_joystick.get(), &Joystick::buttonsPressed, this, &MainWindow::startCounter);
    }
    m_clockSound.stop();
    // switch to score display
    m_ui->stackedWidget->setCurrentWidget(m_ui->pageScores);
    // reset GUI
    m_ui->labelCurrentTime->setText("0.0");
	m_ui->progressBar->setValue(0);
	m_ui->progressBar->setStyleSheet("QProgressBar { background: #333; border-radius: 20px; } QProgressBar::chunk{ background: QLinearGradient(x1 : 0, y1 : 0, x2 : 0, y2 : 1, stop : 0 #91ff91, stop: 0.4 #2dff2d, stop: 1 #008c00); border-radius: 20px;}");
	m_ui->lineEditTeamA->clearFocus();
	m_ui->lineEditTeamB->clearFocus();
	m_ui->lineEditTeamC->clearFocus();
	m_ui->lineEditTeamD->clearFocus();
}

void MainWindow::startCounter()
{
    std::lock_guard<std::mutex> locker(m_applicationStateMutex);
    if (m_applicationState == WAITING_FOR_START)
	{
        // new state is that we're waiting for the user to press again
        m_applicationState = WAITING_FOR_STOP;
        // disconnect from this method and connect to slot stopping counting
        if (m_joystick)
        {
            disconnect(m_joystick.get(), &Joystick::buttonsPressed, this, &MainWindow::startCounter);
            connect(m_joystick.get(), &Joystick::buttonsPressed, this, &MainWindow::stopCounter);
        }
        // switch to timer display and play sound
        m_ui->stackedWidget->setCurrentWidget(m_ui->pageTimer);
		m_buzzerSound.play();
		// save current time
		counterTime.start();
		// set up counter and connect to slot to update counter display every 100ms
        connect(&counterTimer, &QTimer::timeout, this, &MainWindow::counterUpdate);
		counterTimer.setInterval(100);
		counterTimer.start();
		// start clock ticking
		m_clockSound.play();
	}
}

void MainWindow::stopCounter()
{
    std::lock_guard<std::mutex> locker(m_applicationStateMutex);
    if (m_applicationState == WAITING_FOR_STOP)
	{
        // play buzzer sound
		m_buzzerSound.play();
        // reset to default state
		resetToDefaultState();
	}
}

void MainWindow::counterUpdate()
{
    // update counter label
    const int targetMs = m_timerDurationInS * 1000;
    const int elapsedMs = counterTime.elapsed();
    int msDigit = (elapsedMs / 100) % 10;
    int secs = (elapsedMs / 1000) % 60;
    int mins = (elapsedMs / (60 * 1000)) % 60;
    QString time = mins > 0 ? QString("%1:%2.%3").arg(mins).arg(secs, 2, 10, QChar('0')).arg(msDigit, 1) : QString("%1.%2").arg(secs).arg(msDigit, 1);
    m_ui->labelCurrentTime->setText(time);
	// update the progress
    const int progress = 100 - (std::max(0, targetMs - elapsedMs) * 100) / targetMs;
	m_ui->progressBar->setValue(progress);
	const int hue = ((100 - progress) * 120) / 100;
	QColor c0 = QColor::fromHsl(hue, 255, 200);
	QColor c1 = QColor::fromHsl(hue, 255, 150);
	QColor c2 = QColor::fromHsl(hue, 255, 70);
	QString newStyle = m_progressStyle.arg(c0.name(), c1.name(), c2.name());
	m_ui->progressBar->setStyleSheet(newStyle);
	// check if we need to stop the timer
    if (elapsedMs >= targetMs)
    {
        stopCounter();
    }
}

void MainWindow::updateScores()
{
    m_ui->labelTeamAScore->setText(QString::number(m_teams[0].score));
    m_ui->labelTeamBScore->setText(QString::number(m_teams[1].score));
    m_ui->labelTeamCScore->setText(QString::number(m_teams[2].score));
    m_ui->labelTeamDScore->setText(QString::number(m_teams[3].score));
    // find highest score from visible teams
	int maxScore = 0;
    for (const auto & team : m_teams)
	{
        if (team.visible)
        {
            maxScore = team.score > maxScore ? team.score : maxScore;
        }
	}
    // only mark the winner if only one visible team has the highest score
    int count = std::count_if(m_teams.cbegin(), m_teams.cend(), [&maxScore](const Team & team){ return team.visible && team.score >= maxScore; });
	if (count == 1)
	{
        auto wIt = std::find_if(m_teams.cbegin(), m_teams.cend(), [&maxScore](const Team & team){ return team.visible && team.score >= maxScore; });
        if (wIt != m_teams.cend())
		{
            int index = wIt - m_teams.cbegin();
			if (index == 0)
			{
                m_ui->labelLorbeerTeamA->setMovie(&m_lorbeerMovie);
                m_ui->labelLorbeerTeamB->setPixmap(QPixmap());
                m_ui->labelLorbeerTeamC->setPixmap(QPixmap());
                m_ui->labelLorbeerTeamD->setPixmap(QPixmap());
			}
			else if (index == 1)
			{
                m_ui->labelLorbeerTeamB->setMovie(&m_lorbeerMovie);
                m_ui->labelLorbeerTeamA->setPixmap(QPixmap());
                m_ui->labelLorbeerTeamC->setPixmap(QPixmap());
                m_ui->labelLorbeerTeamD->setPixmap(QPixmap());
			}
			else if (index == 2)
			{
                m_ui->labelLorbeerTeamC->setMovie(&m_lorbeerMovie);
                m_ui->labelLorbeerTeamA->setPixmap(QPixmap());
                m_ui->labelLorbeerTeamB->setPixmap(QPixmap());
                m_ui->labelLorbeerTeamD->setPixmap(QPixmap());
			}
			else if (index == 3)
			{
                m_ui->labelLorbeerTeamD->setMovie(&m_lorbeerMovie);
                m_ui->labelLorbeerTeamA->setPixmap(QPixmap());
                m_ui->labelLorbeerTeamB->setPixmap(QPixmap());
                m_ui->labelLorbeerTeamC->setPixmap(QPixmap());
			}
		}
	}
    else
    {
        // clear all lorbeers
        m_ui->labelLorbeerTeamA->setPixmap(QPixmap());
        m_ui->labelLorbeerTeamB->setPixmap(QPixmap());
        m_ui->labelLorbeerTeamC->setPixmap(QPixmap());
        m_ui->labelLorbeerTeamD->setPixmap(QPixmap());
    }
}

void MainWindow::updateTeams()
{
    m_ui->widgetTeamA->setVisible(m_teams[0].visible);
    m_ui->widgetTeamB->setVisible(m_teams[1].visible);
    m_ui->widgetTeamC->setVisible(m_teams[2].visible);
    m_ui->widgetTeamD->setVisible(m_teams[3].visible);
	updateScores();
}

