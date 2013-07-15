/*
 * Copyright © 2010, Intel Corporation.
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include <QDebug>
#include "technologymodel.h"

#define CONNECT_TECHNOLOGY_SIGNALS(tech) \
    connect(tech, \
        SIGNAL(poweredChanged(bool)), \
        this, \
        SLOT(changedPower(bool))); \
    connect(tech, \
        SIGNAL(connectedChanged(bool)), \
        this, \
        SLOT(changedConnected(bool))); \
    connect(tech, \
            SIGNAL(scanFinished()), \
            this, \
            SLOT(finishedScan()));


#define DISCONNECT_TECHNOLOGY_SIGNALS(tech) \
    disconnect(tech, SIGNAL(poweredChanged(bool)), \
               this, SLOT(changedPower(bool))); \
    disconnect(tech, SIGNAL(scanFinished()), \
               this, SLOT(finishedScan()))


TechnologyModel::TechnologyModel(QAbstractListModel* parent)
  : QAbstractListModel(parent),
    m_manager(NULL),
    m_tech(NULL)
{
    m_manager = NetworkManagerFactory::createInstance();

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    setRoleNames(roleNames());
#endif
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

const QString TechnologyModel::name() const
{
    return m_techname;
}

bool TechnologyModel::isAvailable() const
{
    return m_manager->isAvailable();
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

void TechnologyModel::setPowered(const bool &powered)
{
    if (m_tech) {
        m_tech->setPowered(powered);
    } else {
        qWarning() << "Can't set: technology is NULL";
    }
}

void TechnologyModel::setName(const QString &name)
{
    if (m_techname == name) {
        return;
    }
    QStringList netTypes = m_manager->technologiesList();

    bool oldPowered(false);
    bool oldConnected(false);

    if (!netTypes.contains(name)) {
        qDebug() << name <<  "is not a known technology name:" << netTypes;
        return;
    }

    if (m_tech) {
        oldPowered = m_tech->powered();
        oldConnected = m_tech->connected();
        DISCONNECT_TECHNOLOGY_SIGNALS(m_tech);
    }

    m_tech = m_manager->getTechnology(name);

    if (!m_tech) {
        return;
    } else {
        m_techname = name;
        emit nameChanged(m_techname);
        if (oldPowered != m_tech->powered()) {
            emit poweredChanged(!oldPowered);
        }
        if (oldConnected != m_tech->connected()) {
            emit connectedChanged(!oldConnected);
        }
        CONNECT_TECHNOLOGY_SIGNALS(m_tech);
        updateServiceList();
    }
}

void TechnologyModel::requestScan() const
{
    if (m_tech) {
        m_tech->scan();
    }
}

void TechnologyModel::updateTechnologies()
{
    NetworkTechnology *test = NULL;
    if (m_tech) {
        if ((test = m_manager->getTechnology(m_techname)) == NULL) {
            // if wifi is set and manager doesn't return a wifi, it means
            // that wifi was removed
            DISCONNECT_TECHNOLOGY_SIGNALS(m_tech);
            m_tech = NULL;
            Q_EMIT technologiesChanged();
        }
    } else {
        if ((test = m_manager->getTechnology(m_techname)) != NULL) {
            // if wifi is not set and manager returns a wifi, it means
            // that wifi was added
            m_tech = test;
            CONNECT_TECHNOLOGY_SIGNALS(m_tech);
            emit technologiesChanged();
        }
    }
}

void TechnologyModel::managerAvailabilityChanged(bool available)
{
    emit availabilityChanged(available);
}

int TechnologyModel::indexOf(const QString &dbusObjectPath) const
{
    int idx(-1);

    foreach (NetworkService *service, m_services) {
        idx++;
        if (service->path() == dbusObjectPath) return idx;
    }

    return -1;
}

void TechnologyModel::updateServiceList()
{
    const QVector<NetworkService *> new_services = m_manager->getServices(m_techname);
    int num_new = new_services.count();

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

    int num_old = m_services.count();
    if (num_old > num_new) {
        beginRemoveRows(QModelIndex(), num_new, num_old - 1);
        m_services.remove(num_new, num_old - num_new);
        endRemoveRows();
    }
}

void TechnologyModel::changedPower(bool b)
{
    NetworkTechnology *tech = qobject_cast<NetworkTechnology *>(sender());
    if (tech->type() == m_tech->type())
        Q_EMIT poweredChanged(b);
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
    if (tech->type() == m_tech->type())
        Q_EMIT scanRequestFinished();
}

