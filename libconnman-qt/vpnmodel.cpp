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

#include <QtDBus>
#include <vpnmanager.h>
#include <vpnconnection.h>

#include "vpnmodel.h"
#include "vpnmodel_p.h"

#include "vpnmanager_p.h"

const QHash<int, QByteArray> VpnModelPrivate::m_roles({{VpnModel::VpnRole, "vpnService"}});

// ==========================================================================
// VpnModel
// ==========================================================================

VpnModelPrivate::VpnModelPrivate(VpnModel &qq)
    : m_manager(nullptr)
    , q_ptr(&qq)
{
}

void VpnModelPrivate::init()
{
    Q_Q(VpnModel);

    m_manager = VpnManagerFactory::createInstance();

    emit q->vpnManagerChanged();

    VpnModel::connect(m_manager, &VpnManager::connectionsChanged, q, &VpnModel::connectionsChanged);
    VpnModel::connect(m_manager, &VpnManager::populatedChanged, q, &VpnModel::populatedChanged);

    VpnModel::connect(VpnManagerPrivate::get(m_manager), &VpnManagerPrivate::beginConnectionsReset, q, [q, this]() {
        q->beginResetModel();
        m_connections.clear();
    });

    VpnModel::connect(VpnManagerPrivate::get(m_manager), &VpnManagerPrivate::endConnectionsReset, q, [q]() {
        q->endResetModel();
    });

    q->connectionsChanged();
}

VpnModel::VpnModel(QObject* parent)
    : QAbstractListModel(parent)
    , d_ptr(new VpnModelPrivate(*this))
{
    Q_D(VpnModel);
    d->init();
}

VpnModel::VpnModel(VpnModelPrivate &dd, QObject *parent)
    : QAbstractListModel(parent)
    , d_ptr(&dd)
{
    Q_D(VpnModel);
    d->init();
}

VpnModel::~VpnModel()
{
    Q_D(VpnModel);

    disconnect(d->m_manager, &VpnManager::connectionsChanged, this, &VpnModel::connectionsChanged);
}

QHash<int, QByteArray> VpnModel::roleNames() const
{
    return VpnModelPrivate::m_roles;
}

QVariant VpnModel::data(const QModelIndex &index, int role) const
{
    Q_D(const VpnModel);

    if (index.isValid() && index.row() >= 0 && index.row() < d->m_connections.count()) {
        switch (role) {
        case VpnRole:
            return QVariant::fromValue(static_cast<QObject *>(d->m_connections.at(index.row())));
        }
    }

    return QVariant();
}

int VpnModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    Q_D(const VpnModel);

    return d->m_connections.size();
}

QModelIndex VpnModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const VpnModel);

    return !parent.isValid() && column == 0 && row >= 0 && row < d->m_connections.count()
            ? createIndex(row, column)
            : QModelIndex();
}

int VpnModel::count() const
{
    Q_D(const VpnModel);

    return d->m_connections.size();
}

bool VpnModel::isConnected() const
{
    return false;
}

void VpnModel::connectionsChanged()
{
    Q_D(VpnModel);

    // Update the connections list
    const int num_old = d->m_connections.size();

    for (const VpnConnection *connection: d->m_connections) {
        disconnect(connection, &VpnConnection::destroyed,
                   this, &VpnModel::connectionDestroyed);
    }

    QVector<VpnConnection *> new_connections(d->m_manager->connections());

    // Allow connection ordering to be overridden
    orderConnections(new_connections);

    const int num_new = new_connections.count();

    // Since m_changesInhibited can also inhibit updates
    // about removed/deleted services, connect destroyed.
    for (const VpnConnection *connection: new_connections) {
        connect(connection, &VpnConnection::destroyed,
                this, &VpnModel::connectionDestroyed);
    }

    for (int i = 0; i < num_new; i++) {
        int j = d->m_connections.indexOf(new_connections.value(i));
        if (j == -1) {
            // Vpn connection not found -> add to list
            beginInsertRows(QModelIndex(), i, i);
            d->m_connections.insert(i, new_connections.value(i));
            endInsertRows();
        } else if (i != j) {
            // Vpn connection changed its position -> move it
            VpnConnection* connection = d->m_connections.value(j);
            beginMoveRows(QModelIndex(), j, j, QModelIndex(), i);
            d->m_connections.remove(j);
            d->m_connections.insert(i, connection);
            endMoveRows();
        }
    }
    // After loop:
    // connections contains [new_connections, old_connections \ new_connections]

    int num_union = d->m_connections.count();
    if (num_union > num_new) {
        beginRemoveRows(QModelIndex(), num_new, num_union - 1);
        d->m_connections.remove(num_new, num_union - num_new);
        endRemoveRows();
    }

    if (num_new != num_old)
        Q_EMIT countChanged();
}

void VpnModel::connectionDestroyed(QObject *connection)
{
    Q_D(VpnModel);

    int ind = d->m_connections.indexOf(dynamic_cast<VpnConnection*>(connection));
    if (ind >= 0) {
        qWarning() << "out-of-band removal of vpn connection" << connection;
        beginRemoveRows(QModelIndex(), ind, ind);
        d->m_connections.remove(ind);
        endRemoveRows();
        Q_EMIT countChanged();
    }
}

bool VpnModel::populated() const
{
    Q_D(const VpnModel);

    return d->m_manager->populated();
}

VpnManager * VpnModel::vpnManager() const
{
    Q_D(const VpnModel);

    return d->m_manager;
}

QVariantMap VpnModel::connectionSettings(const QString &path) const
{
    Q_D(const VpnModel);

    QVariantMap properties;
    if (VpnConnection *conn = d->m_manager->connection(path)) {
        properties = conn->properties();
    }
    return properties;
}

void VpnModel::orderConnections(QVector<VpnConnection*> &connections)
{
    Q_UNUSED(connections)
    // Do nothing - no ordering
}

void VpnModel::moveItem(int oldIndex, int newIndex)
{
    Q_D(VpnModel);

    if (oldIndex >= 0 && oldIndex < d->m_connections.size() && newIndex >= 0 && newIndex < d->m_connections.size()) {
        beginMoveRows(QModelIndex(), oldIndex, oldIndex, QModelIndex(), (newIndex > oldIndex) ? (newIndex + 1) : newIndex);
        d->m_connections.move(oldIndex, newIndex);
        endMoveRows();
    }
}

QVector<VpnConnection*> VpnModel::connections() const
{
    Q_D(const VpnModel);

    return d->m_connections;
}
