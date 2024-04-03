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

#include <QDBusPendingReply>
#include <QWeakPointer>

namespace  {
const auto Name = QStringLiteral("Name");
const auto Type = QStringLiteral("Type");
const auto Powered = QStringLiteral("Powered");
const auto Connected = QStringLiteral("Connected");
const auto IdleTimeout = QStringLiteral("IdleTimeout");
const auto Tethering = QStringLiteral("Tethering");
const auto TetheringIdentifier = QStringLiteral("TetheringIdentifier");
const auto TetheringPassphrase = QStringLiteral("TetheringPassphrase");
}

class TechnologyTracker: public QObject {
    Q_OBJECT
public:
    static QSharedPointer<TechnologyTracker> instance();
    TechnologyTracker();

    QSet<QString> technologies();

signals:
    void technologyAdded(const QString &technology);
    void technologyRemoved(const QString &technology);

public slots:
    void onTechnologyAdded(const QDBusObjectPath &technology, const QVariantMap &properties);
    void onTechnologyRemoved(const QDBusObjectPath &technology);
    void getTechnologies();

private:
    QDBusServiceWatcher *m_dbusWatcher;
    QSet<QString> m_technologies;
};

QSharedPointer<TechnologyTracker> TechnologyTracker::instance()
{
    static QWeakPointer<TechnologyTracker> sharedTracker;

    QSharedPointer<TechnologyTracker> tracker = sharedTracker.toStrongRef();

    if (!tracker) {
        tracker = QSharedPointer<TechnologyTracker>::create();
        sharedTracker = tracker;
    }

    return tracker;
}

TechnologyTracker::TechnologyTracker()
    : QObject()
    , m_dbusWatcher(new QDBusServiceWatcher(CONNMAN_SERVICE, CONNMAN_BUS,
                                            QDBusServiceWatcher::WatchForRegistration
                                            | QDBusServiceWatcher::WatchForUnregistration, this))
{
    // Monitor connman itself.
    connect(m_dbusWatcher, &QDBusServiceWatcher::serviceRegistered,
            this, &TechnologyTracker::getTechnologies);
    connect(m_dbusWatcher, &QDBusServiceWatcher::serviceUnregistered,
            this, [this]() {
        for (const QString &technology : m_technologies) {
            Q_EMIT technologyRemoved(technology);
        }
        m_technologies.clear();
    });

    // Monitor TechnologyAdded and TechnologyRemoved.
    CONNMAN_BUS.connect(
                CONNMAN_SERVICE,
                "/",
                "net.connman.Manager",
                "TechnologyAdded",
                this,
                SLOT(onTechnologyAdded(QDBusObjectPath,QVariantMap)));

    CONNMAN_BUS.connect(
                CONNMAN_SERVICE,
                "/",
                "net.connman.Manager",
                "TechnologyRemoved",
                this,
                SLOT(onTechnologyRemoved(QDBusObjectPath)));

    getTechnologies();
}

QSet<QString> TechnologyTracker::technologies()
{
    return m_technologies;
}

void TechnologyTracker::getTechnologies()
{
    QDBusInterface managerInterface(CONNMAN_SERVICE, "/", "net.connman.Manager", CONNMAN_BUS);

    QDBusPendingCall pendingCall = managerInterface.asyncCall("GetTechnologies");

    connect(new QDBusPendingCallWatcher(pendingCall, this),
            &QDBusPendingCallWatcher::finished, [this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<ConnmanObjectList> reply = *watcher;
        watcher->deleteLater();

        if (reply.isError()) {
            qWarning() << "Failed to get connman techonologies:" << reply.isError() << reply.error().type();
        } else {
            for (const ConnmanObject &object : reply.value()) {
                QString technology = object.objpath.path();
                m_technologies.insert(technology);
                Q_EMIT technologyAdded(technology);
            }
        }
    });
}

