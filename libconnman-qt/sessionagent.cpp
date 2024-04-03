/*
 * Copyright © 2012, Jolla Ltd.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "sessionagent.h"
#include "connman_session_interface.h"

/*
This class is used to run a connman session.
Example:
    SessionAgent *sessionAgent = new SessionAgent("/ConnmanSessionAgent",this);
    connect(sessionAgent,SIGNAL(settingsUpdated(QVariantMap)),
            this,SLOT(sessionSettingsUpdated(QVariantMap)));
    sessionAgent->setAllowedBearers(QStringList() << "wifi" << "ethernet" << "cellular");
    sessionAgent->requestConnect();

    There can be multiple sessions.

  */

class SessionAgentPrivate
{
public:
    SessionAgentPrivate(const QString &path);

    QString agentPath;
    QVariantMap sessionSettings;
    QSharedPointer<NetworkManager> m_manager;
    NetConnmanSessionInterface *m_session;
};

SessionAgentPrivate::SessionAgentPrivate(const QString &path)
    : agentPath(path)
    , m_manager(NetworkManager::sharedInstance())
    , m_session(nullptr)
{
}

SessionAgent::SessionAgent(const QString &path, QObject *parent)
    : QObject(parent)
    , d_ptr(new SessionAgentPrivate(path))
{
    createSession();
}

SessionAgent::~SessionAgent()
{
    d_ptr->m_manager->destroySession(d_ptr->agentPath);

    delete d_ptr;
    d_ptr = nullptr;
}

void SessionAgent::setAllowedBearers(const QStringList &bearers)
{
    if (!d_ptr->m_session)
        return;

    QVariantMap map;
    map.insert("AllowedBearers",  QVariant::fromValue(bearers));
    QDBusPendingReply<> reply = d_ptr->m_session->Change("AllowedBearers", QDBusVariant(bearers));
    // hope this is not a lengthy task
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << Q_FUNC_INFO << reply.error();
    }
}

void SessionAgent::setConnectionType(const QString &type)
{
    if (!d_ptr->m_session)
        return;

    QVariantMap map;
    map.insert("ConnectionType", QVariant::fromValue(type));
    d_ptr->m_session->Change("ConnectionType", QDBusVariant(type));
}

void SessionAgent::createSession()
{
    if (d_ptr->m_manager->isAvailable()) {
        QDBusObjectPath objectPath = d_ptr->m_manager->createSession(QVariantMap(), d_ptr->agentPath);

        if (!objectPath.path().isEmpty()) {
            d_ptr->m_session = new NetConnmanSessionInterface("net.connman", objectPath.path(),
                                                              QDBusConnection::systemBus(), this);
            new SessionNotificationAdaptor(this);
            QDBusConnection::systemBus().unregisterObject(d_ptr->agentPath);
            if (!QDBusConnection::systemBus().registerObject(d_ptr->agentPath, this)) {
                qDebug() << "Could not register agent object";
            }
        } else {
            qDebug() << "agentPath is not valid" << d_ptr->agentPath;
        }
    } else {
        qDebug() << Q_FUNC_INFO << "manager not valid";
    }
}

void SessionAgent::requestConnect()
{
    if (d_ptr->m_session) {
        QDBusPendingReply<> reply = d_ptr->m_session->Connect();
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                this, SLOT(onConnectFinished(QDBusPendingCallWatcher*)));
    }
}

void SessionAgent::requestDisconnect()
{
    if (d_ptr->m_session)
        d_ptr->m_session->Disconnect();
}

void SessionAgent::requestDestroy()
{
    if (d_ptr->m_session)
        d_ptr->m_session->Destroy();
}

void SessionAgent::release()
{
    Q_EMIT released();
}

void SessionAgent::update(const QVariantMap &settings)
{
    Q_EMIT settingsUpdated(settings);
}

void SessionAgent::onConnectFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<> reply = *call;
    if (reply.isError())
        qDebug() << reply.error().message();

    call->deleteLater();
}

SessionNotificationAdaptor::SessionNotificationAdaptor(SessionAgent* parent)
    : QDBusAbstractAdaptor(parent),
      m_sessionAgent(parent)
{
}

SessionNotificationAdaptor::~SessionNotificationAdaptor() {}

void SessionNotificationAdaptor::Release()
{
    m_sessionAgent->release();
}

void SessionNotificationAdaptor::Update(const QVariantMap &settings)
{
    m_sessionAgent->update(settings);
}
