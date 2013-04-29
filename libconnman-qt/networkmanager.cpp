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

#include "commondbustypes.h"
#include "connman_manager_interface.h"
#include "connman_manager_interface.cpp" // not bug
#include "moc_connman_manager_interface.cpp" // not bug

#include "debug.h"

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
const QString NetworkManager::SessionMode("SessionMode");

NetworkManager::NetworkManager(QObject* parent)
  : QObject(parent),
    m_manager(NULL),
    m_defaultRoute(NULL),
    watcher(NULL),
    m_available(false)
{
    registerCommonDataTypes();

    watcher = new QDBusServiceWatcher("net.connman",QDBusConnection::systemBus(),
            QDBusServiceWatcher::WatchForRegistration |
            QDBusServiceWatcher::WatchForUnregistration, this);
    connect(watcher, SIGNAL(serviceRegistered(QString)),
            this, SLOT(connectToConnman(QString)));
    connect(watcher, SIGNAL(serviceUnregistered(QString)),
            this, SLOT(connmanUnregistered(QString)));


    m_available = QDBusConnection::systemBus().interface()->isServiceRegistered("net.connman");

    if(m_available)
        connectToConnman();
    else
        qDebug() << "connman not AVAILABLE";
}

NetworkManager::~NetworkManager()
{
}

void NetworkManager::connectToConnman(QString)
{
    disconnectFromConnman();
    m_manager = new NetConnmanManagerInterface("net.connman", "/",
            QDBusConnection::systemBus(), this);

    if (!m_manager->isValid()) {

        delete m_manager;
        m_manager = NULL;

        // shouldn't happen but in this case service isn't available
        if(m_available)
            emit availabilityChanged(m_available = false);
    } else {

        QDBusPendingReply<QVariantMap> props_reply = m_manager->GetProperties();
        props_reply.waitForFinished();
        if (!props_reply.isError()) {
            m_propertiesCache = props_reply.value();

            emit stateChanged(m_propertiesCache[State].toString());

            connect(m_manager,
                    SIGNAL(PropertyChanged(const QString&, const QDBusVariant&)),
                    this,
                    SLOT(propertyChanged(const QString&, const QDBusVariant&)));
        }
        setupTechnologies();
        setupServices();

        if(!m_available)
            emit availabilityChanged(m_available = true);

        pr_dbg() << "Connected";
    }
}

void NetworkManager::disconnectFromConnman(QString)
{
    if (m_manager) {
        delete m_manager;
        m_manager = NULL;
    }

    foreach (QString skey, m_servicesCache.keys()) {
        m_servicesCache.value(skey)->deleteLater();
        m_servicesCache.remove(skey);
    }
    m_servicesOrder.clear();
    emit servicesChanged();

    foreach (QString tkey, m_technologiesCache.keys()) {
        m_technologiesCache.value(tkey)->deleteLater();
        m_technologiesCache.remove(tkey);
    }
    emit technologiesChanged();
}


void NetworkManager::connmanUnregistered(QString)
{
    disconnectFromConnman();

    if(m_available)
        emit availabilityChanged(m_available = false);
}

void NetworkManager::setupTechnologies()
{
    QDBusPendingReply<ConnmanObjectList> reply = m_manager->GetTechnologies();
    reply.waitForFinished();
    if (!reply.isError()) {
        ConnmanObjectList lst = reply.value();
        ConnmanObject obj;
        foreach (obj, lst) { // TODO: consider optimizations

            NetworkTechnology *tech = new NetworkTechnology(obj.objpath.path(),
                                                            obj.properties, this);

            m_technologiesCache.insert(tech->type(), tech);
        }

        connect(m_manager,
                SIGNAL(TechnologyAdded(const QDBusObjectPath &, const QVariantMap &)),
                this,
                SLOT(technologyAdded(const QDBusObjectPath &, const QVariantMap &)));

        connect(m_manager,
                SIGNAL(TechnologyRemoved(const QDBusObjectPath &)),
                this,
                SLOT(technologyRemoved(const QDBusObjectPath &)));

        emit technologiesChanged();
    }
}

void NetworkManager::setupServices()
{
    QDBusPendingReply<ConnmanObjectList> reply = m_manager->GetServices();
    reply.waitForFinished();
    if (!reply.isError()) {

        ConnmanObjectList lst = reply.value();
        ConnmanObject obj;
        int order = -1;
        NetworkService *service = NULL;

        // make sure we don't leak memory
        m_servicesOrder.clear();

        foreach (obj, lst) { // TODO: consider optimizations
            order++;

            service = new NetworkService(obj.objpath.path(),
                                         obj.properties, this);

            m_servicesCache.insert(obj.objpath.path(), service);
            m_servicesOrder.push_back(service);

            // by connman's documentation, first service is always
            // the default route's one
            if (order == 0)
                updateDefaultRoute(service);
        }

        // if no service was replied
        if (order == -1)
            updateDefaultRoute(NULL);

        emit servicesChanged();

        connect(m_manager,
                SIGNAL(ServicesChanged(ConnmanObjectList, QList<QDBusObjectPath>)),
                this,
                SLOT(updateServices(ConnmanObjectList, QList<QDBusObjectPath>)));

    }
}

