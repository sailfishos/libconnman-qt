/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2017 Jolla Ltd.
 * Contact: Slava Monich <slava.monich@jolla.com>
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "networkmanager.h"

#include "libconnman_p.h"
#include <QRegExp>

// ==========================================================================
// NetworkManagerFactory
// ==========================================================================

static NetworkManager* staticInstance = NULL;

NetworkManager* NetworkManagerFactory::createInstance()
{
    if (!staticInstance)
        staticInstance = new NetworkManager;

    return staticInstance;
}

NetworkManager* NetworkManagerFactory::instance()
{
    return createInstance();
}

// ==========================================================================
// NetworkManager::Private
// ==========================================================================

class NetworkManager::Private : QObject
{
    Q_OBJECT

public:
    static const QString InputRequestTimeout;
    static const uint DefaultInputRequestTimeout;

    static const QString WifiTechnology;
    static const QString CellularTechnology;
    static const QString BluetoothTechnology;
    static const QString GpsTechnology;

    static bool selectAll(NetworkService *service);
    static bool selectSaved(NetworkService *service);
    static bool selectAvailable(NetworkService *service);

    bool m_registered;

public:
    Private(NetworkManager *parent) :
        QObject(parent), m_registered(false) {}
    NetworkManager* manager()
        { return (NetworkManager*)parent(); }
    void maybeCreateInterfaceProxyLater()
        { QMetaObject::invokeMethod(this, "maybeCreateInterfaceProxy"); }

public Q_SLOTS:
    void maybeCreateInterfaceProxy();
};

const QString NetworkManager::Private::InputRequestTimeout("InputRequestTimeout");
const uint NetworkManager::Private::DefaultInputRequestTimeout(300000);

const QString NetworkManager::Private::WifiTechnology("/net/connman/technology/wifi");
const QString NetworkManager::Private::CellularTechnology("/net/connman/technology/cellular");
const QString NetworkManager::Private::BluetoothTechnology("/net/connman/technology/bluetooth");
const QString NetworkManager::Private::GpsTechnology("/net/connman/technology/gps");

bool NetworkManager::Private::selectAll(NetworkService *service)
{
    return service != NULL;
}

bool NetworkManager::Private::selectSaved(NetworkService *service)
{
    return service && service->saved();
}

bool NetworkManager::Private::selectAvailable(NetworkService *service)
{
    return service && service->available();
}

void NetworkManager::Private::maybeCreateInterfaceProxy()
{
    // Theoretically, connman may have become unregistered while this call
    // was sitting in the queue. We need to re-check the m_registered flag.
    if (m_registered) {
        NetworkManager* mgr = manager();
        if (!mgr->m_available) {
            mgr->setConnmanAvailable(true);
        }
    }
}

// ==========================================================================
// NetworkManager::InterfaceProxy
//
// qdbusxml2cpp doesn't really do much, it's easier to write these proxies
// by hand. Basically, this is what we have here:
//
// <interface name="net.connman.Manager">
//   <method name="GetProperties">
//     <arg name="properties" type="a{sv}" direction="out"/>
//   </method>
//   <method name="SetProperty">
//     <arg name="name" type="s"/>
//     <arg name="value" type="v"/>
//   </method>
//   <method name="GetTechnologies">
//     <arg name="technologies" type="a(oa{sv})" direction="out"/>
//   </method>
//   <method name="GetServices">
//     <arg name="services" type="a(oa{sv})" direction="out"/>
//   </method>
//   <method name="RegisterAgent">
//     <arg name="path" type="o"/>
//   </method>
//   <method name="UnregisterAgent">
//     <arg name="path" type="o"/>
//   </method>
//   <method name="RegisterCounter">
//     <arg name="path" type="o"/>
//     <arg name="accuracy" type="u"/>
//     <arg name="period" type="u"/>
//   </method>
//   <method name="UnregisterCounter">
//     <arg name="path" type="o"/>
//   </method>
//   <method name="ResetCounters">
//     <arg name="type" type="s"/>
//   </method>
//   <method name="CreateSession">
//     <arg name="settings" type="a{sv}"/>
//     <arg name="notifier" type="o"/>
//     <arg name="session" type="o" direction="out"/>
//   </method>
//   <method name="DestroySession">
//     <arg name="session" type="o"/>
//   </method>
//   <method name="CreateService">
//     <arg name="service_type" type="s"/>
//     <arg name="device_ident" type="s"/>
//     <arg name="network_ident" type="s"/>
//     <arg name="settings" type="a(ss)"/>
//     <arg name="service" type="o"/>
//   </method>
//   <signal name="TechnologyAdded">
//     <arg name="technology" type="o"/>
//     <arg name="properties" type="a{sv}"/>
//   </signal>
//   <signal name="TechnologyRemoved">
//     <arg name="technology" type="o"/>
//   </signal>
//   <signal name="ServicesChanged">
//     <arg name="changed" type="a(oa{sv})"/>
//     <arg name="removed" type="ao"/>
//   </signal>
//   <signal name="PropertyChanged">
//     <arg name="name" type="s"/>
//     <arg name="value" type="v"/>
//   </signal>
// </interface>
//
// ==========================================================================

