/*
 * Copyright © 2012, Jolla Ltd.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "debug.h"
#include "sessionagent.h"
#include "session.h"

/*
This class is used to run a connman session.
Example:
    SessionAgent *sessionAgent = new SessionAgent("/ConnmanSessionAgent",this);
    connect(sessionAgent,SIGNAL(settingsUpdated(QVariantMap)),
            this,SLOT(sessionSettingsUpdated(QVariantMap)));

    sessionAgent->registerSession(); // you MUST call this!

    sessionAgent->setAllowedBearers(QStringList() << "wifi" << "ethernet" << "cellular");
    sessionAgent->requestConnect();

    There can be multiple sessions.

  */

SessionAgent::SessionAgent(const QString &path, QObject* parent) :
    QObject(parent),
    agentPath(path),
    m_session(0)
{
    m_manager = new Manager("net.connman", "/", QDBusConnection::systemBus(), this);
    const QString sessionmode = "SessionMode";
    m_manager->SetProperty(sessionmode, QDBusVariant(true));
}

SessionAgent::~SessionAgent()
{
    m_manager->DestroySession(QDBusObjectPath(agentPath));
}

void SessionAgent::setAllowedBearers(const QStringList &bearers)
{
    if (!m_session)
        return;
    QVariantMap map;
    map.insert("AllowedBearers",  qVariantFromValue(bearers));
    m_session->Change("AllowedBearers",QDBusVariant(bearers));
}

void SessionAgent::setConnectionType(const QString &type)
{
    if (!m_session)
        return;
    QVariantMap map;
    map.insert("ConnectionType",  qVariantFromValue(type));
    m_session->Change("ConnectionType",QDBusVariant(type));
}

void SessionAgent::registerSession()
{
    if (m_manager->isValid()) {
        QDBusPendingReply<QDBusObjectPath> obpath = m_manager->CreateSession(QVariantMap(),QDBusObjectPath(agentPath));
        m_session = new Session(obpath.value().path(), this);
        new SessionNotificationAdaptor(this);
        QDBusConnection::systemBus().registerObject(agentPath, this);
    }
}

void SessionAgent::requestConnect()
{
    if (m_session)
        m_session->Connect();
}

void SessionAgent::requestDisconnect()
{
    if (m_session)
        m_session->Disconnect();
}

void SessionAgent::requestDestroy()
{
    if (m_session)
        m_session->Destroy();
}

void SessionAgent::release()
{
    Q_EMIT released();
}

void SessionAgent::update(const QVariantMap &settings)
{
    Q_EMIT settingsUpdated(settings);
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
