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
#include "technologyservicemodel.h"

TechnologyServiceModel::TechnologyServiceModel(QObject *parent)
  : QAbstractListModel(parent),
    m_tech(nullptr),
    m_scanning(false),
    m_changesInhibited(false),
    m_uneffectedChanges(false),
    m_filter(AvailableServices)
{
    m_manager = NetworkManager::sharedInstance();

    connect(m_manager.data(), &NetworkManager::availabilityChanged,
            this, &TechnologyServiceModel::managerAvailabilityChanged);

    connect(m_manager.data(), &NetworkManager::technologiesChanged,
            this, &TechnologyServiceModel::updateTechnologies);

    connect(m_manager.data(), &NetworkManager::servicesChanged,
            this, &TechnologyServiceModel::updateServiceList);
}

TechnologyServiceModel::~TechnologyServiceModel()
{
}

QHash<int, QByteArray> TechnologyServiceModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ServiceRole] = "networkService";
    return roles;
}

QVariant TechnologyServiceModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case ServiceRole:
        return QVariant::fromValue(static_cast<QObject *>(m_services.value(index.row())));
    }

    return QVariant();
}

int TechnologyServiceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_services.count();
}

int TechnologyServiceModel::count() const
{
    return rowCount();
}

QString TechnologyServiceModel::name() const
{
    return m_techname;
}

bool TechnologyServiceModel::isAvailable() const
{
    return m_manager->isAvailable() && m_tech;
}

bool TechnologyServiceModel::isConnected() const
{
    if (m_tech) {
        return m_tech->connected();
    } else {
        return false;
    }
}

bool TechnologyServiceModel::isPowered() const
{
    if (m_tech) {
        return m_tech->powered();
    } else {
        return false;
    }
}

bool TechnologyServiceModel::isScanning() const
{
    return m_scanning;
}

bool TechnologyServiceModel::changesInhibited() const
{
    return m_changesInhibited;
}

TechnologyServiceModel::ServiceFilter TechnologyServiceModel::filter() const
{
    return m_filter;
}

void TechnologyServiceModel::setFilter(ServiceFilter filter)
{
    if (m_filter != filter) {
        m_filter = filter;
        updateServiceList();
        Q_EMIT filterChanged();
    }
}

void TechnologyServiceModel::setPowered(bool powered)
{
    if (m_tech) {
        m_tech->setPowered(powered);
    } else {
        qWarning() << "Can't set: technology is NULL";
    }
}

void TechnologyServiceModel::setName(const QString &name)
{
    if (m_techname == name || name.isEmpty()) {
        return;
    }
    m_techname = name;
    Q_EMIT nameChanged(m_techname);

    updateTechnologies();
}

void TechnologyServiceModel::setChangesInhibited(bool b)
{
    if (m_changesInhibited != b) {
        m_changesInhibited = b;

        if (m_changesInhibited) {
            // Since m_changesInhibited can also inhibit updates
            // about removed/deleted services, connect destroyed.
            for (const NetworkService *service : m_services) {
                connect(service, &QObject::destroyed,
                        this, &TechnologyServiceModel::networkServiceDestroyed);
            }
        } else {
            for (const NetworkService *service : m_services) {
                disconnect(service, &QObject::destroyed,
                           this, &TechnologyServiceModel::networkServiceDestroyed);
            }

        }

        Q_EMIT changesInhibitedChanged(m_changesInhibited);

        if (!m_changesInhibited && m_uneffectedChanges) {
            m_uneffectedChanges = false;
            updateServiceList();
        }
    }
}

void TechnologyServiceModel::requestScan()
{
    if (m_tech && !m_tech->tethering()) {
        m_tech->scan();
        m_scanning = true;
        Q_EMIT scanningChanged(m_scanning);
    }
}

void TechnologyServiceModel::updateTechnologies()
{
    bool wasAvailable = m_manager->isAvailable() && m_tech;

    doUpdateTechnologies();

    bool isAvailable = m_manager->isAvailable() && m_tech;

    if (wasAvailable != isAvailable)
        Q_EMIT availabilityChanged(isAvailable);
}

void TechnologyServiceModel::doUpdateTechnologies()
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

void TechnologyServiceModel::managerAvailabilityChanged(bool available)
{
    bool wasAvailable = !available && m_tech;

    doUpdateTechnologies();

    bool isAvailable = available && m_tech;
    if (wasAvailable != isAvailable)
        Q_EMIT availabilityChanged(isAvailable);
}

NetworkService *TechnologyServiceModel::get(int index) const
{
    if (index < 0 || index > m_services.count())
        return 0;
    return m_services.value(index);
}

int TechnologyServiceModel::indexOf(const QString &dbusObjectPath) const
{
    int idx(-1);

    for (const NetworkService *service : m_services) {
        idx++;
        if (service->path() == dbusObjectPath) return idx;
    }

    return -1;
}

void TechnologyServiceModel::updateServiceList()
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

void TechnologyServiceModel::changedPower(bool b)
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

void TechnologyServiceModel::changedConnected(bool b)
{
    NetworkTechnology *tech = qobject_cast<NetworkTechnology *>(sender());
    if (tech->type() == m_tech->type())
        Q_EMIT connectedChanged(b);
}

void TechnologyServiceModel::finishedScan()
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

void TechnologyServiceModel::networkServiceDestroyed(QObject *service)
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

TechnologyModel::TechnologyModel(QObject *parent)
    : TechnologyServiceModel(parent)
{
    qWarning() << "TechnologyModel is deprecated. Use TechnologyServiceModel";
}