void TechnologyTracker::onTechnologyAdded(const QDBusObjectPath &technology, const QVariantMap &properties)
{
    Q_UNUSED(properties);
    m_technologies.insert(technology.path());
    Q_EMIT technologyAdded(technology.path());
}

void TechnologyTracker::onTechnologyRemoved(const QDBusObjectPath &technology)
{
    m_technologies.remove(technology.path());
    Q_EMIT technologyRemoved(technology.path());
}


NetworkTechnology::NetworkTechnology(const QString &path, const QVariantMap &properties, QObject* parent)
  : QObject(parent)
  , m_technology(nullptr)
{
    Q_ASSERT(!path.isEmpty());
    m_propertiesCache = properties;

    initialize();
    setPath(path);
}

NetworkTechnology::NetworkTechnology(QObject* parent)
    : QObject(parent)
    , m_technology(nullptr)
{
    initialize();
}

NetworkTechnology::~NetworkTechnology()
{
    destroyInterface();
}

void NetworkTechnology::initialize()
{
    m_technologyTracker = TechnologyTracker::instance();
    connect(m_technologyTracker.data(), &TechnologyTracker::technologyRemoved,
            this, &NetworkTechnology::onInterfaceChanged);
    connect(m_technologyTracker.data(), &TechnologyTracker::technologyAdded,
            this, &NetworkTechnology::onInterfaceChanged);

    createInterface();
}

void NetworkTechnology::getPropertiesFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QVariantMap> reply = *call;
    call->deleteLater();

    if (!reply.isError()) {
        // Handle first pending values. If there would be more of these,
        // we could think of having a container of key, setter, and getter.
        // Over engineering for now.
        // Call pending update regardless of the actual pending value change but
        // emit changes only if there are real changes.

        QVariantMap tmpCache = reply.value();
        if (m_pendingProperties.contains(Powered)) {
            bool newValue = tmpCache[Powered].toBool();
            bool pendingValue = m_pendingProperties[Powered].toBool();
            setPowered(pendingValue);
            if (pendingValue == newValue) {
                m_pendingProperties.remove(Powered);
            }
        }

        if (m_pendingProperties.contains(IdleTimeout)) {
            quint32 newValue = tmpCache[IdleTimeout].toUInt();
            quint32 pendingValue = m_pendingProperties[IdleTimeout].toUInt();
            setIdleTimeout(pendingValue);
            if (pendingValue == newValue) {
                m_pendingProperties.remove(IdleTimeout);
            }
        }

        if (m_pendingProperties.contains(Tethering)) {
            bool newValue = tmpCache[Tethering].toBool();
            bool pendingValue = m_pendingProperties[Tethering].toBool();
            setTethering(pendingValue);
            if (pendingValue == newValue) {
                m_pendingProperties.remove(Tethering);
            }
        }

        if (m_pendingProperties.contains(TetheringIdentifier)) {
            QString newValue = tmpCache[TetheringIdentifier].toString();
            QString pendingValue = m_pendingProperties[TetheringIdentifier].toString();
            setTetheringId(pendingValue);
            if (pendingValue == newValue) {
                m_pendingProperties.remove(TetheringIdentifier);
            }
        }

        if (m_pendingProperties.contains(TetheringPassphrase)) {
            QString newValue = tmpCache[TetheringPassphrase].toString();
            QString pendingValue = m_pendingProperties[TetheringPassphrase].toString();
            setTetheringPassphrase(pendingValue);
            if (pendingValue == newValue) {
                m_pendingProperties.remove(TetheringPassphrase);
            }
        }

        for (const QString &name : tmpCache.keys()) {
            if (!m_pendingProperties.contains(name)) {
                m_propertiesCache.insert(name, tmpCache[name]);
                emitPropertyChange(name, tmpCache[name]);
            }
        }
        m_pendingProperties.clear();
        Q_EMIT propertiesReady();
    } else {
        qWarning() << reply.error().message();
        m_propertiesCache.clear();
    }
}