class NetworkManager::InterfaceProxy: public QDBusAbstractInterface
{
    Q_OBJECT

public:
    InterfaceProxy(NetworkManager *parent) :
        QDBusAbstractInterface(CONNMAN_SERVICE, "/", "net.connman.Manager",
            CONNMAN_BUS, parent) {}

public:
    QDBusPendingCall GetProperties()
        { return asyncCall("GetProperties"); }
    QDBusPendingCall SetProperty(QString name, QVariant value)
        { return asyncCall("SetProperty", name, qVariantFromValue(QDBusVariant(value))); }
    QDBusPendingCall GetTechnologies()
        { return asyncCall("GetTechnologies"); }
    QDBusPendingCall GetServices()
        { return asyncCall("GetServices"); }
    QDBusPendingCall RegisterAgent(const QString &path)
        { return asyncCall("RegisterAgent", qVariantFromValue(QDBusObjectPath(path))); }
    QDBusPendingCall UnregisterAgent(const QString &path)
        { return asyncCall("UnregisterAgent", qVariantFromValue(QDBusObjectPath(path))); }
    QDBusPendingCall RegisterCounter(const QString &path, uint accuracy, uint period)
        { return asyncCall("RegisterCounter", qVariantFromValue(QDBusObjectPath(path)), accuracy, period); }
    QDBusPendingCall ResetCounters(const QString &type)
        { return asyncCall("ResetCounters", type); }
    QDBusPendingCall UnregisterCounter(const QString &path)
        { return asyncCall("UnregisterCounter", qVariantFromValue(QDBusObjectPath(path))); }
    QDBusPendingReply<QDBusObjectPath> CreateSession(const QVariantMap &settings, const QString &path)
        { return asyncCall("CreateSession", settings, qVariantFromValue(QDBusObjectPath(path))); }
    QDBusPendingCall DestroySession(const QString &path)
        { return asyncCall("DestroySession", qVariantFromValue(QDBusObjectPath(path))); }
    QDBusPendingReply<QDBusObjectPath> CreateService(const QString &type, const QString &device, const QString &network, const StringPairArray &settings)
        { return asyncCall("CreateService", type, device, network, qVariantFromValue(settings)); }

Q_SIGNALS:
    void PropertyChanged(const QString &name, const QDBusVariant &value);
    void ServicesChanged(ConnmanObjectList changed, const QList<QDBusObjectPath> &removed);
    void TechnologyAdded(const QDBusObjectPath &technology, const QVariantMap &properties);
    void TechnologyRemoved(const QDBusObjectPath &technology);
};

// ==========================================================================
// NetworkManager
// ==========================================================================

const QString NetworkManager::State("State");
const QString NetworkManager::OfflineMode("OfflineMode");
const QString NetworkManager::SessionMode("SessionMode");

NetworkManager::NetworkManager(QObject* parent)
  : QObject(parent),
    m_proxy(NULL),
    m_defaultRoute(NULL),
    m_invalidDefaultRoute(new NetworkService("/", QVariantMap(), this)),
    m_priv(new Private(this)),
    m_available(false),
    m_servicesEnabled(true),
    m_technologiesEnabled(true)
{
    registerCommonDataTypes();
    QDBusServiceWatcher* watcher = new QDBusServiceWatcher(CONNMAN_SERVICE, CONNMAN_BUS,
            QDBusServiceWatcher::WatchForRegistration |
            QDBusServiceWatcher::WatchForUnregistration, this);
    connect(watcher, SIGNAL(serviceRegistered(QString)), SLOT(onConnmanRegistered()));
    connect(watcher, SIGNAL(serviceUnregistered(QString)), SLOT(onConnmanUnregistered()));
    setConnmanAvailable(CONNMAN_BUS.interface()->isServiceRegistered(CONNMAN_SERVICE));
}

