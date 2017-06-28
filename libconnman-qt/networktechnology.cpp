/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2017 Jolla Ltd.
 * Contact: Slava Monich <slava.monich@jolla.com>
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0. The full text of the Apache License
 * is at http://www.apache.org/licenses/LICENSE-2.0
 */

#include "networktechnology.h"
#include "connman_technology_interface.h"
#include "libconnman_p.h"

const QString NetworkTechnology::Name("Name");
const QString NetworkTechnology::Type("Type");
const QString NetworkTechnology::Powered("Powered");
const QString NetworkTechnology::Connected("Connected");
const QString NetworkTechnology::IdleTimeout("IdleTimeout");
const QString NetworkTechnology::Tethering("Tethering");
const QString NetworkTechnology::TetheringIdentifier("TetheringIdentifier");
const QString NetworkTechnology::TetheringPassphrase("TetheringPassphrase");

NetworkTechnology::NetworkTechnology(const QString &path, const QVariantMap &properties, QObject* parent)
  : QObject(parent),
    m_technology(NULL)
{
    Q_ASSERT(!path.isEmpty());
    m_propertiesCache = properties;
    init(path);
}

NetworkTechnology::NetworkTechnology(QObject* parent)
    : QObject(parent),
      m_technology(NULL)
{
}

NetworkTechnology::~NetworkTechnology()
{
}

void NetworkTechnology::init(const QString &path)
{
    if (path != m_path) {
        m_path = path;

        delete m_technology;
        m_technology = 0;

        // Clear the property cache (only) if the path is becoming empty.
        if (m_path.isEmpty()) {
            QStringList keys = m_propertiesCache.keys();
            m_propertiesCache.clear();
            Q_EMIT pathChanged(m_path);
            const int n = keys.count();
            for (int i=0; i<n; i++) {
                emitPropertyChange(keys.at(i), QVariant());
            }
            return;
        }

        // Emit the signal after creating NetConnmanTechnologyInterface
        m_technology = new NetConnmanTechnologyInterface(CONNMAN_SERVICE, path,
                                                         CONNMAN_BUS, this);
        Q_EMIT pathChanged(m_path);

        connect(m_technology,
                SIGNAL(PropertyChanged(QString,QDBusVariant)),
                SLOT(propertyChanged(QString,QDBusVariant)));

        connect(new QDBusPendingCallWatcher(m_technology->GetProperties(), m_technology),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(getPropertiesFinished(QDBusPendingCallWatcher*)));
    }
}

void NetworkTechnology::getPropertiesFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QVariantMap> reply = *call;
    call->deleteLater();

    if (!reply.isError()) {
        QVariantMap tmpCache = reply.value();
        Q_FOREACH(const QString &name, tmpCache.keys()) {
            m_propertiesCache.insert(name, tmpCache[name]);
            emitPropertyChange(name, tmpCache[name]);
        }
        Q_EMIT propertiesReady();
    } else {
        qWarning() << reply.error().message();
    }
}

// Public API

// Getters

QString NetworkTechnology::path() const
{
    return m_path;
}

QString NetworkTechnology::name() const
{
    return m_propertiesCache.value(Name).toString();
}

QString NetworkTechnology::type() const
{
    return m_propertiesCache.value(Type).toString();
}

bool NetworkTechnology::powered() const
{
    return m_propertiesCache.value(Powered).toBool();
}

bool NetworkTechnology::connected() const
{
    return m_propertiesCache.value(Connected).toBool();
}

QString NetworkTechnology::objPath() const
{
    if (m_technology)
        return m_technology->path();
    return QString();
}

quint32 NetworkTechnology::idleTimeout() const
{
    return m_propertiesCache.value(IdleTimeout).toUInt();
}

bool NetworkTechnology::tethering() const
{
    return m_propertiesCache.value(Tethering).toBool();
}

QString NetworkTechnology::tetheringId() const
{
    return m_propertiesCache.value(TetheringIdentifier).toString();
}

QString NetworkTechnology::tetheringPassphrase() const
{
    return m_propertiesCache.value(TetheringPassphrase).toString();
}

// Setters

void NetworkTechnology::setPowered(const bool &powered)
{
    if (m_technology)
        m_technology->SetProperty(Powered, QDBusVariant(QVariant(powered)));
}

void NetworkTechnology::setPath(const QString &path)
{
    init(path);
}

void NetworkTechnology::setIdleTimeout(quint32 timeout)
{
    if (m_technology)
        m_technology->SetProperty(IdleTimeout, QDBusVariant(QVariant(timeout)));
}

void NetworkTechnology::setTethering(bool b)
{
    if (m_technology)
        m_technology->SetProperty(Tethering, QDBusVariant(QVariant(b)));
}

void NetworkTechnology::setTetheringId(const QString &id)
{
    if (m_technology)
        m_technology->SetProperty(TetheringIdentifier, QDBusVariant(QVariant(id)));
}

void NetworkTechnology::setTetheringPassphrase(const QString &pass)
{
    if (m_technology)
        m_technology->SetProperty(TetheringPassphrase, QDBusVariant(QVariant(pass)));
}

// Private

void NetworkTechnology::scan()
{
    if (!m_technology)
        return;

    QDBusPendingReply<> reply = m_technology->Scan();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, m_technology);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(scanReply(QDBusPendingCallWatcher*)));
}

void NetworkTechnology::emitPropertyChange(const QString &name, const QVariant &value)
{
    if (name == Powered) {
        Q_EMIT poweredChanged(value.toBool());
    } else if (name == Connected) {
        Q_EMIT connectedChanged(value.toBool());
    } else if (name == IdleTimeout) {
        Q_EMIT idleTimeoutChanged(value.toUInt());
    } else if (name == Tethering) {
        Q_EMIT tetheringChanged(value.toBool());
    } else if (name == TetheringIdentifier) {
        Q_EMIT tetheringIdChanged(value.toString());
    } else if (name == TetheringPassphrase) {
        Q_EMIT tetheringPassphraseChanged(value.toString());
    } else if (name == Name) {
        Q_EMIT nameChanged(value.toString());
    } else if (name == Type) {
        Q_EMIT typeChanged(value.toString());
    }
}

void NetworkTechnology::propertyChanged(const QString &name, const QDBusVariant &value)
{
    QVariant tmp = value.variant();

    Q_ASSERT(m_technology);

    m_propertiesCache[name] = tmp;
    emitPropertyChange(name,tmp);
}

void NetworkTechnology::scanReply(QDBusPendingCallWatcher *call)
{
    Q_EMIT scanFinished();

    call->deleteLater();
}
