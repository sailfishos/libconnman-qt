/*   -*- Mode: C++ -*-
 * meegotouchcp-connman - connectivity plugin for duicontrolpanel
 * Copyright © 2010, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
#include "networkmanager.h"
#include "debug.h"

static NetworkManager* staticInstance = NULL;

NetworkManager* NetworkManagerFactory::createInstance()
{
    if(!staticInstance)
        staticInstance = new NetworkManager;

    return staticInstance;
}



const QString NetworkManager::availTechs("AvailableTechnologies");
const QString NetworkManager::enablTechs("EnabledTechnologies");
const QString NetworkManager::connTechs("ConnectedTechnologies");
const QString NetworkManager::OfflineMode("OfflineMode");
const QString NetworkManager::DefaultTechnology("DefaultTechnology");
const QString NetworkManager::State("State");

NetworkManager::NetworkManager(QObject* parent)
  : QObject(parent),
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

  connect(this,SIGNAL(countChanged(int)),this,SLOT(countChangedSlot(int)));
}

NetworkManager::~NetworkManager()
{
}

void NetworkManager::setProperty(const int &index, QString property, const QVariant &value)
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

void NetworkManager::enableTechnology(const QString &technology)
{
  qDebug("enabling technology \"%s\"", STR(technology));
  QDBusReply<void> reply = m_manager->EnableTechnology(technology);
  if(reply.error().isValid())
  {
	  qDebug()<<reply.error().message();
	  enabledTechnologiesChanged(enabledTechnologies());
  }
}

void NetworkManager::disableTechnology(const QString &technology)
{
  qDebug("disenabling technology \"%s\"", STR(technology));
  m_manager->DisableTechnology(technology);
}

NetworkItemModel* NetworkManager::service(QString name)
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

void NetworkManager::connectService(const QString &name, const QString &security,
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

void NetworkManager::connectService(const QString &name)
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

const QStringList NetworkManager::availableTechnologies() const
{
  return qdbus_cast<QStringList>
    (m_propertiesCache[NetworkManager::availTechs]);
}

const QStringList NetworkManager::enabledTechnologies() const
{
  return qdbus_cast<QStringList>
      (m_propertiesCache[NetworkManager::enablTechs]);
}

const QStringList NetworkManager::connectedTechnologies() const
{
  return qdbus_cast<QStringList>
      (m_propertiesCache[NetworkManager::connTechs]);
}

void NetworkManager::setDefaultRoute(NetworkItemModel *item)
{
    if(m_networks.count() > 0 && (item->state() == NetworkItemModel::StateReady || item->state() == NetworkItemModel::StateOnline))
    {
        NetworkItemModel * topservice = m_networks.at(0);

        item->moveBefore(topservice);
    }
}

void NetworkManager::connectToConnman(QString)
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

void NetworkManager::disconnectFromConnman(QString)
{
  if (m_manager) {
    delete m_manager; //we think that m_getPropertiesWatcher will be
		      //deleted due to m_manager getting deleted

    m_manager = NULL;
    int count = m_networks.count();
    m_networks.clear();
    networksRemoved(0, count);
  }
}

void NetworkManager::getPropertiesReply(QDBusPendingCallWatcher *call)
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
		int count = m_networks.count();
		m_networks.clear();

		if(count > 0)
			networksRemoved(0, count);

		m_propertiesCache = reply.value();
		QList<QDBusObjectPath> services = qdbus_cast<QList<QDBusObjectPath> >(m_propertiesCache["Services"]);
		//beginInsertRows(QModelIndex(), 0, services.count()-1);

		foreach (QDBusObjectPath p, services)
		{
			qDebug()<< QString("service path:\t%1").arg(p.path());
			NetworkItemModel *pNIM = new NetworkItemModel(p.path(), this);
			connect(pNIM,SIGNAL(propertyChanged(QString, QVariant)),this,SLOT(itemPropertyChanged(QString, QVariant)));
			connect(pNIM,SIGNAL(stateChanged(NetworkItemModel::StateType)),
					this,SLOT(itemStateChanged(NetworkItemModel::StateType)));
			itemStateChanged(pNIM->state());
			m_networks.append(pNIM);
		}
		//endInsertRows();
		networksAdded(0,services.count() - 1);
		connect(m_manager,
				SIGNAL(PropertyChanged(const QString&, const QDBusVariant&)),
				this,
				SLOT(propertyChanged(const QString&, const QDBusVariant&)));
		emitTechnologiesChanged();
		emit defaultTechnologyChanged(m_propertiesCache[DefaultTechnology].toString());
		emit stateChanged(m_propertiesCache[State].toString());
	}
}

void NetworkManager::connectServiceReply(QDBusPendingCallWatcher *call)
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

void NetworkManager::propertyChanged(const QString &name,
				       const QDBusVariant &value)
{
	qDebug("NetworkListModel: Property \"%s\" changed", STR(name));
	m_propertiesCache[name] = value.variant();

	if (name == NetworkManager::availTechs ||
		name == NetworkManager::enablTechs ||
		name == NetworkManager::connTechs)
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
				//beginInsertRows(QModelIndex(), i, i);
				NetworkItemModel *pNIM = new NetworkItemModel(path.path());
				connect(pNIM,SIGNAL(propertyChanged(QString, QVariant)),this,SLOT(itemPropertyChanged(QString, QVariant)));
				connect(pNIM,SIGNAL(stateChanged(NetworkItemModel::StateType)),
						this,SLOT(itemStateChanged(NetworkItemModel::StateType)));
				m_networks.insert(i, pNIM);
				//endInsertRows();
				networksAdded(i, i);
				countChanged(m_networks.count());
			}
			else
			{
				if (i != j)
				{ //only move if from and to aren't the same
					//beginMoveRows(QModelIndex(), j, j, QModelIndex(), i);
					NetworkItemModel *pNIM = m_networks[j];
					Q_ASSERT(pNIM);
					m_networks.remove(j);
					m_networks.insert(i, pNIM);
					//endMoveRows();
					networksMoved(j,j,i);

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
			//beginRemoveRows(QModelIndex(), num_new, num_old - 1);

			for (int i = num_new; i < num_old; i++)
			{
				//DCP_CRITICAL(QString("removing network %1").arg(m_networks[i]->servicePath()).toAscii());
				//	m_networks[i]->decreaseReferenceCount();
				m_networks[i]->deleteLater();
			}
			m_networks.remove(num_new, num_old - num_new);

			networksRemoved(num_new, num_old - 1);

			countChanged(m_networks.count());
		}
	}
	else if (name == OfflineMode)
	{
		m_propertiesCache[OfflineMode] = value.variant();
		emit offlineModeChanged(m_propertiesCache[OfflineMode].toBool());
	}
}

 void NetworkManager::networkItemModified(const QList<const char *> &members)
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
	   networkChanged(row);
   }
 }

 void NetworkManager::itemPropertyChanged(QString name, QVariant value)
 {
	 int row = m_networks.indexOf(qobject_cast<NetworkItemModel*>(sender()));
	 if(row == -1)
	 {
		 //Q_ASSERT(0);
		 qDebug()<<"caught property changed signal for network item not in our list";
		 return;
	 }
	 qDebug()<<"Properties changed for "<< m_networks[row]->name();
	 networkChanged(row);
 }

 void NetworkManager::countChangedSlot(int newCount)
 {
     if(newCount > 0)
     {
         NetworkItemModel* tempItem = m_networks.at(0);
         if(tempItem != m_defaultRoute &&
                 (tempItem->state() == NetworkItemModel::StateReady ||
                  tempItem->state() == NetworkItemModel::StateOnline))
         {
             m_defaultRoute = m_networks.at(0);
             defaultRouteChanged(m_defaultRoute);
         }
     }
     else
     {
         m_defaultRoute = NULL;
         defaultRouteChanged(m_defaultRoute);
     }
 }

 void NetworkManager::itemStateChanged(NetworkItemModel::StateType itemState)
 {
     connectedNetworkItemsChanged();

     NetworkItemModel* item = qobject_cast<NetworkItemModel*>(sender());
     if(m_networks.count() && item == m_networks.at(0) )
     {
         if(itemState == NetworkItemModel::StateReady ||
                 itemState == NetworkItemModel::StateOnline)
         {
             m_defaultRoute = m_networks.at(0);
         }
         else
         {
             m_defaultRoute = NULL;
         }
         defaultRouteChanged(m_defaultRoute);
     }
 }

int NetworkManager::findNetworkItemModel(const QDBusObjectPath &path) const
{
	for (int i= 0; i < m_networks.count(); i++)
	{
		if (m_networks[i]->servicePath() == path.path()) return i;
	}
	return -1;
}

 void NetworkManager::emitTechnologiesChanged()
 {
   const QStringList availableTechnologies = qdbus_cast<QStringList>
     (m_propertiesCache[NetworkManager::availTechs]);
   const QStringList enabledTechnologies = qdbus_cast<QStringList>
     (m_propertiesCache[NetworkManager::enablTechs]);
   const QStringList connectedTechnologies = qdbus_cast<QStringList>
     (m_propertiesCache[NetworkManager::connTechs]);
   qDebug() << availTechs << ": " << m_propertiesCache[NetworkManager::availTechs];
   qDebug() << enablTechs << ": " << m_propertiesCache[NetworkManager::enablTechs];
   qDebug() << connTechs << ": " << m_propertiesCache[NetworkManager::connTechs];
   emit technologiesChanged(availableTechnologies,
			    enabledTechnologies,
			    connectedTechnologies);

   availableTechnologiesChanged(availableTechnologies);
   enabledTechnologiesChanged(enabledTechnologies);
   connectedTechnologiesChanged(connectedTechnologies);
   countChanged(m_networks.count());
 }


void NetworkManager::timerEvent(QTimerEvent *event)
{
  Q_UNUSED(event);
//  DCP_CRITICAL("Dumping list of networks");
  foreach(NetworkItemModel* pNIM, m_networks) {
    pNIM->dump();
  }
}

void NetworkManager::requestScan()
{
  if (!m_manager) return;
  const QString wifi("wifi");
  m_manager->RequestScan(wifi);
  //FIXME: error returns, etc
}

void NetworkManager::setOfflineMode(const bool &offlineMode)
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

bool NetworkManager::offlineMode() const
{
  return m_propertiesCache[OfflineMode].toBool();
}

QString NetworkManager::defaultTechnology() const
{
  return m_propertiesCache[DefaultTechnology].toString();
}

QString NetworkManager::state() const
{
  return m_propertiesCache[State].toString();
}
