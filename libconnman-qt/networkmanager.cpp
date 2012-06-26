/*
 * Copyright © 2010, Intel Corporation.
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "networkmanager.h"

static NetworkManager* staticInstance = NULL;

NetworkManager* NetworkManagerFactory::createInstance()
{
    if(!staticInstance)
        staticInstance = new NetworkManager;

    return staticInstance;
}


// NetworkManager implementation

const QString NetworkManager::State("State");
const QString NetworkManager::OfflineMode("OfflineMode");

NetworkManager::NetworkManager(QObject* parent)
  : QObject(parent),
    m_manager(NULL),
    m_getPropertiesWatcher(NULL),
    m_getTechnologiesWatcher(NULL),
    m_getServicesWatcher(NULL),
    m_defaultRoute(NULL),
    watcher(NULL)
{
    registerCommonDataTypes();
    connectToConnman();
}

NetworkManager::~NetworkManager() {}

void NetworkManager::connectToConnman(QString)
{
    if(!watcher) {
        watcher = new QDBusServiceWatcher("net.connman",QDBusConnection::systemBus(),
                QDBusServiceWatcher::WatchForRegistration |
                QDBusServiceWatcher::WatchForUnregistration, this);
        connect(watcher, SIGNAL(serviceRegistered(QString)),
                this, SLOT(connectToConnman(QString)));
        connect(watcher, SIGNAL(serviceUnregistered(QString)),
                this, SLOT(disconnectFromConnman(QString)));
    }

    disconnectFromConnman();
    m_manager = new Manager("net.connman", "/",
            QDBusConnection::systemBus(), this);

    if (!m_manager->isValid()) {
        qDebug("manager is invalid. connman may not be running or is invalid");
        delete m_manager;
        m_manager = NULL;
    } else {
        QDBusPendingReply<QVariantMap> props_reply = m_manager->GetProperties();
        m_getPropertiesWatcher = new QDBusPendingCallWatcher(props_reply, m_manager);
        connect(m_getPropertiesWatcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                this,
                SLOT(getPropertiesReply(QDBusPendingCallWatcher*)));

        QDBusPendingReply<ConnmanObjectList> techs_reply = m_manager->GetTechnologies();
        m_getTechnologiesWatcher = new QDBusPendingCallWatcher(techs_reply, m_manager);
        connect(m_getTechnologiesWatcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                this,
                SLOT(getTechnologiesReply(QDBusPendingCallWatcher*)));

        QDBusPendingReply<ConnmanObjectList> services_reply = m_manager->GetServices();
        m_getServicesWatcher = new QDBusPendingCallWatcher(services_reply, m_manager);
        connect(m_getServicesWatcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                this,
                SLOT(getServicesReply(QDBusPendingCallWatcher*)));

    }
    qDebug("connected");
}

void NetworkManager::disconnectFromConnman(QString)
{
    if (m_manager) {
        delete m_manager;
        m_manager = NULL;
    }
    // FIXME: should we delete technologies and services?
}

// These functions is a part of setup procedure

void NetworkManager::getPropertiesReply(QDBusPendingCallWatcher *call)
{
    Q_ASSERT(call);
    QDBusPendingReply<QVariantMap> reply = *call;
    if (reply.isError()) {
        qDebug() << "Error getPropertiesReply: " << reply.error().message();
        disconnectFromConnman();
        // TODO: set up timer to reconnect in a bit
        QTimer::singleShot(10000,this,SLOT(connectToConnman()));
    } else {
        m_propertiesCache = reply.value();
        emit stateChanged(m_propertiesCache[State].toString());
        qDebug() << "State: " << m_propertiesCache["State"].toString();
        connect(m_manager,
                SIGNAL(PropertyChanged(const QString&, const QDBusVariant&)),
                this,
                SLOT(propertyChanged(const QString&, const QDBusVariant&)));
    }
}

void NetworkManager::getTechnologiesReply(QDBusPendingCallWatcher *call)
{
    Q_ASSERT(call);
    QDBusPendingReply<ConnmanObjectList> reply = *call;
    if (reply.isError()) {
        qDebug() << "Error getTechnologiesReply: " << reply.error().message();
        disconnectFromConnman();
        // TODO: set up timer to reconnect in a bit
        //QTimer::singleShot(10000,this,SLOT(connectToConnman()));
    } else {
        ConnmanObjectList lst = reply.value();
        ConnmanObject obj;
        foreach (obj, lst) { // TODO: consider optimizations
            NetworkTechnology *tech = new NetworkTechnology(obj.objpath.path(),
                    obj.properties, this);
            m_technologiesCache.insert(tech->type(), tech);
            qDebug() << "From NT: " << tech->type() << " " << tech->connected() << " " << tech->powered();
        }
        if (!m_technologiesCache.isEmpty()) {
            emit technologiesChanged(m_technologiesCache, QStringList());
        }
    }
    qDebug() << "Got reply with technolgies";
}

void NetworkManager::getServicesReply(QDBusPendingCallWatcher *call)
{
    Q_ASSERT(call);
    QDBusPendingReply<ConnmanObjectList> reply = *call;
    if (reply.isError()) {
        qDebug() << "Error getServicesReply: " << reply.error().message();
        disconnectFromConnman();
        // TODO: set up timer to reconnect in a bit
        //QTimer::singleShot(10000,this,SLOT(connectToConnman()));
    } else {
        ConnmanObjectList lst = reply.value();
        ConnmanObject obj;
        int order = -1;
        foreach (obj, lst) { // TODO: consider optimizations
            order++;
            NetworkService *service = new NetworkService(obj.objpath.path(),
                    obj.properties, this);
            m_servicesCache.insert(obj.objpath.path(), service);
            m_servicesOrder.insert(obj.objpath.path(), order);
            qDebug() << "From Service: " << obj.objpath.path();
            if (order == 0) {
                updateDefaultRoute(service);
            }
        }
        if (order == -1) {
            updateDefaultRoute(NULL);
        }
        emit servicesChanged();
        connect(m_manager,
                SIGNAL(ServicesChanged(ConnmanObjectList, QList<QDBusObjectPath>)),
                this,
                SLOT(updateServices(ConnmanObjectList, QList<QDBusObjectPath>)));

    }
    qDebug() << "Got reply with services";
}

void NetworkManager::updateServices(const ConnmanObjectList &changed, const QList<QDBusObjectPath> &removed)
{
    qDebug() << "services changed: " << changed.size();
    foreach (QDBusObjectPath obj, removed) {
        qDebug() << "removing " << obj.path();
        m_servicesCache.value(obj.path())->deleteLater();
        m_servicesCache.remove(obj.path());
        m_servicesOrder.remove(obj.path());
    }
    ConnmanObject connmanobj;
    int order = -1;
    foreach (connmanobj, changed) {
        order++;
        m_servicesOrder.insert(connmanobj.objpath.path(), order);
        if (!m_servicesCache.contains(connmanobj.objpath.path())) {
            NetworkService *service = new NetworkService(connmanobj.objpath.path(),
                    connmanobj.properties, this);
            m_servicesCache.insert(connmanobj.objpath.path(), service);
            qDebug() << "Added service " << connmanobj.objpath.path();
        }
        if (order == 0) {
            updateDefaultRoute(m_servicesCache.value(connmanobj.objpath.path()));
        }
    }
    if (order == -1) {
        updateDefaultRoute(NULL);
    }
    emit servicesChanged();
}

void NetworkManager::updateDefaultRoute(NetworkService* defaultRoute)
{
    if (m_defaultRoute != defaultRoute) {
        m_defaultRoute = defaultRoute;
        emit defaultRouteChanged(m_defaultRoute);
    }
}

void NetworkManager::propertyChanged(const QString &name,
        const QDBusVariant &value)
{
    qDebug() << "NetworkManager::propertyChanged(" << name << ")";
    m_propertiesCache[name] = value.variant();
    if (name == State) {
        emit stateChanged(m_propertiesCache[State].toString());
    } else if (name == OfflineMode) {
        emit offlineModeChanged(m_propertiesCache[OfflineMode].toBool());
    }
}

// Public API /////////////

// Getters

const QString NetworkManager::state() const
{
    return m_propertiesCache[State].toString();
}

bool NetworkManager::offlineMode() const
{
    return m_propertiesCache[OfflineMode].toBool();
}

NetworkService* NetworkManager::defaultRoute() const
{
    return m_defaultRoute;
}

NetworkTechnology* NetworkManager::getTechnology(const QString &type) const
{
    if (m_technologiesCache.contains(type))
        return m_technologiesCache.value(type);
    else {
        qDebug() << "Technology " << type << " doesn't exist";
        return NULL;
    }
}

const QVector<NetworkService*> NetworkManager::getServices() const
{
    QVector<NetworkService*> networks(m_servicesCache.size(), NULL);
    foreach (const QString &path, m_servicesCache.keys()) {
        networks[m_servicesOrder.value(path)] = m_servicesCache.value(path);
    }
    return networks; // TODO: optimizations
}

// Setters

void NetworkManager::setOfflineMode(const bool &offlineMode)
{
    if(!m_manager) return;

    QDBusPendingReply<void> reply =
        m_manager->SetProperty(OfflineMode,
                               QDBusVariant(QVariant(offlineMode)));
}

void NetworkManager::registerAgent(const QString &path)
{
    Q_ASSERT(m_manager);
    m_manager->RegisterAgent(QDBusObjectPath(path));
}

void NetworkManager::unregisterAgent(const QString &path)
{
    Q_ASSERT(m_manager);
    m_manager->UnregisterAgent(QDBusObjectPath(path));
}
