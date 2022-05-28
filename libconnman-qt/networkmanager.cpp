/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2020 Jolla Ltd.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "networkmanager.h"

#include "libconnman_p.h"
#include <QRegularExpression>

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

class NetworkManager::Private : public QObject
{
    Q_OBJECT

public:
    class ListUpdate;

    static const QString InputRequestTimeout;
    static const uint DefaultInputRequestTimeout;

    static const QString WifiType;
    static const QString CellularType;
    static const QString EthernetType;

    bool m_registered;
    bool m_servicesAvailable;
    bool m_technologiesAvailable;

    // For now, we are only tracking connecting state for WiFi service
    bool m_connectingWifi;
    bool m_connected;

    QStringList m_availableServicesOrder;
    QStringList m_wifiServicesOrder;
    QStringList m_cellularServicesOrder;
    QStringList m_ethernetServicesOrder;
    NetworkService* m_connectedWifi;
    NetworkService* m_connectedEthernet;

public:
    static bool selectSaved(NetworkService *service);
    static bool selectAvailable(NetworkService *service);
    static bool selectSavedOrAvailable(NetworkService *service);
    bool updateWifiConnected(NetworkService *service);
    bool updateEthernetConnected(NetworkService *service);
    bool updateWifiConnecting(NetworkService *service);
    void setServicesAvailable(bool servicesAvailable);
    void setTechnologiesAvailable(bool technologiesAvailable);

    void updateState(const QString &newState);

public:
    Private(NetworkManager *parent)
        : QObject(parent)
        , m_registered(false)
        , m_servicesAvailable(false)
        , m_technologiesAvailable(false)
        , m_connectingWifi(false)
        , m_connected(false)
        , m_connectedWifi(NULL)
        , m_connectedEthernet(NULL) {}
    NetworkManager* manager()
        { return (NetworkManager*)parent(); }
    void maybeCreateInterfaceProxyLater()
        { QMetaObject::invokeMethod(this, "maybeCreateInterfaceProxy"); }

public Q_SLOTS:
    void maybeCreateInterfaceProxy();
    void onConnectedChanged();
    void onWifiConnectingChanged();
};

const QString NetworkManager::Private::InputRequestTimeout("InputRequestTimeout");
const uint NetworkManager::Private::DefaultInputRequestTimeout(300000);

const QString NetworkManager::Private::WifiType("wifi");
const QString NetworkManager::Private::CellularType("cellular");
const QString NetworkManager::Private::EthernetType("ethernet");

bool NetworkManager::Private::selectSaved(NetworkService *service)
{
    return service && service->saved();
}

bool NetworkManager::Private::selectAvailable(NetworkService *service)
{
    return service && service->available();
}