void NetworkManager::updateServices(const ConnmanObjectList &changed, const QList<QDBusObjectPath> &removed)
{
    ConnmanObject connmanobj;
    int order = -1;
    NetworkService *service = NULL;

    // make sure we don't leak memory
    m_servicesOrder.clear();
    QStringList serviceList;
    foreach (connmanobj, changed) {
        order++;
        bool addedService = false;

        if (!m_servicesCache.contains(connmanobj.objpath.path())) {
            service = new NetworkService(connmanobj.objpath.path(),
                                         connmanobj.properties, this);
            m_servicesCache.insert(connmanobj.objpath.path(), service);
            addedService = true;
        } else {
            service = m_servicesCache.value(connmanobj.objpath.path());
        }
        m_servicesOrder.push_back(service);
        serviceList.push_back(service->path());

        if (order == 0)
            updateDefaultRoute(service);
        if (addedService) { //emit this after m_servicesOrder is updated
            Q_EMIT serviceAdded(connmanobj.objpath.path());
        }
    }

    foreach (QDBusObjectPath obj, removed) {
        if (m_servicesCache.contains(obj.path())) {
            m_servicesCache.value(obj.path())->deleteLater();
            m_servicesCache.remove(obj.path());
            Q_EMIT serviceRemoved(obj.path());
        } else {
            // connman maintains a virtual "hidden" wifi network and removes it upon init
            pr_dbg() << "attempted to remove non-existing service";
        }
    }

    if (order == -1)
        updateDefaultRoute(NULL);

    emit servicesChanged();
    Q_EMIT servicesListChanged(serviceList);
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
    QVariant tmp = value.variant();

    m_propertiesCache[name] = tmp;
    if (name == State) {
        emit stateChanged(tmp.toString());
    } else if (name == OfflineMode) {
        emit offlineModeChanged(tmp.toBool());
    } else if (name == SessionMode) {
       emit sessionModeChanged(tmp.toBool());
   }
}

void NetworkManager::technologyAdded(const QDBusObjectPath &technology,
                                     const QVariantMap &properties)
{
    NetworkTechnology *tech = new NetworkTechnology(technology.path(),
                                                    properties, this);

    m_technologiesCache.insert(tech->type(), tech);
    emit technologiesChanged();
}

void NetworkManager::technologyRemoved(const QDBusObjectPath &technology)
{
    NetworkTechnology *net;
    // if we weren't storing by type() this loop would be unecessary
    // but since this function will be triggered rarely that's fine
    foreach (net, m_technologiesCache) {
        if (net->objPath() == technology.path()) {

            pr_dbg() << "Removing " << net->objPath();
            m_technologiesCache.remove(net->type());
            net->deleteLater();

            break;
        }
    }

    emit technologiesChanged();
}


// Public API /////////////

// Getters


bool NetworkManager::isAvailable() const
{
    return m_available;
}


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
        pr_dbg() << "Technology " << type << " doesn't exist";
        return NULL;
    }
}

const QVector<NetworkTechnology *> NetworkManager::getTechnologies() const
{
    QVector<NetworkTechnology *> techs;

    foreach (NetworkTechnology *tech, m_technologiesCache) {
        techs.push_back(tech);
    }

    return techs;
}

const QVector<NetworkService*> NetworkManager::getServices(const QString &tech) const
{
    QVector<NetworkService *> services;

    // this foreach is based on the m_servicesOrder to keep connman's sort
    // of services.
    foreach (NetworkService *service, m_servicesOrder) {
        if (tech.isEmpty() || service->type() == tech)
            services.push_back(service);
    }

    return services;
}

// Setters

void NetworkManager::setOfflineMode(const bool &offlineMode)
{
    if(!m_manager) return;

    QDBusPendingReply<void> reply =
        m_manager->SetProperty(OfflineMode,
                               QDBusVariant(QVariant(offlineMode)));
}

  // these shouldn't crash even if connman isn't available
void NetworkManager::registerAgent(const QString &path)
{
    if(m_manager)
        m_manager->RegisterAgent(QDBusObjectPath(path));
}

void NetworkManager::unregisterAgent(const QString &path)
{
    if(m_manager)
        m_manager->UnregisterAgent(QDBusObjectPath(path));
}

void NetworkManager::registerCounter(const QString &path, quint32 accuracy,quint32 period)
{
    if(m_manager)
        m_manager->RegisterCounter(QDBusObjectPath(path),accuracy, period);
}

void NetworkManager::unregisterCounter(const QString &path)
{
    if(m_manager)
        m_manager->UnregisterCounter(QDBusObjectPath(path));
}

QDBusObjectPath NetworkManager::createSession(const QVariantMap &settings, const QString &sessionNotifierPath)
{
    if(!m_manager)
        return QDBusObjectPath();

    QDBusPendingReply<QDBusObjectPath> reply =
        m_manager->CreateSession(settings,QDBusObjectPath(sessionNotifierPath));
    reply.waitForFinished();
    return reply.value();
}

void NetworkManager::destroySession(const QString &sessionAgentPath)
{
    if(m_manager)
        m_manager->DestroySession(QDBusObjectPath(sessionAgentPath));
}

void NetworkManager::setSessionMode(const bool &sessionMode)
{
    if(m_manager)
        m_manager->SetProperty(SessionMode, QDBusVariant(sessionMode));
}

bool NetworkManager::sessionMode() const
{
    return m_propertiesCache[SessionMode].toBool();
}

QStringList NetworkManager::servicesList(const QString &tech)
{
    QStringList services;
    foreach (NetworkService *service, m_servicesOrder) {
        if (tech.isEmpty() || service->type() == tech)
            services.push_back(service->path());
    }
    return services;
}

QString NetworkManager::technologyPathForService(const QString &servicePath)
{
    foreach (NetworkService *service, m_servicesOrder) {
        if (service->path() == servicePath)
            return service->path();
    }
    return QString();
}
QString NetworkManager::technologyPathForType(const QString &techType)
{
    foreach (NetworkTechnology *tech, m_technologiesCache) {
        if (tech->type() == techType)
            return tech->path();
    }
    return QString();
}

QStringList NetworkManager::technologiesList()
{
    QStringList techList;
    foreach (NetworkTechnology *tech, m_technologiesCache) {
        techList << tech->type();
    }
    return techList;
}
