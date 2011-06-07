#include "networkmanager.h"

#define SET_CONNMAN_PROPERTY(key, val) \
		if (!m_manager) { \
			qCritical("ClockModel: SetProperty: not connected to connman"); \
		} else { \
			QDBusPendingReply<> reply = m_manager->SetProperty(key, QDBusVariant(val)); \
			QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this); \
			connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), \
					this, SLOT(setPropertyFinished(QDBusPendingCallWatcher*))); \
		}

NetworkManager::NetworkManager(QObject *parent) :
    QObject(parent)
{
}


void NetworkManager::setPropertyFinished(QDBusPendingCallWatcher *call)
{
	QDBusPendingReply<> reply = *call;
	if (reply.isError()) {
		qCritical() << "NetworkManager: setProperty: " << reply.error().name() << reply.error().message();
	}
	call->deleteLater();
}
