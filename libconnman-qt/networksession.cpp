/*
 * Copyright © 2012, Jolla Ltd
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "networksession.h"
#include "sessionagent.h"

class NetworkSessionPrivate
{
public:
    NetworkSessionPrivate();

    SessionAgent *m_sessionAgent;
    QVariantMap settingsMap;
    QString m_path;
};

NetworkSessionPrivate::NetworkSessionPrivate()
    : m_sessionAgent(0)
    , m_path("/ConnmanQmlSessionAgent")
{
}

NetworkSession::NetworkSession(QObject *parent)
    : QObject(parent)
    , d_ptr(new NetworkSessionPrivate)
{
    createSession();
}

NetworkSession::~NetworkSession()
{
}

void NetworkSession::createSession()
{
    if (d_ptr->m_path.isEmpty())
        return;

    delete d_ptr->m_sessionAgent;
    d_ptr->m_sessionAgent = new SessionAgent(d_ptr->m_path, this);
    connect(d_ptr->m_sessionAgent, SIGNAL(settingsUpdated(QVariantMap)),
            this, SLOT(sessionSettingsUpdated(QVariantMap)));
}

QString NetworkSession::state() const
{
    return d_ptr->settingsMap.value("State").toString();
}

QString NetworkSession::name() const
{
    return d_ptr->settingsMap.value("Name").toString();
}

QString NetworkSession::bearer() const
{
    return d_ptr->settingsMap.value("Bearer").toString();
}

QString NetworkSession::sessionInterface() const
{
    return d_ptr->settingsMap.value("Interface").toString();
}

QVariantMap NetworkSession::ipv4() const
{
    return qdbus_cast<QVariantMap>(d_ptr->settingsMap.value("IPv4"));
}

QVariantMap NetworkSession::ipv6() const
{
    return qdbus_cast<QVariantMap>(d_ptr->settingsMap.value("IPv6"));
}

QStringList NetworkSession::allowedBearers() const
{
    return d_ptr->settingsMap.value("AllowedBearers").toStringList();
}

QString NetworkSession::connectionType() const
{
    return d_ptr->settingsMap.value("ConnectionType").toString();
}

void NetworkSession::setAllowedBearers(const QStringList &bearers)
{
    d_ptr->settingsMap.insert("AllowedBearers", QVariant::fromValue(bearers));
    d_ptr->m_sessionAgent->setAllowedBearers(bearers);
}

void NetworkSession::setConnectionType(const QString &type)
{
    d_ptr->settingsMap.insert("ConnectionType", QVariant::fromValue(type));
    d_ptr->m_sessionAgent->setConnectionType(type);
}

void NetworkSession::requestDestroy()
{
    d_ptr->m_sessionAgent->requestDestroy();
}

void NetworkSession::requestConnect()
{
    d_ptr->m_sessionAgent->requestConnect();
}

void NetworkSession::requestDisconnect()
{
    d_ptr->m_sessionAgent->requestDisconnect();
}

void NetworkSession::sessionSettingsUpdated(const QVariantMap &settings)
{
    for (const QString &name : settings.keys()) {
        d_ptr->settingsMap.insert(name, settings[name]);

        if (name == QLatin1String("State")) {
            Q_EMIT stateChanged(settings[name].toString());
        } else if (name == QLatin1String("Name")) {
            Q_EMIT nameChanged(settings[name].toString());
        } else if (name == QLatin1String("Bearer")) {
            Q_EMIT bearerChanged(settings[name].toString());
        } else if (name == QLatin1String("Interface")) {
            Q_EMIT sessionInterfaceChanged(settings[name].toString());
        } else if (name == QLatin1String("IPv4")) {
            Q_EMIT ipv4Changed(ipv4());
        } else if (name == QLatin1String("IPv6")) {
            Q_EMIT ipv6Changed(ipv6());
        } else if (name == QLatin1String("AllowedBearers")) {
            Q_EMIT allowedBearersChanged(allowedBearers());
        } else if (name == QLatin1String("ConnectionType")) {
            Q_EMIT connectionTypeChanged(settings[name].toString());
        }
    }
    Q_EMIT settingsChanged(settings);
}

QString NetworkSession::path() const
{
    return d_ptr->m_path;
}

void NetworkSession::setPath(const QString &path)
{
    if (path != d_ptr->m_path) {
        d_ptr->m_path = path;
        createSession();
    }
}