NetworkManager::~NetworkManager()
{
}

NetworkManager* NetworkManager::instance()
{
    return NetworkManagerFactory::createInstance();
}

void NetworkManager::onConnmanRegistered()
{
    // Store the current connman registration state so that
    // Private::maybeCreateInterfaceProxy() could check it.
    // m_proxy is not a reliable indicator because proxy creation
    // can (and sometimes does) fail.
    m_priv->m_registered = true;
    setConnmanAvailable(true);
}

void NetworkManager::onConnmanUnregistered()
{
    m_priv->m_registered = false;
    setConnmanAvailable(false);
}

void NetworkManager::setConnmanAvailable(bool available)
{
    if (m_available != available) {
        if (available) {
            if (connectToConnman()) {
                Q_EMIT availabilityChanged(m_available = true);
            } else {

                //
                // There is a race condition (at least in Qt 5.6) between
                // this thread and "QDBusConnection" thread updating the
                // service owner map. If we get there first (before the
                // owner map is updated) we would fetch the old (empty)
                // owner from there which makes QDBusAbstractInterface
                // invalid, even though the name is actually owned.
                //
                // The easy workaround is to try again a bit later.
                // This is a very narrow race condition, no delay is
                // necessary.
                //
                m_priv->maybeCreateInterfaceProxyLater();
            }
        } else {
            DBG_("connman not AVAILABLE");
            Q_EMIT availabilityChanged(m_available = false);
            disconnectFromConnman();
        }
    }
}

bool NetworkManager::connectToConnman()
{
    disconnectFromConnman();
    m_proxy = new InterfaceProxy(this);
    if (!m_proxy->isValid()) {
        WARN_(m_proxy->lastError());
        delete m_proxy;
        m_proxy = NULL;
        return false;
    } else {
        connect(m_proxy,
            SIGNAL(PropertyChanged(QString,QDBusVariant)),
            SLOT(propertyChanged(QString,QDBusVariant)));
        connect(new QDBusPendingCallWatcher(m_proxy->GetProperties(), m_proxy),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(getPropertiesFinished(QDBusPendingCallWatcher*)));
        return true;
    }
}

void NetworkManager::disconnectFromConnman()
{
    delete m_proxy;
    m_proxy = NULL;

    disconnectTechnologies();
    disconnectServices();
}

void NetworkManager::disconnectTechnologies()
{
    if (m_proxy) {
        disconnect(m_proxy, SIGNAL(TechnologyAdded(QDBusObjectPath,QVariantMap)),
                   this, SLOT(technologyAdded(QDBusObjectPath,QVariantMap)));
        disconnect(m_proxy, SIGNAL(TechnologyRemoved(QDBusObjectPath)),
                   this, SLOT(technologyRemoved(QDBusObjectPath)));
    }

    Q_FOREACH (NetworkTechnology *tech, m_technologiesCache) {
        tech->deleteLater();
    }

    if (!m_technologiesCache.isEmpty()) {
        m_technologiesCache.clear();
        Q_EMIT technologiesChanged();
    }
}

void NetworkManager::disconnectServices()
{
    if (m_proxy) {
        disconnect(m_proxy, SIGNAL(ServicesChanged(ConnmanObjectList,QList<QDBusObjectPath>)),
                   this, SLOT(updateServices(ConnmanObjectList,QList<QDBusObjectPath>)));
    }

    Q_FOREACH (NetworkService *service, m_servicesCache) {
        service->deleteLater();
    }

    m_servicesCache.clear();

    if (m_defaultRoute != m_invalidDefaultRoute) {
        m_defaultRoute = m_invalidDefaultRoute;
        Q_EMIT defaultRouteChanged(m_defaultRoute);
    }

    if (!m_servicesOrder.isEmpty()) {
        m_servicesOrder.clear();
        Q_EMIT servicesChanged();
        Q_EMIT savedServicesChanged();
    }
}

