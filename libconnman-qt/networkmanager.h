#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = 0);

signals:

public slots:

private slots:
	void setPropertyFinished(QDBusPendingCallWatcher *call);
};

#endif // NETWORKMANAGER_H
