/*   -*- Mode: C++ -*-
 * meegotouchcp-connman - connectivity plugin for duicontrolpanel
 * Copyright © 2010, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
#include "networklist.h"
#include "debug.h"

NetworkListModel::NetworkListModel(QObject* parent)
    : QAbstractListModel(parent)
{
    m_networkManager = NetworkManagerFactory::createInstance();

    connect(m_networkManager,SIGNAL(networksAdded(int,int)),this,SLOT(networksAdded(int,int)));
    connect(m_networkManager,SIGNAL(networksMoved(int,int,int)),this,SLOT(networksMoved(int,int,int)));
    connect(m_networkManager,SIGNAL(networksRemoved(int,int)),this,SLOT(networksRemoved(int,int)));
    connect(m_networkManager,SIGNAL(networkChanged(int)),this,SLOT(networkChanged(int)));

    connect(m_networkManager,SIGNAL(technologiesChanged(QStringList,QStringList,QStringList)),
            this, SIGNAL(technologiesChanged(QStringList,QStringList,QStringList)));
    connect(m_networkManager,SIGNAL(availableTechnologiesChanged(QStringList)),
            this, SIGNAL(availableTechnologiesChanged(QStringList)));
    connect(m_networkManager,SIGNAL(enabledTechnologiesChanged(QStringList)),
            this,SIGNAL(enabledTechnologiesChanged(QStringList)));
    connect(m_networkManager,SIGNAL(connectedTechnologiesChanged(QStringList)),
            this,SIGNAL(connectedTechnologiesChanged(QStringList)));
    connect(m_networkManager,SIGNAL(offlineModeChanged(bool)),
            this,SIGNAL(offlineModeChanged(bool)));
    connect(m_networkManager,SIGNAL(defaultTechnologyChanged(QString)),
            this,SIGNAL(defaultTechnologyChanged(QString)));
    connect(m_networkManager,SIGNAL(stateChanged(QString)),
            this,SIGNAL(stateChanged(QString)));
    connect(m_networkManager,SIGNAL(countChanged(int)),this,SIGNAL(countChanged(int)));
    connect(m_networkManager,SIGNAL(defaultRouteChanged(NetworkItemModel*)),
            this,SIGNAL(defaultRouteChanged(NetworkItemModel*)));
    connect(m_networkManager,SIGNAL(connectedNetworkItemsChanged()),
            this,SIGNAL(connectedNetworkItemsChanged()));

    if(m_networkManager->networks().count());
    networksAdded(0, m_networkManager->networks().count());

    QHash<int, QByteArray> roles;

    QMetaObject properties = NetworkItemModel::staticMetaObject;
    int i=0;
    for(; i<properties.propertyCount();i++)
    {
        roles[i]=properties.property(i).name();
    }

    roles[++i] = "networkitemmodel";
    roles[++i] = "defaultRoute";

    setRoleNames(roles);
}

NetworkListModel::~NetworkListModel()
{
}

int NetworkListModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_networkManager->networks().size();
}

int NetworkListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

QVariant NetworkListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return QVariant();
}


QVariant NetworkListModel::data(const QModelIndex &index, int role) const
{
	//qDebug("NetworkListModel::data");


	QString roleName = roleNames()[role];
	QMetaObject object = NetworkItemModel::staticMetaObject;

	if(!(index.isValid() && index.row() < shadowList.size()))
		return QVariant();
	if(roleName == "networkitemmodel")
	{
		return QVariant::fromValue<QObject*>((QObject*)shadowList[index.row()]);
	}
	else if(roleName == "defaultRoute")
	{
		return m_networkManager->defaultRoute() == shadowList[index.row()];
	}

	for(int i=0; i<object.propertyCount(); i++)
	{
		if(object.property(i).name() == roleName)
		{
			return object.property(i).read(shadowList[index.row()]);
		}
	}

	return QVariant();
}

void NetworkListModel::setProperty(const int &index, QString property, const QVariant &value)
{
	m_networkManager->setProperty(index, property, value);
}

void NetworkListModel::enableTechnology(const QString &technology)
{
	m_networkManager->enableTechnology(technology);
}

void NetworkListModel::disableTechnology(const QString &technology)
{
   m_networkManager->disableTechnology(technology);
}

NetworkItemModel* NetworkListModel::service(QString name)
{
    return m_networkManager->service(name);
}

void NetworkListModel::connectService(const QString &name, const QString &security,
                                      const QString &passphrase)
{
   m_networkManager->connectService(name, security,passphrase);
}

void NetworkListModel::connectService(const QString &name)
{
    m_networkManager->connectService(name);
}

const QStringList NetworkListModel::availableTechnologies() const
{
    return m_networkManager->availableTechnologies();
}

const QStringList NetworkListModel::enabledTechnologies() const
{
    return m_networkManager->enabledTechnologies();
}

const QStringList NetworkListModel::connectedTechnologies() const
{
    return m_networkManager->connectedTechnologies();
}

void NetworkListModel::setDefaultRoute(NetworkItemModel *item)
{
    m_networkManager->setDefaultRoute(item);
}

void NetworkListModel::requestScan()
{
    m_networkManager->requestScan();
}

void NetworkListModel::setOfflineMode(const bool &offlineMode)
{
   m_networkManager->setOfflineMode(offlineMode);
}

bool NetworkListModel::offlineMode() const
{
    return m_networkManager->offlineMode();
}

QString NetworkListModel::defaultTechnology() const
{
    return m_networkManager->defaultTechnology();
}

QString NetworkListModel::state() const
{
    return m_networkManager->state();
}

void NetworkListModel::networksAdded(int from, int to)
{
    beginInsertRows(QModelIndex(), from, to);

    shadowList = m_networkManager->networks();

    endInsertRows();
}

void NetworkListModel::networksMoved(int from, int to, int to2)
{
    beginMoveRows(QModelIndex(), from, to, QModelIndex(), to2);
    shadowList = m_networkManager->networks();
    endMoveRows();
}

void NetworkListModel::networksRemoved(int from, int to)
{
    beginRemoveRows(QModelIndex(), from, to);
    shadowList = m_networkManager->networks();
    endRemoveRows();
}

void NetworkListModel::networkChanged(int index)
{
    dataChanged(createIndex(index,0),createIndex(index,0));
}
