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

#include "vpnmanager.h"
#include "vpnmanager_p.h"

// ==========================================================================
// VpnManagerFactory
// ==========================================================================

static VpnManager* staticInstance = nullptr;

VpnManager* VpnManagerFactory::createInstance()
{
    if (!staticInstance)
        staticInstance = new VpnManager;

    return staticInstance;
}

VpnManager* VpnManagerFactory::instance()
{
    return createInstance();
}

// ==========================================================================
// VpnManager
// ==========================================================================

namespace  {

const QString connmanVpnService = QStringLiteral("net.connman.vpn");

} // Empty namespace

VpnManagerPrivate::VpnManagerPrivate(VpnManager &qq)
    : m_connmanVpn(connmanVpnService, "/", QDBusConnection::systemBus(), nullptr)
    , m_populated(false)
    , q_ptr(&qq)
{
}

void VpnManagerPrivate::init()
{
    Q_Q(VpnManager);

    qDBusRegisterMetaType<PathProperties>();
    qDBusRegisterMetaType<PathPropertiesArray>();

    VpnManager::connect(&m_connmanVpn, &NetConnmanVpnManagerInterface::ConnectionAdded, q, [this](const QDBusObjectPath &objectPath, const QVariantMap &properties) {
        Q_Q(VpnManager);

        const QString path(objectPath.path());
        VpnConnection *conn = q->connection(path);
        if (!conn) {
            qDebug() << "Adding connection:" << path;
            conn = new VpnConnection(path);
            m_items.append(conn);
        }

        QVariantMap qmlProperties(MarshalUtils::propertiesToQml(properties));
        conn->update(qmlProperties);
        emit q->connectionAdded(path);
        emit q->connectionsChanged();
    });

    VpnManager::connect(&m_connmanVpn, &NetConnmanVpnManagerInterface::ConnectionRemoved, q, [this](const QDBusObjectPath &objectPath) {
        Q_Q(VpnManager);

        const QString path(objectPath.path());
        if (VpnConnection *conn = q->connection(path)) {
            qDebug() << "Removing obsolete connection:" << path;
            m_items.removeOne(conn);
            conn->deleteLater();
        } else {
            qDebug() << "Unable to remove unknown connection:" << path;
        }

        emit q->connectionRemoved(path);
        emit q->connectionsChanged();

        if (m_items.isEmpty()) {
            emit q->connectionsCleared();
        }
    });

    // If connman-vpn restarts, we need to discard and re-read the state
    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(connmanVpnService, QDBusConnection::systemBus(), QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration, q);
    VpnManager::connect(watcher, &QDBusServiceWatcher::serviceUnregistered, q, [this](const QString &) {
        Q_Q(VpnManager);

        emit beginConnectionsReset();
        qDeleteAll(m_items);
        m_items.clear();
        emit endConnectionsReset();
        setPopulated(false);

        emit q->connectionsCleared();
    });
    VpnManager::connect(watcher, &QDBusServiceWatcher::serviceRegistered, q, [this](const QString &) {
        fetchVpnList();
    });

    fetchVpnList();
}

VpnManager::VpnManager(QObject *parent)
    : QObject(parent)
    , d_ptr(new VpnManagerPrivate(*this))
{
    Q_D(VpnManager);
    d->init();
}

VpnManager::VpnManager(VpnManagerPrivate &dd, QObject *parent)
    : QObject(parent)
    , d_ptr(&dd)
{
    Q_D(VpnManager);
    d->init();
}

VpnManager::~VpnManager()
{
    // Destructor needed to handle the QScopedPointer<VpnManagerPrivate>
    // Do nothing
}

void VpnManager::createConnection(const QVariantMap &createProperties)
{
    Q_D(VpnManager);

    const QString path(createProperties.value(QString("path")).toString());
    if (path.isEmpty()) {
        const QString host(createProperties.value(QString("host")).toString());
        const QString name(createProperties.value(QString("name")).toString());
        const QString domain(createProperties.value(QString("domain")).toString());

        if (!host.isEmpty() && !name.isEmpty() && !domain.isEmpty()) {
            // Connman requires a domain value, but doesn't seem to use it...
            QDBusPendingCall call = d->m_connmanVpn.Create(MarshalUtils::propertiesToDBus(createProperties));

            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
            connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
                QDBusPendingReply<QDBusObjectPath> reply = *watcher;
                watcher->deleteLater();

                if (reply.isError()) {
                    qDebug() << "Unable to create Connman VPN connection:" << reply.error().message();
                } else {
                    const QDBusObjectPath &objectPath(reply.value());
                    qDebug() << "Created VPN connection:" << objectPath.path();
                }
            });
        } else {
            qDebug() << "Unable to create VPN connection without domain, host and name properties";
        }
    } else {
        qDebug() << "Unable to create VPN connection with pre-existing path:" << path;
    }
}