void NetworkManager::setupTechnologies()
{
    if (m_proxy) {
        connect(m_proxy,
            SIGNAL(TechnologyAdded(QDBusObjectPath,QVariantMap)),
            SLOT(technologyAdded(QDBusObjectPath,QVariantMap)));
        connect(m_proxy,
            SIGNAL(TechnologyRemoved(QDBusObjectPath)),
            SLOT(technologyRemoved(QDBusObjectPath)));
        connect(new QDBusPendingCallWatcher(m_proxy->GetTechnologies(), m_proxy),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(getTechnologiesFinished(QDBusPendingCallWatcher*)));
    }
}

void NetworkManager::setupServices()
{
    if (m_proxy) {
        connect(m_proxy,
            SIGNAL(ServicesChanged(ConnmanObjectList,QList<QDBusObjectPath>)),
            SLOT(updateServices(ConnmanObjectList,QList<QDBusObjectPath>)));
        connect(new QDBusPendingCallWatcher(m_proxy->GetServices(), m_proxy),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(getServicesFinished(QDBusPendingCallWatcher*)));
    }
}

void NetworkManager::updateServices(const ConnmanObjectList &changed, const QList<QDBusObjectPath> &removed)
{
    ConnmanObject connmanobj;
    int order = -1;
    NetworkService *service = NULL;

    m_servicesOrder.clear();
    m_savedServicesOrder.clear();

    Q_FOREACH (connmanobj, changed) {
        order++;
        bool addedService = false;

        const QString svcPath(connmanobj.objpath.path());

        if (!m_servicesCache.contains(svcPath)) {
            service = new NetworkService(svcPath, connmanobj.properties, this);
            connect(service,SIGNAL(connectedChanged(bool)),this,SLOT(updateDefaultRoute()));
            m_servicesCache.insert(svcPath, service);
            addedService = true;
        } else {
            service = m_servicesCache.value(svcPath);
            service->updateProperties(connmanobj.properties);
        }

        if (!m_servicesOrder.contains(svcPath)) {
            m_servicesOrder.append(svcPath);
            if (service->saved()) {
                m_savedServicesOrder.append(svcPath);
            }
        }

        if (order == 0)
            updateDefaultRoute();

        if (addedService) { //Q_EMIT this after m_servicesOrder is updated
            Q_EMIT serviceAdded(svcPath);
        }
    }

    Q_FOREACH (QDBusObjectPath obj, removed) {
        const QString svcPath(obj.path());

        NetworkService *service = m_servicesCache.value(svcPath);
        if (service) {
            service->deleteLater();
            m_servicesCache.remove(svcPath);
            Q_EMIT serviceRemoved(svcPath);
        } else {
            // connman maintains a virtual "hidden" wifi network and removes it upon init
            DBG_("attempted to remove non-existing service" << svcPath);
        }
    }

    if (order == -1)
        updateDefaultRoute();

    Q_EMIT servicesChanged();
    Q_EMIT savedServicesChanged();
    Q_EMIT servicesListChanged(m_servicesOrder);
}

void NetworkManager::propertyChanged(const QString &name, const QDBusVariant &value)
{
    propertyChanged(name, value.variant());
}

void NetworkManager::updateDefaultRoute()
{
    QString defaultNetDev;
    QFile routeFile("/proc/net/route");
    if (routeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&routeFile);
        QString line = in.readLine();
        while (!line.isNull()) {
            QStringList lineList = line.split('\t');
            if (lineList.size() >= 11) {
                if ((lineList.at(1) == "00000000" && lineList.at(3) == "0003") ||
                   (lineList.at(0).startsWith("ppp") && lineList.at(3) == "0001")) {
                    defaultNetDev = lineList.at(0);
                    break;
                }
            }
            line = in.readLine();
        }
        routeFile.close();
    }
    if (defaultNetDev.isNull()) {
         QFile ipv6routeFile("/proc/net/ipv6_route");
         if (ipv6routeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
             QTextStream ipv6in(&ipv6routeFile);
             QString ipv6line = ipv6in.readLine();
             while (!ipv6line.isNull()) {
                 QStringList ipv6lineList = ipv6line.split(QRegExp("\\s+"));
                 if (ipv6lineList.size() >= 10) {
                     if (ipv6lineList.at(0) == "00000000000000000000000000000000" &&
                        (ipv6lineList.at(8).endsWith("3") || (ipv6lineList.at(8).endsWith("1")))) {
                         defaultNetDev = ipv6lineList.at(9).trimmed();
                         break;
                     }
                     ipv6line = ipv6in.readLine();
                 }
             }
             ipv6routeFile.close();
         }
    }

    Q_FOREACH (NetworkService *service, m_servicesCache) {
        if (service->state() == "online" || service->state() == "ready") {
            if (defaultNetDev == service->ethernet().value("Interface")) {
                if (m_defaultRoute != service) {
                    m_defaultRoute = service;
                    Q_EMIT defaultRouteChanged(m_defaultRoute);
                }
                return;
            }
        }
    }
    m_defaultRoute = m_invalidDefaultRoute;
    Q_EMIT defaultRouteChanged(m_defaultRoute);
}

