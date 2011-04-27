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

const QString NetworkListModel::availTechs("AvailableTechnologies");
const QString NetworkListModel::enablTechs("EnabledTechnologies");
const QString NetworkListModel::connTechs("ConnectedTechnologies");
const QString NetworkListModel::OfflineMode("OfflineMode");
const QString NetworkListModel::DefaultTechnology("DefaultTechnology");
const QString NetworkListModel::State("State");

NetworkListModel::NetworkListModel(QObject* parent)
  : QAbstractListModel(parent),
    m_manager(NULL),
    m_getPropertiesWatcher(NULL),
    m_connectServiceWatcher(NULL),
    watcher(NULL),
    m_defaultRoute(NULL)
{
  m_headerData.append("NetworkItemModel");
  m_headerData.append("Type");
  connectToConnman();
  startTimer(60000);

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

  connect(this,SIGNAL(countChanged(int)),this,SLOT(countChangedSlot(int)));
}

NetworkListModel::~NetworkListModel()
{
}

int NetworkListModel::rowCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return m_networks.size();
}

int NetworkListModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return m_headerData.size();
}

QVariant NetworkListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  Q_UNUSED(orientation);
  if (role == Qt::DisplayRole) {
    if (section < m_headerData.size()) {
      return QVariant(m_headerData.at(section));
    }
  }
  return QVariant();
}


QVariant NetworkListModel::data(const QModelIndex &index, int role) const
{
  //qDebug("NetworkListModel::data");
	if(!(index.isValid() && index.row() < m_networks.size()))
		return QVariant();

	QString roleName = roleNames()[role];
	QMetaObject object = NetworkItemModel::staticMetaObject;

	if(roleName == "networkitemmodel")
	{
		return QVariant::fromValue<QObject*>((QObject*)m_networks[index.row()]);
	}
	else if(roleName == "defaultRoute")
	{
		return defaultRoute() == m_networks[index.row()];
	}

	for(int i=0; i<object.propertyCount(); i++)
	{
		if(object.property(i).name() == roleName)
		{
			return object.property(i).read(m_networks[index.row()]);
		}
	}

  return QVariant();
}

void NetworkListModel::setProperty(const int &index, QString property, const QVariant &value)
{
	QString roleName = property;
	QMetaObject object = NetworkItemModel::staticMetaObject;

	for(int i=0; i<object.propertyCount(); i++)
	{
		if(object.property(i).name() == roleName)
		{
			//emit dataChanged(index.row(),index.row());
			qDebug()<<"changing value of: "<<roleName<< " to "<<value;
			object.property(i).write(m_networks[index],value);
			break;
		}
	}

}

void NetworkListModel::enableTechnology(const QString &technology)
{
  qDebug("enabling technology \"%s\"", STR(technology));
  QDBusReply<void> reply = m_manager->EnableTechnology(technology);
  if(reply.error().isValid())
  {
	  qDebug()<<reply.error().message();
	  enabledTechnologiesChanged(enabledTechnologies());
  }
}

void NetworkListModel::disableTechnology(const QString &technology)
{
  qDebug("disenabling technology \"%s\"", STR(technology));
  m_manager->DisableTechnology(technology);
}

NetworkItemModel* NetworkListModel::service(QString name)
{
	foreach(NetworkItemModel* item, m_networks)
	{
		if(item->name() == name)
		{
			return item;
		}
	}

	return NULL;
}

void NetworkListModel::connectService(const QString &name, const QString &security,
				      const QString &passphrase)
{
  if(!m_manager)
  {
	connectToConnman();
	return;
  }

  qDebug("name: %s", STR(name));
  qDebug("security: %s", STR(security));
  qDebug("passphrase: %s", STR(passphrase));

  QVariantMap dict;
  dict.insert(QString("Type"), QVariant(QString("wifi")));
  dict.insert(QString("SSID"), QVariant(name));
  dict.insert(QString("Mode"), QVariant(QString("managed")));
  dict.insert(QString("Security"), QVariant(security));
  if (security != QString("none")) {
    dict.insert(QString("Passphrase"), QVariant(passphrase));
  }

  QDBusPendingReply<QDBusObjectPath> reply = m_manager->ConnectService(dict);
  m_connectServiceWatcher = new QDBusPendingCallWatcher(reply, m_manager);
  connect(m_connectServiceWatcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
	  this,
	  SLOT(connectServiceReply(QDBusPendingCallWatcher*)));
}

