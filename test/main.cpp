#include <QApplication>
#include <QWidget>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <qdeclarative.h>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	QDeclarativeView *view = new QDeclarativeView;

	view->setSource(QUrl::fromLocalFile("main.qml"));
	view->setGeometry(0,0,800,480);
	view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
	view->show();

    return a.exec();
}