void NetworkManager::technologyAdded(const QDBusObjectPath &technology,
                                     const QVariantMap &properties)
{
    NetworkTechnology *tech = new NetworkTechnology(technology.path(),
                                                    properties, this);

    m_technologiesCache.insert(tech->type(), tech);
    Q_EMIT technologiesChanged();
}

void NetworkManager::technologyRemoved(const QDBusObjectPath &technology)
{
    NetworkTechnology *net;
    // if we weren't storing by type() this loop would be unecessary
    // but since this function will be triggered rarely that's fine
    Q_FOREACH (net, m_technologiesCache) {
        if (net->objPath() == technology.path()) {
            m_technologiesCache.remove(net->type());
            net->deleteLater();
            break;
        }
    }

    Q_EMIT technologiesChanged();
}

void NetworkManager::getPropertiesFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<QVariantMap> reply = *watcher;
    watcher->deleteLater();

    if (reply.isError()) {
        DBG_(reply.error().message());
        return;
    }
    QVariantMap props = reply.value();

    for (QVariantMap::ConstIterator i = props.constBegin(); i != props.constEnd(); ++i)
        propertyChanged(i.key(), i.value());

    if (m_technologiesEnabled)
        setupTechnologies();
    if (m_servicesEnabled)
        setupServices();
}

void NetworkManager::getTechnologiesFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<ConnmanObjectList> reply = *watcher;
    watcher->deleteLater();
    if (reply.isError())
        return;
    Q_FOREACH (const ConnmanObject &object, reply.value()) {
        NetworkTechnology *tech = new NetworkTechnology(object.objpath.path(),
                                                        object.properties, this);
        m_technologiesCache.insert(tech->type(), tech);
    }

    Q_EMIT technologiesChanged();
}

void NetworkManager::getServicesFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<ConnmanObjectList> reply = *watcher;
    watcher->deleteLater();
    if (reply.isError())
        return;
    m_servicesOrder.clear();

    Q_FOREACH (const ConnmanObject &object, reply.value()) {
        const QString servicePath = object.objpath.path();

        NetworkService *service = m_servicesCache.value(servicePath);
        if (service) {
            service->updateProperties(object.properties);
        } else {
            service = new NetworkService(servicePath, object.properties, this);
            connect(service,SIGNAL(connectedChanged(bool)),this,SLOT(updateDefaultRoute()));
            m_servicesCache.insert(servicePath, service);
        }
        if (!m_servicesOrder.contains(servicePath))
            m_servicesOrder.append(servicePath);
    }

    updateDefaultRoute();
    Q_EMIT servicesChanged();
    Q_EMIT servicesListChanged(m_servicesCache.keys());
}

// Public API /////////////

// Getters


bool NetworkManager::isAvailable() const
{
    return m_available;
}


QString NetworkManager::state() const
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
    return m_technologiesCache.value(type);
}

QVector<NetworkTechnology *> NetworkManager::getTechnologies() const
{
    QVector<NetworkTechnology *> techs;

    Q_FOREACH (NetworkTechnology *tech, m_technologiesCache) {
        techs.push_back(tech);
    }

    return techs;
}

