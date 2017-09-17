#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include <vector>
#include <vector>
#include <thread>
#include <atomic>


class Joystick : public QObject
{
    Q_OBJECT

public:
    Joystick(unsigned int index = 0, size_t pollIntervalms = 25);
    ~Joystick();

    bool open();
    void close();
    unsigned int index() const;
    bool isConnected() const;
    QString name() const;
    std::vector<bool> buttons() const;

    static std::vector<std::tuple<unsigned int, QString, bool>> enumerateJoysticks();

signals:
    void buttonsPressed(const std::vector<bool> & buttonIndex);
    void buttonsClicked(const std::vector<bool> & buttonIndex);
    void connectionChanged(bool connected);

private:
    void pollOrProcess();

    std::thread m_pollThread;
    std::atomic_bool m_exitPollThread;
	std::atomic_bool m_threadRunning;
    size_t m_pollIntervalms = 25;
    std::vector<bool> m_currentButtons;
    bool m_currentlyConnected = false;
    unsigned int m_index = 0;
    QString m_name;

    int m_fd = 0;
};