bool NetworkManager::Private::selectSavedOrAvailable(NetworkService *service)
{
    return service && (service->saved() || service->available());
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

bool NetworkManager::Private::updateWifiConnected(NetworkService *service)
{
    if (service->connected()) {
        if (!m_connectedWifi) {
            m_connectedWifi = service;
            return true;
        }
    } else if (m_connectedWifi == service) {
        QVector<NetworkService*> availableWifi = manager()->getAvailableServices(WifiType);
        m_connectedWifi = NULL;
        for (NetworkService *wifi: availableWifi) {
            if (wifi->connected()) {
                m_connectedWifi = wifi;
                break;
            }
        }
        return true;
    }
    return false;
}


bool NetworkManager::Private::updateEthernetConnected(NetworkService *service)
{
    if (service->connected()) {
        if (!m_connectedEthernet) {
            m_connectedEthernet = service;
            return true;
        }
    } else if (m_connectedEthernet == service) {
        QVector<NetworkService*> availableEthernet = manager()->getAvailableServices(EthernetType);
        m_connectedEthernet = NULL;
        for (NetworkService *ethernet: availableEthernet) {
            if (ethernet->connected()) {
                m_connectedEthernet = ethernet;
                break;
            }
        }
        return true;
    }
    return false;
}

void NetworkManager::Private::onConnectedChanged()
{
    manager()->updateDefaultRoute();
    NetworkService *service = qobject_cast<NetworkService*>(sender());
    if (!service) {
        return;
    }
    if (service->type() == Private::WifiType && updateWifiConnected(service)) {
        Q_EMIT manager()->connectedWifiChanged();
    } else if (service->type() == Private::EthernetType && updateEthernetConnected(service)) {
        Q_EMIT manager()->connectedEthernetChanged();
    }
}

bool NetworkManager::Private::updateWifiConnecting(NetworkService *service)
{
    if (service && service->connecting()) {
        // Definitely connecting
        if (!m_connectingWifi) {
            m_connectingWifi = true;
            return true;
        }
    } else {
        // Need to check
        const QVector<NetworkService*> availableWifi = manager()->getAvailableServices(WifiType);
        for (NetworkService *wifi: availableWifi) {
            if (wifi->connecting()) {
                if (m_connectingWifi) {
                    // Already connecting
                    return false;
                } else {
                    // Connecting state changed
                    m_connectingWifi = true;
                    return true;
                }
            }
        }
        // Nothing is connecting
        if (m_connectingWifi) {
            m_connectingWifi = false;
            return true;
        }
    }
    return false;
}

void NetworkManager::Private::onWifiConnectingChanged()
{
    NetworkService *service = qobject_cast<NetworkService*>(sender());
    if (service && updateWifiConnecting(service)) {
        Q_EMIT manager()->connectingChanged();
        Q_EMIT manager()->connectingWifiChanged();
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
        { return asyncCall("SetProperty", name, QVariant::fromValue(QDBusVariant(value))); }
    QDBusPendingCall GetTechnologies()
        { return asyncCall("GetTechnologies"); }
    QDBusPendingCall GetServices()
        { return asyncCall("GetServices"); }
    QDBusPendingCall RegisterAgent(const QString &path)
        { return asyncCall("RegisterAgent", QVariant::fromValue(QDBusObjectPath(path))); }
    QDBusPendingCall UnregisterAgent(const QString &path)
        { return asyncCall("UnregisterAgent", QVariant::fromValue(QDBusObjectPath(path))); }
    QDBusPendingCall RegisterCounter(const QString &path, uint accuracy, uint period)
        { return asyncCall("RegisterCounter", QVariant::fromValue(QDBusObjectPath(path)), accuracy, period); }
    QDBusPendingCall ResetCounters(const QString &type)
        { return asyncCall("ResetCounters", type); }
    QDBusPendingCall UnregisterCounter(const QString &path)
        { return asyncCall("UnregisterCounter", QVariant::fromValue(QDBusObjectPath(path))); }
    QDBusPendingReply<QDBusObjectPath> CreateSession(const QVariantMap &settings, const QString &path)
        { return asyncCall("CreateSession", settings, QVariant::fromValue(QDBusObjectPath(path))); }
    QDBusPendingCall DestroySession(const QString &path)
        { return asyncCall("DestroySession", QVariant::fromValue(QDBusObjectPath(path))); }
    QDBusPendingReply<QDBusObjectPath> CreateService(const QString &type, const QString &device, const QString &network, const StringPairArray &settings)
        { return asyncCall("CreateService", type, device, network, QVariant::fromValue(settings)); }

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

const QString NetworkManager::WifiTechnologyPath("/net/connman/technology/wifi");
const QString NetworkManager::CellularTechnologyPath("/net/connman/technology/cellular");
const QString NetworkManager::BluetoothTechnologyPath("/net/connman/technology/bluetooth");
const QString NetworkManager::GpsTechnologyPath("/net/connman/technology/gps");
const QString NetworkManager::EthernetTechnologyPath("/net/connman/technology/ethernet");

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
    // Update availability and check whether validity changed
    bool wasValid = isValid();
    m_priv->setTechnologiesAvailable(false);

    if (m_proxy) {
        disconnect(m_proxy, SIGNAL(TechnologyAdded(QDBusObjectPath,QVariantMap)),
                   this, SLOT(technologyAdded(QDBusObjectPath,QVariantMap)));
        disconnect(m_proxy, SIGNAL(TechnologyRemoved(QDBusObjectPath)),
                   this, SLOT(technologyRemoved(QDBusObjectPath)));
    }

    for (NetworkTechnology *tech : m_technologiesCache) {
        tech->deleteLater();
    }

    if (!m_technologiesCache.isEmpty()) {
        m_technologiesCache.clear();
        Q_EMIT technologiesChanged();
    }

    if (wasValid != isValid()) {
        Q_EMIT validChanged();
    }
}

void NetworkManager::disconnectServices()
{
    // Update availability and check whether validity changed
    bool wasValid = isValid();
    m_priv->setServicesAvailable(false);

    bool emitDefaultRouteChanged = false;
    if (m_defaultRoute != m_invalidDefaultRoute) {
        m_defaultRoute = m_invalidDefaultRoute;
        emitDefaultRouteChanged = true;
    }

    bool emitConnectedWifiChanged = false;
    if (m_priv->m_connectedWifi) {
        m_priv->m_connectedWifi = NULL;
        emitConnectedWifiChanged = true;
    }

    bool emitConnectedEthernetChanged = false;
    if (m_priv->m_connectedEthernet) {
        m_priv->m_connectedEthernet = NULL;
        emitConnectedEthernetChanged = true;
    }

    if (m_proxy) {
        disconnect(m_proxy, SIGNAL(ServicesChanged(ConnmanObjectList,QList<QDBusObjectPath>)),
                   this, SLOT(updateServices(ConnmanObjectList,QList<QDBusObjectPath>)));
    }

    for (NetworkService *service : m_servicesCache) {
        service->deleteLater();
    }

    m_servicesCache.clear();

    // Clear all lists before emitting the signals

    bool emitSavedServicesChanged = false;
    if (!m_savedServicesOrder.isEmpty()) {
        m_savedServicesOrder.clear();
        emitSavedServicesChanged = true;
    }

    bool emitAvailableServicesChanged = false;
    if (!m_priv->m_availableServicesOrder.isEmpty()) {
        m_priv->m_availableServicesOrder.clear();
        emitAvailableServicesChanged = true;
    }

    bool emitWifiServicesChanged = false;
    if (!m_priv->m_wifiServicesOrder.isEmpty()) {
        m_priv->m_wifiServicesOrder.clear();
        emitWifiServicesChanged = true;
    }

    bool emitCellularServicesChanged = false;
    if (!m_priv->m_cellularServicesOrder.isEmpty()) {
        m_priv->m_cellularServicesOrder.clear();
        emitCellularServicesChanged = true;
    }

    bool emitEthernetServicesChanged = false;
    if (!m_priv->m_ethernetServicesOrder.isEmpty()) {
        m_priv->m_ethernetServicesOrder.clear();
        emitEthernetServicesChanged = true;
    }

    if (!m_servicesOrder.isEmpty()) {
        m_servicesOrder.clear();
        Q_EMIT servicesChanged();
    }

    if (emitDefaultRouteChanged) {
        Q_EMIT defaultRouteChanged(m_defaultRoute);
    }

    if (emitConnectedWifiChanged) {
        Q_EMIT connectedWifiChanged();
    }

    if (emitConnectedEthernetChanged) {
        Q_EMIT connectedEthernetChanged();
    }

    if (emitSavedServicesChanged) {
        Q_EMIT savedServicesChanged();
    }

    if (emitSavedServicesChanged) {
        Q_EMIT savedServicesChanged();
    }

    if (emitAvailableServicesChanged) {
        Q_EMIT availableServicesChanged();
    }

    if (emitWifiServicesChanged) {
        Q_EMIT wifiServicesChanged();
    }

    if (emitCellularServicesChanged) {
        Q_EMIT cellularServicesChanged();
    }

    if (emitEthernetServicesChanged) {
        Q_EMIT ethernetServicesChanged();
    }

    if (wasValid != isValid()) {
        Q_EMIT validChanged();
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

class NetworkManager::Private::ListUpdate {
public:
    ListUpdate(QStringList* list) : storage(list), changed(false), count(0) {}

    void add(const QString& str) {
        if (storage->count() == count) {
            storage->append(str);
            changed = true;
        } else if (storage->at(count) != str) {
            while (storage->count() > count) {
                storage->removeLast();
            }
            storage->append(str);
            changed = true;
        }
        count++;
    }

    void done() {
        while (storage->count() > count) {
            storage->removeLast();
            changed = true;
        }
    }

public:
    QStringList* storage;
    bool changed;
    int count;
};

void NetworkManager::updateServices(const ConnmanObjectList &changed, const QList<QDBusObjectPath> &removed)
{
    Private::ListUpdate services(&m_servicesOrder);
    Private::ListUpdate savedServices(&m_savedServicesOrder);
    Private::ListUpdate availableServices(&m_priv->m_availableServicesOrder);
    Private::ListUpdate wifiServices(&m_priv->m_wifiServicesOrder);
    Private::ListUpdate cellularServices(&m_priv->m_cellularServicesOrder);
    Private::ListUpdate ethernetServices(&m_priv->m_ethernetServicesOrder);

    QStringList addedServices;
    QStringList removedServices;
    NetworkService* prevConnectedWifi = m_priv->m_connectedWifi;
    NetworkService* prevConnectedEthernet = m_priv->m_connectedEthernet;

    for (const ConnmanObject &obj : changed) {
        const QString path(obj.objpath.path());

        // Ignore all WiFi with a zeroed/unknown BSSIDs to reduce list size
        // in crowded areas. These are most likely weak and really unreachable
        // but ConnMan maintains them if they come within reach and then they
        // have a valid BSSID. WiFi services with an empty BSSID are saved ones
        // that are not in the range.
        if (obj.properties.value("Type").toString() == NetworkManager::Private::WifiType) {
            const QString bssid = obj.properties.value("BSSID").toString();

            if (bssid == QStringLiteral("00:00:00:00:00:00"))
                continue;
        }

        NetworkService *service = m_servicesCache.value(path);
        if (service) {
            // We don't want to emit signals at this point. Those will
            // be emitted later, after internal state is fully updated
            disconnect(service, nullptr, m_priv, nullptr);
            service->updateProperties(obj.properties);
        } else {
            service = new NetworkService(path, obj.properties, this);
            m_servicesCache.insert(path, service);
            addedServices.append(path);
        }

        // Full list
        services.add(path);

        // Saved services
        if (Private::selectSaved(service)) {
            savedServices.add(path);
        }

        // Available services
        if (Private::selectAvailable(service)) {
            availableServices.add(path);
        }

        // Per-technology lists
        const QString type(service->type());
        if (type == Private::WifiType) {
            wifiServices.add(path);
            // Some special treatment for WiFi services
            m_priv->updateWifiConnected(service);
            connect(service, &NetworkService::connectingChanged,
                    m_priv, &NetworkManager::Private::onWifiConnectingChanged);
        } else if (type == Private::CellularType) {
            cellularServices.add(path);
        } else if (type == Private::EthernetType) {
            ethernetServices.add(path);
            m_priv->updateEthernetConnected(service);
        }

        connect(service, &NetworkService::connectedChanged,
                m_priv, &NetworkManager::Private::onConnectedChanged);
    }

    // Cut the tails
    services.done();
    savedServices.done();
    availableServices.done();
    wifiServices.done();
    cellularServices.done();
    ethernetServices.done();

    // Removed services
    for (const QDBusObjectPath &obj : removed) {
        const QString path(obj.path());
        NetworkService *service = m_servicesCache.value(path);
        if (service) {
            if (service == m_priv->m_connectedWifi) {
                m_priv->m_connectedWifi = NULL;
            }
            service->deleteLater();
            m_servicesCache.remove(path);
            removedServices.append(path);
        } else {
            // connman maintains a virtual "hidden" wifi network and removes it upon init
            DBG_("attempted to remove non-existing service" << path);
        }
    }

    // Make sure that m_servicesCache doesn't contain stale elements.
    // Hopefully that won't happen too often (if at all)
    if (m_servicesCache.count() > m_servicesOrder.count()) {
        QStringList keys = m_servicesCache.keys();
        for (const QString &path: keys) {
            if (!m_servicesOrder.contains(path)) {
                m_servicesCache.value(path)->deleteLater();
                m_servicesCache.remove(path);
                removedServices.append(path);
                if (m_servicesCache.count() == m_servicesOrder.count()) {
                    break;
                }
            }
        }
    }

    // Update availability and check whether validity changed
    bool wasValid = isValid();
    m_priv->setServicesAvailable(true);
    m_priv->updateWifiConnecting(NULL);

    // Emit signals
    if (m_priv->m_connectedWifi != prevConnectedWifi) {
        Q_EMIT connectedWifiChanged();
    }

    if (m_priv->m_connectedEthernet != prevConnectedEthernet) {
        Q_EMIT connectedEthernetChanged();
    }

    // Added services
    for (const QString &path: addedServices) {
        Q_EMIT serviceAdded(path);
    }

    // Removed services
    for (const QString &path: removedServices) {
        Q_EMIT serviceRemoved(path);
    }

    updateDefaultRoute();

    if (services.changed) {
        Q_EMIT servicesChanged();
        // This one is probably unnecessary:
        Q_EMIT servicesListChanged(m_servicesOrder);
    }
    if (savedServices.changed) {
        Q_EMIT savedServicesChanged();
    }
    if (availableServices.changed) {
        Q_EMIT availableServicesChanged();
    }
    if (wifiServices.changed) {
        Q_EMIT wifiServicesChanged();
    }
    if (cellularServices.changed) {
        Q_EMIT cellularServicesChanged();
    }
    if (ethernetServices.changed) {
        Q_EMIT ethernetServicesChanged();
    }
    if (wasValid != isValid()) {
        Q_EMIT validChanged();
    }
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
                 QStringList ipv6lineList = ipv6line.split(QRegularExpression("\\s+"));
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

    for (NetworkService *service : m_servicesCache) {
        if (service->connected()) {
            if (defaultNetDev == service->ethernet().value("Interface")) {
                if (m_defaultRoute != service) {
                    m_defaultRoute = service;
                    Q_EMIT defaultRouteChanged(m_defaultRoute);
                }
                return;
            }
        }
    }

    if (m_defaultRoute != m_invalidDefaultRoute) {
        m_defaultRoute = m_invalidDefaultRoute;
        Q_EMIT defaultRouteChanged(m_defaultRoute);
    }
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
    // if we weren't storing by type() this loop would be unecessary
    // but since this function will be triggered rarely that's fine
    for (NetworkTechnology *net : m_technologiesCache) {
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
    for (const ConnmanObject &object : reply.value()) {
        NetworkTechnology *tech = new NetworkTechnology(object.objpath.path(),
                                                        object.properties, this);
        m_technologiesCache.insert(tech->type(), tech);
    }

    // Update availability and check whether validity changed
    bool wasValid = isValid();
    m_priv->setTechnologiesAvailable(true);

    Q_EMIT technologiesChanged();

    if (wasValid != isValid()) {
        Q_EMIT validChanged();
    }
}

void NetworkManager::getServicesFinished(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<ConnmanObjectList> reply = *watcher;
    ConnmanObjectList services;
    watcher->deleteLater();
    if (reply.isError()) {
        WARN_(reply.error());
    } else {
        services = reply.value();
    }
    updateServices(services, QList<QDBusObjectPath>());
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

NetworkService* NetworkManager::connectedWifi() const
{
    return m_priv->m_connectedWifi;
}

NetworkService *NetworkManager::connectedEthernet() const
{
    return m_priv->m_connectedEthernet;
}

NetworkTechnology* NetworkManager::getTechnology(const QString &type) const
{
    return m_technologiesCache.value(type);
}

QVector<NetworkTechnology *> NetworkManager::getTechnologies() const
{
    QVector<NetworkTechnology *> techs;

    for (NetworkTechnology *tech : m_technologiesCache) {
        techs.push_back(tech);
    }

    return techs;
}

QVector<NetworkService*> NetworkManager::selectServices(const QStringList &list,
    ServiceSelector selector) const
{
    QVector<NetworkService *> services;
    for (const QString &path : list) {
        NetworkService *service = m_servicesCache.value(path);
        if (selector(service)) {
            services.append(m_servicesCache.value(path));
        }
    }
    return services;
}

QVector<NetworkService*> NetworkManager::selectServices(const QStringList &list,
    const QString &tech) const
{
    QVector<NetworkService*> services;
    if (tech.isEmpty()) {
        for (const QString &path : list) {
            services.append(m_servicesCache.value(path));
        }
    } else {
        for (const QString &path : list) {
            NetworkService *service = m_servicesCache.value(path);
            if (service->type() == tech) {
                services.append(service);
            }
        }
    }
    return services;
}

QStringList NetworkManager::selectServiceList(const QStringList &list,
    ServiceSelector selector) const
{
    QStringList services;
    for (const QString &path : list) {
        if (selector(m_servicesCache.value(path))) {
            services.append(path);
        }
    }
    return services;
}

QStringList NetworkManager::selectServiceList(const QStringList &list,
    const QString &tech) const
{
    if (tech.isEmpty()) {
        return list;
    } else {
        QStringList services;
        for (const QString &path : list) {
            NetworkService *service = m_servicesCache.value(path);
            if (service->type() == tech) {
                services.append(path);
            }
        }
        return services;
    }
}

QVector<NetworkService*> NetworkManager::getServices(const QString &tech) const
{
    if (tech == Private::WifiType) {
        return selectServices(m_priv->m_wifiServicesOrder, QString());
    } else if (tech == Private::CellularType) {
        return selectServices(m_priv->m_cellularServicesOrder, QString());
    } else if (tech == Private::EthernetType) {
        return selectServices(m_priv->m_ethernetServicesOrder, QString());
    } else {
        return selectServices(m_servicesOrder, tech);
    }
}

QVector<NetworkService*> NetworkManager::getSavedServices(const QString &tech) const
{
    // Choose smaller list to scan
    if (tech == Private::WifiType) {
        if (m_priv->m_wifiServicesOrder.count() < m_savedServicesOrder.count()) {
            return selectServices(m_priv->m_wifiServicesOrder, Private::selectSaved);
        }
    } else if (tech == Private::CellularType) {
        if (m_priv->m_cellularServicesOrder.count() < m_savedServicesOrder.count()) {
            return selectServices(m_priv->m_cellularServicesOrder, Private::selectSaved);
        }
    } else if (tech == Private::EthernetType) {
        return selectServices(m_priv->m_ethernetServicesOrder, Private::selectSavedOrAvailable);
    }
    return selectServices(m_savedServicesOrder, tech);
}

QVector<NetworkService*> NetworkManager::getAvailableServices(const QString &tech) const
{
    // Choose smaller list to scan
    if (tech == Private::WifiType) {
        if (m_priv->m_wifiServicesOrder.count() < m_priv->m_availableServicesOrder.count()) {
            return selectServices(m_priv->m_wifiServicesOrder, Private::selectAvailable);
        }
    } else if (tech == Private::CellularType) {
        if (m_priv->m_cellularServicesOrder.count() < m_priv->m_availableServicesOrder.count()) {
            return selectServices(m_priv->m_cellularServicesOrder, Private::selectAvailable);
        }
    } else if (tech == Private::EthernetType) {
        return selectServices(m_priv->m_ethernetServicesOrder, Private::selectAvailable);
    }
    return selectServices(m_priv->m_availableServicesOrder, tech);
}

QStringList NetworkManager::servicesList(const QString &tech)
{
    if (tech == Private::WifiType) {
        return m_priv->m_wifiServicesOrder;
    } else if (tech == Private::CellularType) {
        return m_priv->m_cellularServicesOrder;
    } else if (tech == Private::EthernetType) {
        return m_priv->m_ethernetServicesOrder;
    } else {
        return selectServiceList(m_servicesOrder, tech);
    }
}

QStringList NetworkManager::savedServicesList(const QString &tech)
{
    // Choose smaller list to scan
    if (tech == Private::WifiType) {
        if (m_priv->m_wifiServicesOrder.count() < m_savedServicesOrder.count()) {
            return selectServiceList(m_priv->m_wifiServicesOrder, Private::selectSaved);
        }
    } else if (tech == Private::CellularType) {
        if (m_priv->m_cellularServicesOrder.count() < m_savedServicesOrder.count()) {
            return selectServiceList(m_priv->m_cellularServicesOrder, Private::selectSaved);
        }
    } else if (tech == Private::EthernetType) {
        if (m_priv->m_ethernetServicesOrder.count() < m_savedServicesOrder.count()) {
            return selectServiceList(m_priv->m_ethernetServicesOrder, Private::selectSaved);
        }
    }
    return selectServiceList(m_savedServicesOrder, tech);
}

QStringList NetworkManager::availableServices(const QString &tech)
{
    // Choose smaller list to scan
    if (tech == Private::WifiType) {
        if (m_priv->m_wifiServicesOrder.count() < m_priv->m_availableServicesOrder.count()) {
            return selectServiceList(m_priv->m_wifiServicesOrder, Private::selectAvailable);
        }
    } else if (tech == Private::CellularType) {
        if (m_priv->m_cellularServicesOrder.count() < m_priv->m_availableServicesOrder.count()) {
            return selectServiceList(m_priv->m_cellularServicesOrder, Private::selectAvailable);
        }
    } else if (tech == Private::EthernetType) {
        if (m_priv->m_ethernetServicesOrder.count() < m_priv->m_availableServicesOrder.count()) {
            return selectServiceList(m_priv->m_ethernetServicesOrder, Private::selectAvailable);
        }
    }
    return selectServiceList(m_priv->m_availableServicesOrder, tech);
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
                    m_proxy->CreateService(tech, device, service, settingsStrings), this);

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
        QDBusPendingReply<QDBusObjectPath> reply = m_proxy->CreateService(tech, device, service, settingsStrings);
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
    if (name == State) {
        m_priv->updateState(value.toString());
    } else {
        if (m_propertiesCache.value(name) == value)
            return;

        m_propertiesCache[name] = value;

        if (name == OfflineMode) {
            Q_EMIT offlineModeChanged(value.toBool());
        } else if (name == SessionMode) {
            Q_EMIT sessionModeChanged(value.toBool());
        } else if (name == Private::InputRequestTimeout) {
            Q_EMIT inputRequestTimeoutChanged();
        }
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

    bool wasValid = isValid();

    m_servicesEnabled = enabled;

    if (m_servicesEnabled)
        setupServices();
    else
        disconnectServices();

    Q_EMIT servicesEnabledChanged();

    if (wasValid != isValid()) {
        Q_EMIT validChanged();
    }
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

    bool wasValid = isValid();

    m_technologiesEnabled = enabled;

    if (m_technologiesEnabled)
        setupTechnologies();
    else
        disconnectTechnologies();

    Q_EMIT technologiesEnabledChanged();

    if (wasValid != isValid()) {
        Q_EMIT validChanged();
    }
}

bool NetworkManager::isValid() const
{
    return (!m_servicesEnabled || m_priv->m_servicesAvailable)
            && (!m_technologiesEnabled || m_priv->m_technologiesAvailable);
}

bool NetworkManager::connected() const
{
    return m_priv->m_connected;
}

bool NetworkManager::connecting() const
{
    // For now, we are only tracking connecting state for WiFi service
    return m_priv->m_connectingWifi;
}

bool NetworkManager::connectingWifi() const
{
    return m_priv->m_connectingWifi;
}

void NetworkManager::Private::setServicesAvailable(bool servicesAvailable)
{
    m_servicesAvailable = servicesAvailable;
}

void NetworkManager::Private::setTechnologiesAvailable(bool technologiesAvailable)
{
    m_technologiesAvailable = technologiesAvailable;
}

void NetworkManager::Private::updateState(const QString &newState)
{
    if (manager()->state() == newState)
        return;

    manager()->m_propertiesCache[State] = newState;

    const bool value = ConnmanState::connected(newState);
    bool connectedChanged;
    if (m_connected != value) {
        m_connected = value;
        connectedChanged = true;
    } else {
        connectedChanged = false;
    }

    Q_EMIT manager()->stateChanged(newState);
    if (connectedChanged) {
        Q_EMIT manager()->connectedChanged();
    }

    manager()->updateDefaultRoute();
}

void NetworkManager::resetCountersForType(const QString &type)
{
    if (m_proxy) {
        m_proxy->ResetCounters(type);
    }
}

QString NetworkManager::technologyPathForService(const QString &servicePath)
{
    NetworkService *service = m_servicesCache.value(servicePath, nullptr);
    if (!service)
        return QString();

    return technologyPathForType(service->type());
}

QString NetworkManager::technologyPathForType(const QString &techType)
{
    QHash<QString, NetworkTechnology *>::const_iterator i = m_technologiesCache.constFind(techType);

    if (i != m_technologiesCache.constEnd()) {
        return i.value()->path();
    }
    return QString();
}

QStringList NetworkManager::technologiesList()
{
    QStringList techList;
    for (NetworkTechnology *tech : m_technologiesCache) {
        techList << tech->type();
    }
    return techList;
}

QString NetworkManager::wifiTechnologyPath() const
{
    return WifiTechnologyPath;
}

QString NetworkManager::ethernetTechnologyPath() const
{
    return EthernetTechnologyPath;
}

QString NetworkManager::cellularTechnologyPath() const
{
    return CellularTechnologyPath;
}

QString NetworkManager::bluetoothTechnologyPath() const
{
    return BluetoothTechnologyPath;
}

QString NetworkManager::gpsTechnologyPath() const
{
    return GpsTechnologyPath;
}

#include "networkmanager.moc"