void VpnManager::modifyConnection(const QString &path, const QVariantMap &properties)
{
    VpnConnection *connection = this->connection(path);
    if (connection) {
        connection->modifyConnection(properties);
    } else {
        qDebug() << "Unable to update unknown VPN connection:" << path;
        qDebug() << "Connection count:" << this->count();
    }
}

void VpnManager::deleteConnection(const QString &path)
{
    Q_D(VpnManager);

    if (VpnConnection *conn = connection(path)) {
        if (conn->state() == VpnConnection::Ready ||
                                conn->state() == VpnConnection::Configuration ||
                                conn->state() == VpnConnection::Association) {
            conn->setAutoConnect(false);

            connect(conn, &VpnConnection::stateChanged, this, [this, path, conn](){
                qDebug() << "Reattempting connection deletion";
                // Ensure this only gets triggered once
                disconnect(conn, &VpnConnection::stateChanged, this, nullptr);
                VpnManager::deleteConnection(path);
            });
            conn->deactivate();
        } else {
            QDBusPendingCall call = d->m_connmanVpn.Remove(QDBusObjectPath(path));
            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
            connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, path](QDBusPendingCallWatcher *watcher) {
                QDBusPendingReply<void> reply = *watcher;
                watcher->deleteLater();
                if (reply.isError()) {
                    qDebug() << "Unable to delete Connman VPN connection:" << path << ":" << reply.error().message();
                } else {
                    qDebug() << "Deleted connection:" << path;
                }
            });
        }
    } else {
        qDebug() << "Unable to delete unknown connection:" << path;
    }
}

void VpnManager::activateConnection(const QString &path)
{
    Q_D(VpnManager);

    qDebug() << "Connect" << path;
    for (VpnConnection *conn : d->m_items) {
        QString otherPath = conn->path();
        if (otherPath != path && (conn->state() == VpnConnection::Ready ||
                                conn->state() == VpnConnection::Configuration ||
                                conn->state() == VpnConnection::Association)) {
            deactivateConnection(otherPath);
            qDebug() << "Adding pending vpn disconnect" << otherPath << conn->state() << "when connecting to vpn";
        }
    }

    qDebug() << "About to connect path:" << path;
    VpnConnection *conn = connection(path);
    if (conn) {
        conn->activate();
    } else {
        qDebug() << "Can't find VPN connection to activate it:" << path;
    }
}

void VpnManager::deactivateConnection(const QString &path)
{
    qDebug() << "Disconnect" << path;
    VpnConnection *conn = connection(path);
    if (conn) {
        conn->deactivate();
    } else {
        qDebug() << "Can't find VPN connection to deactivate it:" << path;
    }
}

VpnConnection *VpnManager::get(int index) const
{
    Q_D(const VpnManager);

    if ((index >= 0) && (index < d->m_items.size()))
    {
        return d->m_items.at(index);
    }
    return nullptr;
}

int VpnManager::count() const
{
    Q_D(const VpnManager);

    return d->m_items.size();
}

VpnConnection *VpnManager::connection(const QString &path) const
{
    Q_D(const VpnManager);

    for (VpnConnection *connection : d->m_items) {
        if (connection->path() == path) {
            return connection;
        }
    }

    return nullptr;
}

int VpnManager::indexOf(const QString &path) const
{
    Q_D(const VpnManager);

    int i = 0;
    while (i < d->m_items.size()) {
        if (d->m_items.at(i)->path() == path) {
            return i;
        }
    }

    return -1;
}

QVector<VpnConnection*> VpnManager::connections() const
{
    Q_D(const VpnManager);

    return d->m_items;
}

bool VpnManager::populated() const
{
    Q_D(const VpnManager);

    return d->m_populated;
}

// ==========================================================================
// VpnManagerPrivate methods
// ==========================================================================

void VpnManagerPrivate::fetchVpnList()
{
    Q_Q(VpnManager);

    QDBusPendingCall call = m_connmanVpn.GetConnections();

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, q);
    q->connect(watcher, &QDBusPendingCallWatcher::finished, q, [this](QDBusPendingCallWatcher *watcher) {
        Q_Q(VpnManager);

        QDBusPendingReply<PathPropertiesArray> reply = *watcher;
        watcher->deleteLater();

        if (reply.isError()) {
            qDebug() << "Unable to fetch Connman VPN connections:" << reply.error().message();
        } else {
            const PathPropertiesArray &connections(reply.value());

            for (const PathProperties &connection : connections) {
                const QString &path(connection.first.path());
                const QVariantMap &properties(connection.second);

                QVariantMap qmlProperties(MarshalUtils::propertiesToQml(properties));

                VpnConnection *conn = new VpnConnection(path);
                m_items.append(conn);
                conn->update(qmlProperties);
            }
            emit q->connectionsChanged();
            emit q->connectionsRefreshed();
        }

        setPopulated(true);
    });
}

void VpnManagerPrivate::setPopulated(bool populated)
{
    Q_Q(VpnManager);

    if (m_populated != populated) {
        m_populated = populated;
        emit q->populatedChanged();
    }
}
