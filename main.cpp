#include "mainwindow.h"
#include <QtWidgets/QApplication>

#define APPLICATION_NAME "WaveBuzzer"
#define APPLICATION_VERSION "0.9.1"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MainWindow w;
    w.setWindowTitle(QStringLiteral(APPLICATION_NAME) + QStringLiteral(" ") + QStringLiteral(APPLICATION_VERSION));
	w.show();

	return a.exec();
}