void NetworkListModel::connectService(const QString &name)
{
	foreach(NetworkItemModel* item, m_networks)
	{
		if(item->name() == name)
		{
			item->connectService();
			return;
		}
	}
	qDebug()<<"NetworkListModel::connectService(): service with name: "<<name<<" not found";
}

const QStringList NetworkListModel::availableTechnologies() const
{
  return qdbus_cast<QStringList>
    (m_propertiesCache[NetworkListModel::availTechs]);
}

const QStringList NetworkListModel::enabledTechnologies() const
{
  return qdbus_cast<QStringList>
      (m_propertiesCache[NetworkListModel::enablTechs]);
}

const QStringList NetworkListModel::connectedTechnologies() const
{
  return qdbus_cast<QStringList>
      (m_propertiesCache[NetworkListModel::connTechs]);
}

void NetworkListModel::setDefaultRoute(NetworkItemModel *item)
{
    if(m_networks.count() > 0 && (item->state() == NetworkItemModel::StateReady || item->state() == NetworkItemModel::StateOnline))
    {
        NetworkItemModel * topservice = m_networks.at(0);

        item->moveBefore(topservice);
    }
}

void NetworkListModel::connectToConnman(QString)
{
  if(!watcher) {
    watcher = new QDBusServiceWatcher("net.connman",QDBusConnection::systemBus(),
				      QDBusServiceWatcher::WatchForRegistration |
				      QDBusServiceWatcher::WatchForUnregistration,this);

    connect(watcher,SIGNAL(serviceRegistered(QString)),this,SLOT(connectToConnman(QString)));
    connect(watcher,SIGNAL(serviceUnregistered(QString)),this,SLOT(disconnectFromConnman(QString)));
  }

  disconnectFromConnman();
  m_manager = new Manager("net.connman", "/",
			  QDBusConnection::systemBus(),
			  this);
  if (!m_manager->isValid()) {
    //This shouldn't happen
    qDebug("manager is invalid. connman may not be running or is invalid");
    //QTimer::singleShot(10000,this,SLOT(connectToConnman()));
    delete m_manager;
    m_manager = NULL;
  } else {
    QDBusPendingReply<QVariantMap> reply = m_manager->GetProperties();
    m_getPropertiesWatcher = new QDBusPendingCallWatcher(reply, m_manager);
    connect(m_getPropertiesWatcher,
	    SIGNAL(finished(QDBusPendingCallWatcher*)),
	    this,
	    SLOT(getPropertiesReply(QDBusPendingCallWatcher*)));

  }
}

void NetworkListModel::disconnectFromConnman(QString)
{
  if (m_manager) {
    delete m_manager; //we think that m_getPropertiesWatcher will be
		      //deleted due to m_manager getting deleted
    beginResetModel();
    m_manager = NULL;
    m_networks.clear();
    endResetModel();
  }
}

void NetworkListModel::getPropertiesReply(QDBusPendingCallWatcher *call)
{
	Q_ASSERT(call);
	QDBusPendingReply<QVariantMap> reply = *call;
	if (reply.isError())
	{
		qDebug()<<"Error getPropertiesReply: "<<reply.error().message();
		disconnectFromConnman();
		//TODO set up timer to reconnect in a bit
		QTimer::singleShot(10000,this,SLOT(connectToConnman()));
	}
	else
	{
		///reset everything just in case:
		beginResetModel();
		m_networks.clear();
		endResetModel();

		m_propertiesCache = reply.value();
		QList<QDBusObjectPath> services = qdbus_cast<QList<QDBusObjectPath> >(m_propertiesCache["Services"]);
		beginInsertRows(QModelIndex(), 0, services.count()-1);
		foreach (QDBusObjectPath p, services)
		{
			qDebug()<< QString("service path:\t%1").arg(p.path());
			NetworkItemModel *pNIM = new NetworkItemModel(p.path(), this);
			connect(pNIM,SIGNAL(propertyChanged()),this,SLOT(itemPropertyChanged()));
			connect(pNIM,SIGNAL(stateChanged(NetworkItemModel::StateType)),
					this,SLOT(itemStateChanged(NetworkItemModel::StateType)));
			itemStateChanged(pNIM->state());
			m_networks.append(pNIM);
		}
		endInsertRows();
		connect(m_manager,
				SIGNAL(PropertyChanged(const QString&, const QDBusVariant&)),
				this,
				SLOT(propertyChanged(const QString&, const QDBusVariant&)));
		emitTechnologiesChanged();
		emit defaultTechnologyChanged(m_propertiesCache[DefaultTechnology].toString());
		emit stateChanged(m_propertiesCache[State].toString());
	}
}

