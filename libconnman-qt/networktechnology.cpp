/*
 * Copyright © 2010, Intel Corporation.
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "technology.h"
#include "networktechnology.h"
#include "debug.h"

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
    m_technology(NULL),
    m_path(QString())
{
    Q_ASSERT(!path.isEmpty());
    m_propertiesCache = properties;
    init(path);
}

NetworkTechnology::NetworkTechnology(QObject* parent)
    : QObject(parent),
      m_technology(NULL),
      m_path(QString())
{
}

NetworkTechnology::~NetworkTechnology()
{
}

void NetworkTechnology::init(const QString &path)
{
    m_path = path;

    if (m_technology) {
        delete m_technology;
        m_technology = 0;
        // TODO: After resetting the path iterate through old properties, compare their values
        //       with new ones and emit corresponding signals if changed.
        m_propertiesCache.clear();
    }
    m_technology = new Technology("net.connman", path, QDBusConnection::systemBus(), this);

    if (!m_technology->isValid()) {
        pr_dbg() << "Invalid technology: " << path;
        throw -1; // FIXME
    }

    if (m_propertiesCache.isEmpty()) {
        QDBusReply<QVariantMap> reply;
        reply = m_technology->GetProperties();
        m_propertiesCache = reply.value();
    }
    emit poweredChanged(powered());
    emit connectedChanged(connected());

    connect(m_technology,
            SIGNAL(PropertyChanged(const QString&, const QDBusVariant&)),
            this,
            SLOT(propertyChanged(const QString&, const QDBusVariant&)));

}

// Public API

// Getters

const QString NetworkTechnology::name() const
{
    if (m_propertiesCache.contains(NetworkTechnology::Name))
        return m_propertiesCache[NetworkTechnology::Name].toString();
    else
        return QString();
}

const QString NetworkTechnology::type() const
{
    if (m_propertiesCache.contains(NetworkTechnology::Type))
        return m_propertiesCache[NetworkTechnology::Type].toString();
    else
        return QString();
}

bool NetworkTechnology::powered() const
{
    if (m_propertiesCache.contains(NetworkTechnology::Powered))
        return m_propertiesCache[NetworkTechnology::Powered].toBool();
    else
        return false;
}

bool NetworkTechnology::connected() const
{
    if (m_propertiesCache.contains(NetworkTechnology::Connected))
        return m_propertiesCache[NetworkTechnology::Connected].toBool();
    else
        return false;
}

const QString NetworkTechnology::objPath() const
{
    Q_ASSERT(m_technology);

    return m_technology->path();
}

// Setters

void NetworkTechnology::setPowered(const bool &powered)
{
    Q_ASSERT(m_technology);
    m_technology->SetProperty(Powered, QDBusVariant(QVariant(powered)));
}

void NetworkTechnology::scan()
{
    Q_ASSERT(m_technology);

    QDBusPendingReply<> reply = m_technology->Scan();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, m_technology);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(scanReply(QDBusPendingCallWatcher*)));
}

// Private

void NetworkTechnology::propertyChanged(const QString &name, const QDBusVariant &value)
{
    QVariant tmp = value.variant();

    Q_ASSERT(m_technology);

    m_propertiesCache[name] = tmp;
    if (name == Powered) {
        emit poweredChanged(tmp.toBool());
    } else if (name == Connected) {
        emit connectedChanged(tmp.toBool());
    } else if (name == IdleTimeout) {
      emit idleTimeoutChanged(tmp.toUInt());
    } else if (name == Tethering) {
      emit tetheringChanged(tmp.toBool());
    } else if (name == TetheringIdentifier) {
      emit tetheringIdChanged(tmp.toString());
    } else if (name == TetheringPassphrase) {
      emit tetheringPassphraseChanged(tmp.toString());
    }
}

void NetworkTechnology::scanReply(QDBusPendingCallWatcher *call)
{
    emit scanFinished();

    call->deleteLater();
}

QString NetworkTechnology::path() const
{
    return m_path;
}

void NetworkTechnology::setPath(const QString &path)
{
    if (path != m_path) {
        init(path);
    }
}

quint32 NetworkTechnology::idleTimeout() const
{
    if (m_propertiesCache.contains(NetworkTechnology::IdleTimeout))
        return m_propertiesCache[NetworkTechnology::IdleTimeout].toUInt();
    else
        return 0;
}

void NetworkTechnology::setIdleTimeout(quint32 timeout)
{
    if (m_technology)
        m_technology->SetProperty(IdleTimeout, QDBusVariant(QVariant(timeout)));
}

bool NetworkTechnology::tethering() const
{
    if (m_propertiesCache.contains(NetworkTechnology::Tethering))
        return m_propertiesCache[NetworkTechnology::Tethering].toBool();
    else
        return false;
}

void NetworkTechnology::setTethering(bool b)
{
    if (m_technology)
        m_technology->SetProperty(Tethering, QDBusVariant(QVariant(b)));
}


QString NetworkTechnology::tetheringId() const
{
    if (m_propertiesCache.contains(NetworkTechnology::TetheringIdentifier))
        return m_propertiesCache[NetworkTechnology::TetheringIdentifier].toString();
    else
        return QString();
}

void NetworkTechnology::setTetheringId(const QString &id)
{
    if (m_technology)
        m_technology->SetProperty(TetheringIdentifier, QDBusVariant(QVariant(id)));
}

QString NetworkTechnology::tetheringPassphrase() const
{
    if (m_propertiesCache.contains(NetworkTechnology::TetheringPassphrase))
        return m_propertiesCache[NetworkTechnology::TetheringPassphrase].toString();
    else
        return QString();

}

void NetworkTechnology::setTetheringPassphrase(const QString &pass)
{
    if (m_technology)
        m_technology->SetProperty(TetheringPassphrase, QDBusVariant(QVariant(pass)));
}
