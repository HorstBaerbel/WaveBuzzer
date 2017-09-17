#include "joystick.h"

#include <QtCore/QDir>
#include <QtCore/QRegularExpression>

#include <unistd.h>
#include <fcntl.h>
#include <linux/joystick.h>


Joystick::Joystick(unsigned int index, size_t pollIntervalms)
    : m_index(index)
    , m_exitPollThread(false)
	, m_threadRunning(false)
    , m_pollIntervalms(pollIntervalms)
    , m_currentButtons(32, false)
{
    qRegisterMetaType<std::vector<bool>>("std::vector<bool>");
}

Joystick::~Joystick()
{
    close();
}

bool Joystick::open()
{
	if (!m_threadRunning)
	{
		QString jsDev = QString("/dev/input/js%1").arg(m_index);
		m_fd = ::open(jsDev.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
		if (m_fd > 0) {
		    char temp[256];
		    ioctl(m_fd, JSIOCGNAME(256), temp);
		    m_name.fromLocal8Bit((const char *)&temp);
		    m_exitPollThread = false;
		    m_pollThread = std::thread(&Joystick::pollOrProcess, this);
		    return true;
		}
	}
    return false;
}

void Joystick::close()
{
    if (m_threadRunning)
    {
        m_threadRunning = false;
        m_exitPollThread = true;
        m_pollThread.join();
    }
    if (m_fd > 0)
    {
        ::close(m_fd);
        m_fd = 0;
    }
    m_currentButtons = std::vector<bool>(32, false);
}

unsigned int Joystick::index() const
{
	return m_index;
}

QString Joystick::name() const
{
    return m_name;
}

std::vector<bool> Joystick::buttons() const
{
    return m_currentButtons;
}

bool Joystick::isConnected() const
{
    return m_fd != 0;
}

void Joystick::pollOrProcess()
{
	m_threadRunning = true;
    while (!m_exitPollThread)
    {
        if (m_fd > 0)
        {
            auto buttons = m_currentButtons;
            js_event jsEvent;
            while (read(m_fd, &jsEvent, sizeof(jsEvent)) > 0) {
                jsEvent.type &= ~JS_EVENT_INIT;
                if (jsEvent.type & JS_EVENT_BUTTON) {
                    buttons.at(jsEvent.number) = jsEvent.value;
                }
                /*if (jsEvent.type & JS_EVENT_AXIS) {
                    axis[jsEvent.number] = jsEvent.value;
                }*/
            }
            // if one button is pressed, send signal
            for (unsigned int i = 0; i < buttons.size(); ++i)
            {
                // check if button was not pressed before and is now pressed
                if (!m_currentButtons.at(i) && buttons.at(i))
                {
                    emit buttonsPressed(buttons);
                    break;
                }
            }
            // check if a button has been pressed and is now released
            std::vector<bool> clicked(buttons.size(), false);
            for (unsigned int i = 0; i < buttons.size(); ++i)
            {
                // check if button was pressed before and is now not pressed
                if (m_currentButtons.at(i) && !buttons.at(i))
                {
                    clicked[i] = true;
                }
            }
            // if at least one button has been clicked, send signal
            if (std::find(clicked.cbegin(), clicked.cend(), true) != clicked.cend())
            {
                emit buttonsClicked(clicked);
            }
            m_currentButtons = buttons;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(m_pollIntervalms));
    }
	m_threadRunning = false;
}

std::vector<std::tuple<unsigned int, QString, bool>> Joystick::enumerateJoysticks()
{
    std::vector<std::tuple<unsigned int, QString, bool>> info;
    static QRegularExpression jsDeviceExpr("js\\d+", QRegularExpression::CaseInsensitiveOption);
    static QString inputDir("/dev/input/");
    QDir dir("/dev/input");
    QStringList devices = dir.entryList(QDir::Files | QDir::System);
    for (QString deviceName : devices)
    {
        auto match = jsDeviceExpr.match(deviceName);
        if (match.hasMatch())
        {
            const QString devicePath = inputDir + deviceName;
            bool ok = false;
            uint number = deviceName.remove("js").toUInt(&ok);
            if (ok)
            {
                int jfd = ::open(devicePath.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
                if (jfd > 0) {
                    char temp[256];
                    ioctl(jfd, JSIOCGNAME(256), temp);
                    QString name = QString::fromLocal8Bit((const char *)&temp);
                    info.push_back(std::tuple<unsigned int, QString, bool>(number, name, true));
                    ::close(jfd);
                }
            }
        }
    }
    return info;
}