void NetworkListModel::connectServiceReply(QDBusPendingCallWatcher *call)
{
  Q_ASSERT(call);
  QDBusPendingReply<QDBusObjectPath> reply = *call;
  if (reply.isError()) {
    QDBusError error = reply.error();
	qDebug("ERROR!");
	qDebug("message: %s", STR(error.message()));
	qDebug("name: %s", STR(error.name()));
	qDebug("error type: %s", STR(QDBusError::errorString(error.type())));
  } else {
    QDBusObjectPath p = reply.value();
	qDebug("object path: %s", STR(p.path()));
  }
}

void NetworkListModel::propertyChanged(const QString &name,
				       const QDBusVariant &value)
{
	qDebug("NetworkListModel: Property \"%s\" changed", STR(name));
	m_propertiesCache[name] = value.variant();

	if (name == NetworkListModel::availTechs ||
		name == NetworkListModel::enablTechs ||
		name == NetworkListModel::connTechs)
	{
		emitTechnologiesChanged();
	}
	else if (name == DefaultTechnology) {
		emit defaultTechnologyChanged(m_propertiesCache[DefaultTechnology].toString());
	}
	else if (name == State) {
		emit stateChanged(m_propertiesCache[State].toString());
	}
	else if (name == "Services")
	{
		QList<QDBusObjectPath> new_services =
				qdbus_cast<QList<QDBusObjectPath> >(value.variant());
		int num_new = new_services.count();
		qDebug()<<"number of services: "<<num_new;
		for (int i = 0; i < num_new; i++)
		{
			QDBusObjectPath path(new_services[i]);
			int j = findNetworkItemModel(path);
			if (-1 == j)
			{
				//beginInsertRows(QModelIndex(), i, i+1);
				beginInsertRows(QModelIndex(), i, i);
				NetworkItemModel *pNIM = new NetworkItemModel(path.path());
				connect(pNIM,SIGNAL(propertyChanged()),this,SLOT(itemPropertyChanged()));
				connect(pNIM,SIGNAL(stateChanged(NetworkItemModel::StateType)),
						this,SLOT(itemStateChanged(NetworkItemModel::StateType)));
				m_networks.insert(i, pNIM);
				endInsertRows();
				countChanged(m_networks.count());
			}
			else
			{
				if (i != j)
				{ //only move if from and to aren't the same
					beginMoveRows(QModelIndex(), j, j, QModelIndex(), i);
					NetworkItemModel *pNIM = m_networks[j];
					Q_ASSERT(pNIM);
					m_networks.remove(j);
					m_networks.insert(i, pNIM);
					endMoveRows();

					///We may not need this here:
					countChanged(m_networks.count());
				}
			}

		}
		int num_old = m_networks.count();
		if (num_old > num_new)
		{
			//DCP_CRITICAL(QString("num old: %1  num new: %2").arg(num_old).arg(num_new).toAscii());
			//DCP_CRITICAL(QString("we have %1 networks to remove").arg(num_old - num_new).toAscii());
			beginRemoveRows(QModelIndex(), num_new, num_old - 1);

			for (int i = num_new; i < num_old; i++)
			{
				//DCP_CRITICAL(QString("removing network %1").arg(m_networks[i]->servicePath()).toAscii());
				//	m_networks[i]->decreaseReferenceCount();
				m_networks[i]->deleteLater();
			}
			m_networks.remove(num_new, num_old - num_new);

			endRemoveRows();
			countChanged(m_networks.count());
		}
	}
	else if (name == OfflineMode)
	{
		m_propertiesCache[OfflineMode] = value.variant();
		emit offlineModeChanged(m_propertiesCache[OfflineMode].toBool());
	}
}

 void NetworkListModel::networkItemModified(const QList<const char *> &members)
 {
   //this is a gross hack to keep from doing things again and again we
   //only really care for the sake of the model when the type changes
   //the type changes as a result from the initial getProperties call
   //on the service object after we get the service object paths from
   //the Manager in getProperties
   if (members.contains(NetworkItemModel::Type))
   {
	   int row = m_networks.indexOf(qobject_cast<NetworkItemModel*>(sender()));
	   if(row == -1)
	   {
		   qDebug()<<"caught property changed signal for network item not in our list";
		   return;
	   }
	   emit dataChanged(createIndex(row, 0), createIndex(row, 0));
   }
 }

 void NetworkListModel::itemPropertyChanged()
 {
	 int row = m_networks.indexOf(qobject_cast<NetworkItemModel*>(sender()));
	 if(row == -1)
	 {
		 Q_ASSERT(0);
		 qDebug()<<"caught property changed signal for network item not in our list";
		 return;
	 }
	 qDebug()<<"Properties changed for "<< m_networks[row]->name();
	 emit dataChanged(createIndex(row, 0), createIndex(row, 0));
 }

 void NetworkListModel::countChangedSlot(int newCount)
 {
     if(newCount > 0)
     {
         NetworkItemModel* tempItem = m_networks.at(0);
         if(tempItem != m_defaultRoute &&
                 (tempItem->state() == NetworkItemModel::StateReady || NetworkItemModel::StateOnline))
         {
             m_defaultRoute = m_networks.at(0);
             defaultRouteChanged(m_defaultRoute);
         }
     }
     else
     {
         //m_defaultRoute = NULL;
         //defaultRouteChanged(m_defaultRoute);
     }
 }

 void NetworkListModel::itemStateChanged(NetworkItemModel::StateType)
 {
     connectedNetworkItemsChanged();
 }