QVector<NetworkService*> NetworkManager::selectServices(const QString &tech,
    ServiceSelector selector) const
{
    QVector<NetworkService *> services;

    // this Q_FOREACH is based on the m_servicesOrder to keep connman's sort
    // of services.
    Q_FOREACH (const QString &servicePath, m_servicesOrder) {
        NetworkService *service = m_servicesCache.value(servicePath);
        if (selector(service) && (tech.isEmpty() || service->type() == tech)) {
            services.push_back(service);
        }
    }

    return services;
}

QVector<NetworkService*> NetworkManager::getServices(const QString &tech) const
{
    return selectServices(tech, Private::selectAll);
}

QVector<NetworkService*> NetworkManager::getSavedServices(const QString &tech) const
{
    return selectServices(tech, Private::selectSaved);
}

QVector<NetworkService*> NetworkManager::getAvailableServices(const QString &tech) const
{
    return selectServices(tech, Private::selectAvailable);
}

void NetworkManager::removeSavedService(const QString &) const
{
    // Not used anymore
}

// Setters

void NetworkManager::setOfflineMode(bool offline)
{
    if (m_proxy) {
        m_proxy->SetProperty(OfflineMode, offline);
    }
}

  // these shouldn't crash even if connman isn't available
void NetworkManager::registerAgent(const QString &path)
{
    if (m_proxy) {
        m_proxy->RegisterAgent(path);
    }
}

void NetworkManager::unregisterAgent(const QString &path)
{
    if (m_proxy) {
        m_proxy->UnregisterAgent(path);
    }
}

void NetworkManager::registerCounter(const QString &path, quint32 accuracy, quint32 period)
{
    if (m_proxy) {
        m_proxy->RegisterCounter(path, accuracy, period);
    }
}

void NetworkManager::unregisterCounter(const QString &path)
{
    if (m_proxy) {
        m_proxy->UnregisterCounter(path);
    }
}

QDBusObjectPath NetworkManager::createSession(const QVariantMap &settings, const QString &path)
{
    if (m_proxy) {
        return m_proxy->CreateSession(settings, path).value();
    } else {
        return QDBusObjectPath();
    }
}

void NetworkManager::destroySession(const QString &path)
{
    if (m_proxy) {
        m_proxy->DestroySession(path);
    }
}

bool NetworkManager::createService(
        const QVariantMap &settings, const QString &tech, const QString &service, const QString &device)
{
    if (m_proxy) {
        // The public type is QVariantMap for QML's benefit, covert to a string map now.
        StringPairArray settingsStrings;
        for (QVariantMap::const_iterator it = settings.begin(); it != settings.end(); ++it) {
            settingsStrings.append(qMakePair(it.key(), it.value().toString()));
        }

        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                    m_proxy->CreateService(tech, service, device, settingsStrings), this);

        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            watcher->deleteLater();

            QDBusReply<QDBusObjectPath> reply = *watcher;

            if (!reply.isValid()) {
                qWarning() << "NetworkManager: Failed to create service." << reply.error().name() << reply.error().message();
                emit serviceCreationFailed(reply.error().name());
            } else {
                emit serviceCreated(reply.value().path());
            }
        });
        return true;
    } else {
        return false;
    }
}

QString NetworkManager::createServiceSync(
        const QVariantMap &settings, const QString &tech, const QString &service, const QString &device)
{
    if (m_proxy) {
        // The public type is QVariantMap for QML's benefit, covert to a string map now.
        StringPairArray settingsStrings;
        for (QVariantMap::const_iterator it = settings.begin(); it != settings.end(); ++it) {
            settingsStrings.append(qMakePair(it.key(), it.value().toString()));
        }
        QDBusPendingReply<QDBusObjectPath> reply = m_proxy->CreateService(tech, service, device, settingsStrings);
        reply.waitForFinished();

        if (reply.isError()) {
            qWarning() << "NetworkManager: Failed to create service." << reply.error().name() << reply.error().message();
        }

        return reply.value().path();
    } else {
        return QString();
    }
}

void NetworkManager::setSessionMode(bool sessionMode)
{
    if (m_proxy) {
        m_proxy->SetProperty(SessionMode, sessionMode);
    }
}

