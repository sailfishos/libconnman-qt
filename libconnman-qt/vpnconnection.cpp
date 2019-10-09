/*
 * Copyright (c) 2016 - 2019 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#include "marshalutils.h"

#include "vpnconnection.h"
#include "vpnconnection_p.h"

#define DEFAULT_PROPERTY_METHODS(TYPEIN, TYPEOUT, PROPERTY, NAME) \
    TYPEOUT VpnConnection:: NAME () const \
    { \
        Q_D(const VpnConnection); \
        return qvariant_cast< TYPEOUT >(d->m_properties.value(#NAME)); \
    } \
    void VpnConnection::set ## PROPERTY (const TYPEIN NAME) \
    { \
        Q_D(VpnConnection); \
        d->setProperty(#NAME, (NAME), &VpnConnection:: NAME ## Changed); \
    }

namespace {

const QString connmanService = QStringLiteral("net.connman");
const QString connmanVpnService = QStringLiteral("net.connman.vpn");
const QString autoConnectKey = QStringLiteral("AutoConnect");

QString vpnServicePath(const QString &connectionPath)
{
    return QString("/net/connman/service/vpn_%1").arg(connectionPath.section("/", 5));
}

} // Empty namespace

VpnConnectionPrivate::VpnConnectionPrivate(VpnConnection &qq, const QString &path)
    : m_connectionProxy(connmanVpnService, path, QDBusConnection::systemBus(), nullptr)
    , m_serviceProxy(connmanService, vpnServicePath(path), QDBusConnection::systemBus(), nullptr)
    , m_path(path)
    , m_autoConnect(false)
    , m_state(VpnConnection::Idle)
    , q_ptr(&qq)
{
}

void VpnConnectionPrivate::init()
{
    Q_Q(VpnConnection);

    m_properties.insert("path", m_path);

    QDBusPendingCall servicePropertiesCall = m_serviceProxy.GetProperties();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(servicePropertiesCall, q);
    VpnConnection::connect(watcher, &QDBusPendingCallWatcher::finished, q, [q, this](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<> reply = *watcher;
        if (reply.isFinished() && reply.isValid()) {
            QDBusMessage message = reply.reply();
            QVariantMap properties = MarshalUtils::demarshallArgument<QVariantMap>(message.arguments().value(0));
            bool autoConnect = properties.value(autoConnectKey).toBool();
            properties.clear();
            properties.insert(autoConnectKey, autoConnect);
            q->update(MarshalUtils::propertiesToQml(properties));
        } else {
            qDebug() << "Error :" << m_path << ":" << reply.error().message();
        }

        watcher->deleteLater();
    });

    VpnConnection::connect(&m_connectionProxy, &NetConnmanVpnConnectionInterface::PropertyChanged, q, [q](const QString &name, const QDBusVariant &value) {
        QVariantMap properties;
        qDebug() << "VPN connection property changed:" << name << value.variant() << q->path() << q->name();
        properties.insert(name, value.variant());
        q->update(MarshalUtils::propertiesToQml(properties));
    });

    VpnConnection::connect(&m_serviceProxy, &NetConnmanServiceInterface::PropertyChanged, q, [q](const QString &name, const QDBusVariant &value) {
        qDebug() << "VPN service property changed:" << name << value.variant() << q->path() << q->name();
        if (name == autoConnectKey) {
            QVariantMap properties;
            properties.insert(name, value.variant());
            q->update(MarshalUtils::propertiesToQml(properties));
        }
    });
}

VpnConnection::VpnConnection(QObject *parent)
    : QObject(parent)
    , d_ptr(new VpnConnectionPrivate(*this, ""))
{
    Q_D(VpnConnection);
    d->init();
}

VpnConnection::VpnConnection(const QString &path, QObject *parent)
    : QObject(parent)
    , d_ptr(new VpnConnectionPrivate(*this, path))
{
    Q_D(VpnConnection);
    d->init();
}

VpnConnection::VpnConnection(VpnConnectionPrivate &dd, QObject *parent)
    : QObject(parent)
    , d_ptr(&dd)
{
    Q_D(VpnConnection);
    d->init();
}

VpnConnection::~VpnConnection()
{
    // Destructor needed to handle the QScopedPointer<VpnConnectionPrivate>
    // Do nothing
}

void VpnConnection::modifyConnection(const QVariantMap &properties)
{
    Q_D(VpnConnection);

    qDebug() << "Updating VPN connection for modification:" << d->m_path;

    // Remove properties that connman doesn't know about
    QVariantMap updatedProperties(properties);
    updatedProperties.remove(QString("path"));
    updatedProperties.remove(QString("state"));
    updatedProperties.remove(QString("index"));
    updatedProperties.remove(QString("immutable"));
    updatedProperties.remove(QString("storeCredentials"));

    QVariantMap dbusProps = MarshalUtils::propertiesToDBus(updatedProperties);
    for (QMap<QString, QVariant>::const_iterator i = dbusProps.constBegin(); i != dbusProps.constEnd(); ++i) {
        d->m_connectionProxy.SetProperty(i.key(), QDBusVariant(i.value()));
    }
}

void VpnConnection::activate()
{
    Q_D(VpnConnection);

    QDBusPendingCall call = d->m_connectionProxy.Connect();
    qDebug() << "Connect to vpn" << d->m_path;

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [d](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<void> reply = *watcher;
        watcher->deleteLater();

        if (reply.isError()) {
            qDebug() << "Unable to activate Connman VPN connection:" << d->m_path << ":" << reply.error().message();
        }
    });
}

void VpnConnection::deactivate()
{
    Q_D(VpnConnection);

    QDBusPendingCall call = d->m_connectionProxy.Disconnect();

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [d](QDBusPendingCallWatcher *watcher) {
        QDBusPendingReply<void> reply = *watcher;
        watcher->deleteLater();
        if (reply.isError()) {
            qDebug() << "Unable to deactivate Connman VPN connection:" << d->m_path << ":" << reply.error().message();
        }
    });
}

void VpnConnection::update(const QVariantMap &updateProperties)
{
    Q_D(VpnConnection);

    QVariantMap properties(updateProperties);

    // If providerProperties have been modified, merge them with existing values
    bool changed = false;
    auto ppit = properties.find(QStringLiteral("providerProperties"));
    if (ppit != properties.end()) {
        QVariantMap existingProperties = providerProperties();

        QVariantMap updated = (*ppit).value<QVariantMap>();
        for (QVariantMap::const_iterator pit = updated.cbegin(), pend = updated.cend(); pit != pend; ++pit) {
            if (existingProperties.value(pit.key()) != pit.value()) {
                changed = true;
                existingProperties.insert(pit.key(), pit.value());
            }
        }

        *ppit = QVariant::fromValue(existingProperties);
        if (changed) {
            emit providerPropertiesChanged();
        }
    }

    ConnectionState oldState(d->m_state);
    QQueue<void(VpnConnection::*)()> emissions;

    d->checkChanged(properties, emissions, "name", &VpnConnection::nameChanged);
    d->checkChanged(properties, emissions, "host", &VpnConnection::hostChanged);
    d->checkChanged(properties, emissions, "domain", &VpnConnection::domainChanged);
    d->checkChanged(properties, emissions, "storeCredentials", &VpnConnection::storeCredentialsChanged);
    d->checkChanged(properties, emissions, "type", &VpnConnection::typeChanged);
    d->checkChanged(properties, emissions, "immutable", &VpnConnection::immutableChanged);
    d->checkChanged(properties, emissions, "index", &VpnConnection::indexChanged);
    d->checkChanged(properties, emissions, "ipv4", &VpnConnection::ipv4Changed);
    d->checkChanged(properties, emissions, "ipv6", &VpnConnection::ipv6Changed);
    d->checkChanged(properties, emissions, "nameservers", &VpnConnection::nameserversChanged);
    d->checkChanged(properties, emissions, "usreRoutes", &VpnConnection::userRoutesChanged);
    d->checkChanged(properties, emissions, "serverRoutes", &VpnConnection::serverRoutesChanged);

    d->updateVariable(properties, emissions, "autoConnect", &d->m_autoConnect, &VpnConnection::autoConnectChanged);
    d->updateVariable(properties, emissions, "state", &d->m_state, &VpnConnection::stateChanged);

    changed = (emissions.count() > 0);

    for (QVariantMap::const_iterator it = properties.constBegin(); it != properties.constEnd(); ++it) {
        QVariantMap::iterator existing = d->m_properties.find(it.key());
        if ((existing != d->m_properties.end()) && (it.value() != existing.value())) {
            changed = true;
        }
        d->m_properties.insert(it.key(), it.value());
    }

    for (auto emission : emissions) {
        emit (this->*emission)();
    }

    if (changed) {
        emit propertiesChanged();
    }

    if ((d->m_state == VpnConnection::Ready) != (oldState == VpnConnection::Ready)) {
        emit connectedChanged();
    }
}

int VpnConnection::connected() const
{
    Q_D(const VpnConnection);

    return d->m_state == VpnConnection::Ready;
}

QString VpnConnection::path() const
{
    Q_D(const VpnConnection);

    return d->m_path;
}

bool VpnConnection::autoConnect() const
{
    Q_D(const VpnConnection);

    return d->m_autoConnect;
}

void VpnConnection::setAutoConnect(bool autoConnect)
{
    Q_D(VpnConnection);

    if (d->m_autoConnect != autoConnect) {
        d->m_autoConnect = autoConnect;
        qDebug() << "VPN autoconnect changed:" << d->m_properties.value("name").toString() << autoConnect;
        d->m_serviceProxy.SetProperty(autoConnectKey, QDBusVariant(autoConnect));
        emit autoConnectChanged();
    }
}

VpnConnection::ConnectionState VpnConnection::state() const
{
    Q_D(const VpnConnection);

    return d->m_state;
}

QVariantMap VpnConnection::properties() const
{
    Q_D(const VpnConnection);

    return d->m_properties;
}

void VpnConnection::setProperties(const QVariantMap properties)
{
    Q_D(VpnConnection);

    if (d->m_properties != properties) {
        d->m_properties = properties;

        modifyConnection(properties);

        emit propertiesChanged();
    }
}

DEFAULT_PROPERTY_METHODS(QString &, QString, Name, name)
DEFAULT_PROPERTY_METHODS(QString &, QString, Host, host)
DEFAULT_PROPERTY_METHODS(QString &, QString, Domain, domain)
DEFAULT_PROPERTY_METHODS(bool, bool, StoreCredentials, storeCredentials)
DEFAULT_PROPERTY_METHODS(QString &, QString, Type, type)
DEFAULT_PROPERTY_METHODS(bool, bool, Immutable, immutable)
DEFAULT_PROPERTY_METHODS(int, int, Index, index)
DEFAULT_PROPERTY_METHODS(QVariantMap &, QVariantMap, Ipv4, ipv4)
DEFAULT_PROPERTY_METHODS(QVariantMap &, QVariantMap, Ipv6, ipv6)
DEFAULT_PROPERTY_METHODS(QStringList &, QStringList, Nameservers, nameservers)
DEFAULT_PROPERTY_METHODS(QVariantList &, QVariantList, UserRoutes, userRoutes)
DEFAULT_PROPERTY_METHODS(QVariantList &, QVariantList, ServerRoutes, serverRoutes)
DEFAULT_PROPERTY_METHODS(QVariantMap &, QVariantMap, ProviderProperties, providerProperties)

// ==========================================================================
// VpnConnectionPrivate methods
// ==========================================================================

inline void VpnConnectionPrivate::checkChanged(QVariantMap &properties, QQueue<void(VpnConnection::*)()> &emissions, const QString &name, void(VpnConnection::*changedSignal)())
{
    QVariantMap::const_iterator it = properties.constFind(name);
    if ((it != properties.constEnd()) && (m_properties.value(name) != it.value())) {
        m_properties.insert(name, it.value());
        properties.remove(name);
        emissions << changedSignal;
    }
}

template<typename T>
inline void VpnConnectionPrivate::updateVariable(QVariantMap &properties, QQueue<void(VpnConnection::*)()> &emissions, const QString &name, T *property, void(VpnConnection::*changedSignal)())
{
    QVariantMap::const_iterator it = properties.constFind(name);
    if ((it != properties.constEnd()) && (*property != qvariant_cast< T >(it.value()))) {
        *property = qvariant_cast< T >(it.value());
        m_properties.insert(name, it.value());
        properties.remove(name);
        emissions << changedSignal;
    }
}

void VpnConnectionPrivate::setProperty(const QString &key, const QVariant &value, void(VpnConnection::*changedSignal)())
{
    Q_Q(VpnConnection);

    if (m_properties.value(key) != value) {
        m_connectionProxy.SetProperty(key, QDBusVariant(value));
        m_properties.insert(key, value);
        emit q->propertiesChanged();
        emit (q->*changedSignal)();
    }
}

