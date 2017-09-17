#include "Joystick.h"

#define UNICODE
#include <windows.h>
#include <mmsystem.h>
#include <regstr.h>


QString getJoystickName(const QString & registryKey, unsigned int joystickIndex)
{
    HKEY hKey;
    HKEY hTopKey = HKEY_LOCAL_MACHINE;
    LSTATUS result;
    WCHAR temp[256];
    DWORD tempSize;

    // check if the joystick configuration registry path exists and open it
    QString configPath = QString("%1\\%2\\%3").arg(QString::fromWCharArray(REGSTR_PATH_JOYCONFIG)).arg(registryKey).arg(QString::fromWCharArray(REGSTR_KEY_JOYCURR));
    if (RegOpenKeyEx(hTopKey, (LPCWSTR)configPath.utf16(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        //failed. try user settings
        hTopKey = HKEY_CURRENT_USER;
        if (RegOpenKeyEx(hTopKey, (LPCWSTR)configPath.utf16(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        {
            return QString();
        }
    }
    // find the key
    QString propertyKeyName = QString("Joystick%1%2").arg(joystickIndex + 1).arg(QString::fromWCharArray(REGSTR_VAL_JOYOEMNAME));
    tempSize = sizeof(temp);
    result = RegQueryValueEx(hKey, (LPCWSTR)propertyKeyName.utf16(), NULL, NULL, (LPBYTE)temp, &tempSize);
    RegCloseKey(hKey);
    if (result != ERROR_SUCCESS)
    {
        return QString();
    }
    // try to open the key we just got
    QString propertyKeyPath = QString("%1\\%2").arg(QString::fromWCharArray(REGSTR_PATH_JOYOEM)).arg(QString::fromWCharArray(temp));
    if (RegOpenKeyEx(hTopKey, (LPCWSTR)propertyKeyPath.utf16(), 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return QString();
    }
    // try to read OEM name value
    tempSize = sizeof(temp);
    result = RegQueryValueEx(hKey, REGSTR_VAL_JOYOEMNAME, NULL, NULL, (LPBYTE)temp, &tempSize);
    RegCloseKey(hKey);
    if (result != ERROR_SUCCESS)
    {
        return QString();
    }
    return QString::fromWCharArray(temp);
}

std::vector<bool> getButtonsPressed(unsigned int joystickIndex)
{
    std::vector<bool> buttons(32);
    JOYINFOEX joyinfoex;
    joyinfoex.dwSize = sizeof(JOYINFOEX);
    joyinfoex.dwFlags = JOY_RETURNBUTTONS;
    if (joyGetPosEx(JOYSTICKID1 + (UINT)joystickIndex, &joyinfoex) == JOYERR_NOERROR)
    {
        for (int i = 0; i < 32; ++i)
        {
            buttons[i] = joyinfoex.dwButtons & (1 << i);
        }
    }
    return buttons;
}

//-------------------------------------------------------------------------------------------------

Joystick::Joystick(unsigned int index, size_t pollIntervalms)
    : m_index(index)
    , m_exitPollThread(false)
	, m_threadRunning(false)
    , m_pollIntervalms(pollIntervalms)
    , m_currentButtons(32, false)
{
    qRegisterMetaType<std::vector<bool>>("std::vector<bool>");
    JOYCAPS joycaps;
    if (joyGetDevCaps(JOYSTICKID1 + (UINT)index, &joycaps, sizeof(JOYCAPS)) == JOYERR_NOERROR)
    {
        // get joystick name
        m_name = getJoystickName(QString::fromWCharArray(joycaps.szRegKey), index);
        // check if it is connected. get joystick position and button state
        m_currentlyConnected = isConnected();
    }
}

Joystick::~Joystick()
{
    close();
}

bool Joystick::open()
{
	if (!m_threadRunning)
	{
		m_exitPollThread = false;
		m_pollThread = std::thread(&Joystick::pollOrProcess, this);
	}
	return true;
}

void Joystick::close()
{
    if (m_threadRunning)
    {
        m_threadRunning = false;
        m_exitPollThread = true;
        m_pollThread.join();
    }
    m_currentButtons = std::vector<bool>(32, false);
    m_currentlyConnected = false;
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
	JOYINFOEX joyinfoex;
	joyinfoex.dwSize = sizeof(JOYINFOEX);
	joyinfoex.dwFlags = JOY_RETURNBUTTONS;
	return (joyGetPosEx(JOYSTICKID1 + (UINT)m_index, &joyinfoex) == JOYERR_NOERROR);
}

void Joystick::pollOrProcess()
{
	m_threadRunning = true;
    while (!m_exitPollThread)
    {
        // check if joystick connected
        const bool isNowConnected = isConnected();
        // check if connection setting changed
        if (m_currentlyConnected != isNowConnected)
        {
            m_currentlyConnected = isNowConnected;
            emit connectionChanged(m_currentlyConnected);
        }
        if (isNowConnected)
        {
            auto buttons = getButtonsPressed(m_index);
            // if one button is pressed, send signal
            for (int i = 0; i < buttons.size(); ++i)
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
            for (int i = 0; i < buttons.size(); ++i)
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
	const size_t wNumberOfAnnouncedDevs = (size_t)joyGetNumDevs();
	if (wNumberOfAnnouncedDevs > 0)
	{
        // loop through announced joysticks
		for (unsigned int i = 0; i < wNumberOfAnnouncedDevs; ++i)
		{
            // get joystick information
            JOYCAPS joycaps;
            if (joyGetDevCaps(JOYSTICKID1 + (UINT)i, &joycaps, sizeof(JOYCAPS)) == JOYERR_NOERROR)
            {
                // check if connected
                JOYINFOEX joyinfoex;
                joyinfoex.dwSize = sizeof(JOYINFOEX);
                joyinfoex.dwFlags = JOY_RETURNBUTTONS;
                const bool connected = joyGetPosEx(JOYSTICKID1 + (UINT)i, &joyinfoex) == JOYERR_NOERROR;
                info.push_back(std::tuple<unsigned int, QString, bool>(i, getJoystickName(QString::fromWCharArray(joycaps.szRegKey), i), connected));
            }
		}
	}
	return info;
}
