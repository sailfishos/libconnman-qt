/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2017 Jolla Ltd.
 * Contact: Slava Monich <slava.monich@jolla.com>
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0. The full text of the Apache License
 * is at http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QDebug>
#include "technologymodel.h"

TechnologyModel::TechnologyModel(QAbstractListModel* parent)
  : QAbstractListModel(parent),
    m_manager(NULL),
    m_tech(NULL),
    m_scanning(false),
    m_changesInhibited(false),
    m_uneffectedChanges(false),
    m_filter(AvailableServices)
{
    m_manager = NetworkManagerFactory::createInstance();

    connect(m_manager, SIGNAL(availabilityChanged(bool)),
            this, SLOT(managerAvailabilityChanged(bool)));

    connect(m_manager,
            SIGNAL(technologiesChanged()),
            this,
            SLOT(updateTechnologies()));

    connect(m_manager,
            SIGNAL(servicesChanged()),
            this,
            SLOT(updateServiceList()));
}

TechnologyModel::~TechnologyModel()
{
}

QHash<int, QByteArray> TechnologyModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ServiceRole] = "networkService";
    return roles;
}

QVariant TechnologyModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case ServiceRole:
        return QVariant::fromValue(static_cast<QObject *>(m_services.value(index.row())));
    }

    return QVariant();
}

int TechnologyModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_services.count();
}

int TechnologyModel::count() const
{
    return rowCount();
}

QString TechnologyModel::name() const
{
    return m_techname;
}

bool TechnologyModel::isAvailable() const
{
    return m_manager->isAvailable() && m_tech;
}

bool TechnologyModel::isConnected() const
{
    if (m_tech) {
        return m_tech->connected();
    } else {
        qWarning() << "Can't get: technology is NULL";
        return false;
    }
}

bool TechnologyModel::isPowered() const
{
    if (m_tech) {
        return m_tech->powered();
    } else {
        qWarning() << "Can't get: technology is NULL";
        return false;
    }
}

bool TechnologyModel::isScanning() const
{
    return m_scanning;
}

bool TechnologyModel::changesInhibited() const
{
    return m_changesInhibited;
}

TechnologyModel::ServiceFilter TechnologyModel::filter() const
{
    return m_filter;
}

void TechnologyModel::setFilter(ServiceFilter filter)
{
    if (m_filter != filter) {
        m_filter = filter;
        updateServiceList();
        Q_EMIT filterChanged();
    }
}

void TechnologyModel::setPowered(bool powered)
{
    if (m_tech) {
        m_tech->setPowered(powered);
    } else {
        qWarning() << "Can't set: technology is NULL";
    }
}

void TechnologyModel::setName(const QString &name)
{
    if (m_techname == name || name.isEmpty()) {
        return;
    }
    m_techname = name;
    Q_EMIT nameChanged(m_techname);

    updateTechnologies();
}

void TechnologyModel::setChangesInhibited(bool b)
{
    if (m_changesInhibited != b) {
        m_changesInhibited = b;

        if (m_changesInhibited) {
            // Since m_changesInhibited can also inhibit updates
            // about removed/deleted services, connect destroyed.
            for (const NetworkService *service : m_services) {
                connect(service, &QObject::destroyed,
                        this, &TechnologyModel::networkServiceDestroyed);
            }
        } else {
            for (const NetworkService *service : m_services) {
                disconnect(service, &QObject::destroyed,
                           this, &TechnologyModel::networkServiceDestroyed);
            }

        }

        Q_EMIT changesInhibitedChanged(m_changesInhibited);

        if (!m_changesInhibited && m_uneffectedChanges) {
            m_uneffectedChanges = false;
            updateServiceList();
        }
    }
}

void TechnologyModel::requestScan()
{
    if (m_tech && !m_tech->tethering()) {
        m_tech->scan();
        m_scanning = true;
        Q_EMIT scanningChanged(m_scanning);
    }
}

void TechnologyModel::updateTechnologies()
{
    bool wasAvailable = m_manager->isAvailable() && m_tech;

    doUpdateTechnologies();

    bool isAvailable = m_manager->isAvailable() && m_tech;

    if (wasAvailable != isAvailable)
        Q_EMIT availabilityChanged(isAvailable);
}