void NetworkTechnology::pendingSetProperty(const QString &key, const QVariant &value)
{
    QDBusPendingCallWatcher *pendingCall
            = new QDBusPendingCallWatcher(m_technology->SetProperty(key, QDBusVariant(value)), m_technology);
    connect(pendingCall, &QDBusPendingCallWatcher::finished,
            [this, key, value](QDBusPendingCallWatcher *call) {
        QDBusPendingReply<QVariantMap> reply = *call;
        call->deleteLater();

        // If technology object is not yet registered update pending value accordingly.
        // This is merely a fallback mechnanism as technologies are also istened.
        if (reply.isError() && reply.error().type() == QDBusError::UnknownObject) {
            m_pendingProperties.insert(key, value);
        }
    });
}

void NetworkTechnology::createInterface()
{
    if (!m_path.isEmpty() && m_technologyTracker->technologies().contains(m_path)) {
        m_technology = new NetConnmanTechnologyInterface(CONNMAN_SERVICE, m_path, CONNMAN_BUS, this);

        connect(m_technology, &NetConnmanTechnologyInterface::PropertyChanged,
                this, &NetworkTechnology::propertyChanged);

        QDBusPendingCallWatcher *pendingCall = new QDBusPendingCallWatcher(m_technology->GetProperties(), m_technology);
        connect(pendingCall, &QDBusPendingCallWatcher::finished,
                this, &NetworkTechnology::getPropertiesFinished);
    }
}

void NetworkTechnology::destroyInterface()
{
    delete m_technology;
    m_technology = nullptr;
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

void NetworkTechnology::setPowered(bool powered)
{
    if (m_technology) {
        pendingSetProperty(Powered, QVariant(powered));
    } else {
        m_pendingProperties.insert(Powered, QVariant(powered));
    }
}

void NetworkTechnology::setPath(const QString &path)
{
    if (path == m_path) {
        return;
    }

    m_path = path;
    destroyInterface();

    if (!m_path.isEmpty()) {
        createInterface();
    } else {
        QStringList keys = m_propertiesCache.keys();
        m_propertiesCache.clear();
        const int n = keys.count();
        for (int i=0; i<n; i++) {
            emitPropertyChange(keys.at(i), QVariant());
        }
    }

    Q_EMIT pathChanged(m_path);
}

void NetworkTechnology::setIdleTimeout(quint32 timeout)
{
    if (m_technology)
        pendingSetProperty(IdleTimeout, QVariant(timeout));
    else
        m_pendingProperties.insert(IdleTimeout, QVariant(timeout));
}

void NetworkTechnology::setTethering(bool b)
{
    if (m_technology)
        pendingSetProperty(Tethering, QVariant(b));
    else
        m_pendingProperties.insert(Tethering, QVariant(b));
}

void NetworkTechnology::setTetheringId(const QString &id)
{
    if (m_technology)
        pendingSetProperty(TetheringIdentifier, QVariant(id));
    else
        m_pendingProperties.insert(TetheringIdentifier, QVariant(id));
}

void NetworkTechnology::setTetheringPassphrase(const QString &pass)
{
    if (m_technology)
        pendingSetProperty(TetheringPassphrase, QVariant(pass));
    else
        m_pendingProperties.insert(TetheringPassphrase, QVariant(pass));
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

void NetworkTechnology::onInterfaceChanged(const QString &interface)
{
    if (interface == m_path) {
        destroyInterface();
        createInterface();
    }
}

void NetworkTechnology::propertyChanged(const QString &name, const QDBusVariant &value)
{
    QVariant tmp = value.variant();

    Q_ASSERT(m_technology);
    m_propertiesCache[name] = tmp;
    emitPropertyChange(name, tmp);
}

void NetworkTechnology::scanReply(QDBusPendingCallWatcher *call)
{
    Q_EMIT scanFinished();

    call->deleteLater();
}

#include "networktechnology.moc"