void NetworkManager::propertyChanged(const QString &name, const QVariant &value)
{
    if (m_propertiesCache.value(name) == value)
        return;

    m_propertiesCache[name] = value;

    if (name == State) {
        Q_EMIT stateChanged(value.toString());
        updateDefaultRoute();
    } else if (name == OfflineMode) {
        Q_EMIT offlineModeChanged(value.toBool());
    } else if (name == SessionMode) {
        Q_EMIT sessionModeChanged(value.toBool());
    } else if (name == Private::InputRequestTimeout) {
        Q_EMIT inputRequestTimeoutChanged();
    }
}

bool NetworkManager::sessionMode() const
{
    return m_propertiesCache.value(SessionMode).toBool();
}

uint NetworkManager::inputRequestTimeout() const
{
    bool ok = false;
    uint value = m_propertiesCache.value(Private::InputRequestTimeout).toUInt(&ok);
    return (ok && value) ? value : Private::DefaultInputRequestTimeout;
}

bool NetworkManager::servicesEnabled() const
{
    return m_servicesEnabled;
}

void NetworkManager::setServicesEnabled(bool enabled)
{
    if (m_servicesEnabled == enabled)
        return;

    if (this == staticInstance) {
        qWarning() << "Refusing to modify the shared instance";
        return;
    }

    m_servicesEnabled = enabled;

    if (m_servicesEnabled)
        setupServices();
    else
        disconnectServices();

    Q_EMIT servicesEnabledChanged();
}

bool NetworkManager::technologiesEnabled() const
{
    return m_technologiesEnabled;
}

void NetworkManager::setTechnologiesEnabled(bool enabled)
{
    if (m_technologiesEnabled == enabled)
        return;

    if (this == staticInstance) {
        qWarning() << "Refusing to modify the shared instance";
        return;
    }

    m_technologiesEnabled = enabled;

    if (m_technologiesEnabled)
        setupTechnologies();
    else
        disconnectTechnologies();

    Q_EMIT technologiesEnabledChanged();
}

void NetworkManager::resetCountersForType(const QString &type)
{
    if (m_proxy) {
        m_proxy->ResetCounters(type);
    }
}

QStringList NetworkManager::selectServiceList(const QString &tech, ServiceSelector selector) const
{
    QStringList services;
    if (!m_servicesOrder.isEmpty()) {
        Q_FOREACH (const QString &servicePath, m_servicesOrder) {
            NetworkService *service = m_servicesCache.value(servicePath);
            if (selector(service) && (tech.isEmpty() || service->type() == tech)) {
                services.push_back(servicePath);
            }
        }
    } else {
        QHash<QString, NetworkService *>::const_iterator i = m_servicesCache.constBegin();
        while (i != m_servicesCache.constEnd()) {
            NetworkService *service = i.value();
            if (selector(service) && (tech.isEmpty() || service->type() == tech)) {
                services.push_back(i.key());
            }
            ++i;
        }
    }
    return services;
}

QStringList NetworkManager::servicesList(const QString &tech)
{
    return selectServiceList(tech, Private::selectAll);
}

QStringList NetworkManager::savedServicesList(const QString &tech)
{
    return servicesList(tech);
}

QStringList NetworkManager::availableServices(const QString &tech)
{
    return selectServiceList(tech, Private::selectAvailable);
}

QString NetworkManager::technologyPathForService(const QString &servicePath)
{
    NetworkService *service = m_servicesCache.value(servicePath);
    if (!service)
        return QString();

    return technologyPathForType(service->type());
}

QString NetworkManager::technologyPathForType(const QString &techType)
{
    Q_FOREACH (NetworkTechnology *tech, m_technologiesCache) {
        if (tech->type() == techType)
            return tech->path();
    }
    return QString();
}

QStringList NetworkManager::technologiesList()
{
    QStringList techList;
    Q_FOREACH (NetworkTechnology *tech, m_technologiesCache) {
        techList << tech->type();
    }
    return techList;
}

QString NetworkManager::wifiTechnologyPath() const
{
    return NetworkManager::Private::WifiTechnology;
}

QString NetworkManager::cellularTechnologyPath() const
{
    return NetworkManager::Private::CellularTechnology;
}

QString NetworkManager::bluetoothTechnologyPath() const
{
    return NetworkManager::Private::BluetoothTechnology;
}

QString NetworkManager::gpsTechnologyPath() const
{
    return NetworkManager::Private::GpsTechnology;
}

#include "networkmanager.moc"
