
#include <QObject>

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QtWidgets>
#include <QQuickView>
#include <qqml.h>
#else
#include <QApplication>
#include <QGLWidget>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <qdeclarative.h>
#endif


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    QQuickView *view = new QQuickView;
    view->setResizeMode(QQuickView::SizeRootObjectToView);
#else
    QDeclarativeView *view = new QDeclarativeView;
    view->setViewport(new QGLWidget);
    view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
#endif
    view->setSource(QUrl::fromLocalFile("main.qml"));
    view->setGeometry(0,0,800,480);
    view->show();

    return a.exec();
}