void TechnologyModel::doUpdateTechnologies()
{
    NetworkTechnology *newTech = m_manager->getTechnology(m_techname);
    if (m_tech == newTech)
        return;

    bool oldPowered = false;
    bool oldConnected = false;

    if (m_tech) {
        oldPowered = m_tech->powered();
        oldConnected = m_tech->connected();

        disconnect(m_tech, SIGNAL(poweredChanged(bool)), this, SLOT(changedPower(bool)));
        disconnect(m_tech, SIGNAL(connectedChanged(bool)), this, SLOT(changedConnected(bool)));
        disconnect(m_tech, SIGNAL(scanFinished()), this, SLOT(finishedScan()));
    }

    if (m_scanning) {
        m_scanning = false;
        Q_EMIT scanningChanged(m_scanning);
    }

    m_tech = newTech;

    if (m_tech) {
        connect(m_tech, SIGNAL(poweredChanged(bool)), this, SLOT(changedPower(bool)));
        connect(m_tech, SIGNAL(connectedChanged(bool)), this, SLOT(changedConnected(bool)));
        connect(m_tech, SIGNAL(scanFinished()), this, SLOT(finishedScan()));

        bool b = m_tech->powered();
        if (b != oldPowered)
            Q_EMIT poweredChanged(b);
        b = m_tech->connected();
        if (b != oldConnected)
            Q_EMIT connectedChanged(b);
    } else {
        if (oldPowered)
            Q_EMIT poweredChanged(false);
        if (oldConnected)
            Q_EMIT connectedChanged(false);
    }

    Q_EMIT technologiesChanged();

    updateServiceList();
}

void TechnologyModel::managerAvailabilityChanged(bool available)
{
    bool wasAvailable = !available && m_tech;

    doUpdateTechnologies();

    bool isAvailable = available && m_tech;
    if (wasAvailable != isAvailable)
        Q_EMIT availabilityChanged(isAvailable);
}

NetworkService *TechnologyModel::get(int index) const
{
    if (index < 0 || index > m_services.count())
        return 0;
    return m_services.value(index);
}

int TechnologyModel::indexOf(const QString &dbusObjectPath) const
{
    int idx(-1);

    for (const NetworkService *service : m_services) {
        idx++;
        if (service->path() == dbusObjectPath) return idx;
    }

    return -1;
}

void TechnologyModel::updateServiceList()
{
    if (m_changesInhibited) {
        m_uneffectedChanges = true;
        return;
    }

    if (m_techname.isEmpty())
        return;

    const int num_old = m_services.count();

    const QVector<NetworkService *> new_services =
        (m_filter == SavedServices) ? m_manager->getSavedServices(m_techname)
                                    : (m_filter == AvailableServices)
                                      ? m_manager->getAvailableServices(m_techname)
                                      : m_manager->getServices(m_techname);

    const int num_new = new_services.count();

    for (int i = 0; i < num_new; i++) {
        int j = m_services.indexOf(new_services.value(i));
        if (j == -1) {
            // wifi service not found -> remove from list
            beginInsertRows(QModelIndex(), i, i);
            m_services.insert(i, new_services.value(i));
            endInsertRows();
        } else if (i != j) {
            // wifi service changed its position -> move it
            NetworkService* service = m_services.value(j);
            beginMoveRows(QModelIndex(), j, j, QModelIndex(), i);
            m_services.remove(j);
            m_services.insert(i, service);
            endMoveRows();
        }
    }
    // After loop:
    // m_services contains [new_services, old_services \ new_services]

    int num_union = m_services.count();
    if (num_union > num_new) {
        beginRemoveRows(QModelIndex(), num_new, num_union - 1);
        m_services.remove(num_new, num_union - num_new);
        endRemoveRows();
    }

    if (num_new != num_old)
        Q_EMIT countChanged();
}

void TechnologyModel::changedPower(bool b)
{
    NetworkTechnology *tech = qobject_cast<NetworkTechnology *>(sender());
    if (tech->type() != m_tech->type())
        return;

    Q_EMIT poweredChanged(b);

    if (!b && m_scanning) {
        m_scanning = false;
        Q_EMIT scanningChanged(m_scanning);
    }
}

void TechnologyModel::changedConnected(bool b)
{
    NetworkTechnology *tech = qobject_cast<NetworkTechnology *>(sender());
    if (tech->type() == m_tech->type())
        Q_EMIT connectedChanged(b);
}

void TechnologyModel::finishedScan()
{
    NetworkTechnology *tech = qobject_cast<NetworkTechnology *>(sender());
    if (tech->type() != m_tech->type())
        return;

    Q_EMIT scanRequestFinished();

    if (m_scanning) {
        m_scanning = false;
        Q_EMIT scanningChanged(m_scanning);
    }
}

void TechnologyModel::networkServiceDestroyed(QObject *service)
{
    int ind = m_services.indexOf(static_cast<NetworkService*>(service));
    if (ind >= 0) {
        qWarning() << "out-of-band removal of network service" << service;
        beginRemoveRows(QModelIndex(), ind, ind);
        m_services.remove(ind);
        endRemoveRows();
        Q_EMIT countChanged();
    }
}