int NetworkListModel::findNetworkItemModel(const QDBusObjectPath &path) const
{
	for (int i= 0; i < m_networks.count(); i++)
	{
		if (m_networks[i]->servicePath() == path.path()) return i;
	}
	return -1;
}

 void NetworkListModel::emitTechnologiesChanged()
 {
   const QStringList availableTechnologies = qdbus_cast<QStringList>
     (m_propertiesCache[NetworkListModel::availTechs]);
   const QStringList enabledTechnologies = qdbus_cast<QStringList>
     (m_propertiesCache[NetworkListModel::enablTechs]);
   const QStringList connectedTechnologies = qdbus_cast<QStringList>
     (m_propertiesCache[NetworkListModel::connTechs]);
   qDebug() << availTechs << ": " << m_propertiesCache[NetworkListModel::availTechs];
   qDebug() << enablTechs << ": " << m_propertiesCache[NetworkListModel::enablTechs];
   qDebug() << connTechs << ": " << m_propertiesCache[NetworkListModel::connTechs];
   emit technologiesChanged(availableTechnologies,
			    enabledTechnologies,
			    connectedTechnologies);

   availableTechnologiesChanged(availableTechnologies);
   enabledTechnologiesChanged(enabledTechnologies);
   connectedTechnologiesChanged(connectedTechnologies);
   countChanged(m_networks.count());
 }


void NetworkListModel::timerEvent(QTimerEvent *event)
{
  Q_UNUSED(event);
//  DCP_CRITICAL("Dumping list of networks");
  foreach(NetworkItemModel* pNIM, m_networks) {
    pNIM->dump();
  }
}

void NetworkListModel::requestScan()
{
  const QString wifi("wifi");
  m_manager->RequestScan(wifi);
  //FIXME: error returns, etc
}

void NetworkListModel::setOfflineMode(const bool &offlineMode)
{
 if(!m_manager) return;

    QDBusPendingReply<void> reply =
      m_manager->SetProperty(OfflineMode, QDBusVariant(QVariant(offlineMode)));


#if 0 //It is fundamentally broken to wait here and it doesn't even do
      //anything but crash the app
    reply.waitForFinished();
    if (reply.isError()) {
	  qDebug("got error from setProperty");
      throw -1;
    }
#endif
}

bool NetworkListModel::offlineMode() const
{
  return m_propertiesCache[OfflineMode].toBool();
}

QString NetworkListModel::defaultTechnology() const
{
  return m_propertiesCache[DefaultTechnology].toString();
}

QString NetworkListModel::state() const
{
  return m_propertiesCache[State].toString();
}
