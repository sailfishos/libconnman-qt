/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2020 Jolla Ltd.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "networkmanager.h"
#include "commondbustypes.h"
#include "libconnman_p.h"

#include <QRegularExpression>
#include <QWeakPointer>

static const QString InputRequestTimeout("InputRequestTimeout");
static const uint DefaultInputRequestTimeout(300000);

static const QString WifiType("wifi");
static const QString CellularType("cellular");
static const QString EthernetType("ethernet");

static const QString State("State");
static const QString OfflineMode("OfflineMode");
static const QString DefaultService("DefaultService");

// ==========================================================================
// NetworkManagerFactory
// ==========================================================================

static NetworkManager* staticInstance = NULL;

static NetworkManager* internalCreateInstance()
{
    qWarning() << "NetworkManagerFactory::createInstance/instance() is deprecated. Use NetworkManager::sharedInstance() instead.";
    if (!staticInstance)
        staticInstance = new NetworkManager;

    return staticInstance;
}

NetworkManager* NetworkManagerFactory::createInstance()
{
    return internalCreateInstance();
}

NetworkManager* NetworkManagerFactory::instance()
{
    return internalCreateInstance();
}

// ==========================================================================
// NetworkManager::Private
// ==========================================================================
class InterfaceProxy;

class NetworkManager::Private : public QObject
{
    Q_OBJECT

public:
    class ListUpdate;

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

    InterfaceProxy *m_proxy;

    /* Contains all property related to this net.connman.Manager object */
    QVariantMap m_propertiesCache;

    /* Not just for cache, but actual containers of Network* type objects */
    QHash<QString, NetworkTechnology *> m_technologiesCache;
    QHash<QString, NetworkService *> m_servicesCache;
    bool m_servicesCacheHasUpdates;

    /* Define the order of services returned in service lists */
    QStringList m_servicesOrder;
    QStringList m_savedServicesOrder;

    /* This variable is used just to send signal if changed */
    NetworkService* m_defaultRoute;

    /* Invalid default route service for use when there is no default route */
    NetworkService *m_invalidDefaultRoute;

    /* Since VPNs are not known this holds info necessary for dropping out of VPN */
    bool m_defaultRouteIsVPN;

    bool m_available;

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

public slots:
    void updateServices(const ConnmanObjectList &changed, const QList<QDBusObjectPath> &removed);

public:
    Private(NetworkManager *parent)
        : QObject(parent)
        , m_registered(false)
        , m_servicesAvailable(false)
        , m_technologiesAvailable(false)
        , m_connectingWifi(false)
        , m_connected(false)
        , m_connectedWifi(nullptr)
        , m_connectedEthernet(nullptr)
        , m_proxy(nullptr)
        , m_servicesCacheHasUpdates(false)
        , m_defaultRoute(nullptr)
        , m_invalidDefaultRoute(new NetworkService("/", QVariantMap(), this))
        , m_defaultRouteIsVPN(false)
        , m_available(false)
    {
    }

    NetworkManager* manager()
        { return static_cast<NetworkManager*>(parent()); }
    void maybeCreateInterfaceProxyLater()
        { QMetaObject::invokeMethod(this, "maybeCreateInterfaceProxy"); }

public Q_SLOTS:
    void maybeCreateInterfaceProxy();
    void onConnectedChanged();
    void onWifiConnectingChanged();
};

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
        if (!m_available) {
            NetworkManager* mgr = manager();
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
    if (service->type() == WifiType && updateWifiConnected(service)) {
        Q_EMIT manager()->connectedWifiChanged();
    } else if (service->type() == EthernetType && updateEthernetConnected(service)) {
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

    m_propertiesCache[State] = newState;

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

void NetworkManager::Private::updateServices(const ConnmanObjectList &changed, const QList<QDBusObjectPath> &removed)
{
    ListUpdate services(&m_servicesOrder);
    ListUpdate savedServices(&m_savedServicesOrder);
    ListUpdate availableServices(&m_availableServicesOrder);
    ListUpdate wifiServices(&m_wifiServicesOrder);
    ListUpdate cellularServices(&m_cellularServicesOrder);
    ListUpdate ethernetServices(&m_ethernetServicesOrder);

    QStringList addedServices;
    QStringList removedServices;
    NetworkService* prevConnectedWifi = m_connectedWifi;
    NetworkService* prevConnectedEthernet = m_connectedEthernet;

    for (const ConnmanObject &obj : changed) {
        const QString path(obj.objpath.path());

        // Ignore all WiFi with a zeroed/unknown BSSIDs to reduce list size
        // in crowded areas. These are most likely weak and really unreachable
        // but ConnMan maintains them if they come within reach and then they
        // have a valid BSSID. WiFi services with an empty BSSID are saved ones
        // that are not in the range.
        if (obj.properties.value("Type").toString() == WifiType) {
            const QString bssid = obj.properties.value("BSSID").toString();

            if (bssid == QStringLiteral("00:00:00:00:00:00"))
                continue;
        }

        NetworkService *service = m_servicesCache.value(path);
        if (service) {
            // We don't want to emit signals at this point. Those will
            // be emitted later, after internal state is fully updated
            disconnect(service, nullptr, this, nullptr);
            service->updateProperties(obj.properties);
        } else {
            service = new NetworkService(path, obj.properties, this);
            m_servicesCache.insert(path, service);
            addedServices.append(path);
        }

        // Full list
        services.add(path);

        // Saved services
        if (selectSaved(service)) {
            savedServices.add(path);
        }

        // Available services
        if (selectAvailable(service)) {
            availableServices.add(path);
        }

        // Per-technology lists
        const QString type(service->type());
        if (type == WifiType) {
            wifiServices.add(path);
            // Some special treatment for WiFi services
            updateWifiConnected(service);
            connect(service, &NetworkService::connectingChanged,
                    this, &NetworkManager::Private::onWifiConnectingChanged);
        } else if (type == CellularType) {
            cellularServices.add(path);
        } else if (type == EthernetType) {
            ethernetServices.add(path);
            updateEthernetConnected(service);
        }

        connect(service, &NetworkService::connectedChanged,
                this, &NetworkManager::Private::onConnectedChanged);
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
        NetworkService *service = m_servicesCache.take(path);
        if (service) {
            if (service == m_connectedWifi) {
                m_connectedWifi = nullptr;
            }
            if (service == m_defaultRoute) {
                m_defaultRoute = m_invalidDefaultRoute;
            }
            service->deleteLater();
            removedServices.append(path);
        } else {
            // connman maintains a virtual "hidden" wifi network and removes it upon init
            qCDebug(lcConnman) << "attempted to remove non-existing service" << path;
        }
    }

    // Make sure that m_servicesCache doesn't contain stale elements.
    // Hopefully that won't happen too often (if at all)
    if (m_servicesCache.count() > m_servicesOrder.count()) {
        QStringList keys = m_servicesCache.keys();
        for (const QString &path: keys) {
            if (!m_servicesOrder.contains(path)) {
                NetworkService *service = m_servicesCache.take(path);
                if (service == m_defaultRoute) {
                    m_defaultRoute = m_invalidDefaultRoute;
                }
                service->deleteLater();
                removedServices.append(path);
                if (m_servicesCache.count() == m_servicesOrder.count()) {
                    break;
                }
            }
        }
    }

    // Update availability and check whether validity changed
    bool wasValid = manager()->isValid();
    setServicesAvailable(true);
    updateWifiConnecting(NULL);

    // Emit signals
    if (m_connectedWifi != prevConnectedWifi) {
        Q_EMIT manager()->connectedWifiChanged();
    }

    if (m_connectedEthernet != prevConnectedEthernet) {
        Q_EMIT manager()->connectedEthernetChanged();
    }

    // Added services
    for (const QString &path: addedServices) {
        Q_EMIT manager()->serviceAdded(path);
    }

    // Removed services
    for (const QString &path: removedServices) {
        Q_EMIT manager()->serviceRemoved(path);
    }

    m_servicesCacheHasUpdates = true;
    manager()->updateDefaultRoute();

    if (services.changed) {
        Q_EMIT manager()->servicesChanged();
        // This one is probably unnecessary:
        Q_EMIT manager()->servicesListChanged(m_servicesOrder);
    }
    if (savedServices.changed) {
        Q_EMIT manager()->savedServicesChanged();
    }
    if (availableServices.changed) {
        Q_EMIT manager()->availableServicesChanged();
    }
    if (wifiServices.changed) {
        Q_EMIT manager()->wifiServicesChanged();
    }
    if (cellularServices.changed) {
        Q_EMIT manager()->cellularServicesChanged();
    }
    if (ethernetServices.changed) {
        Q_EMIT manager()->ethernetServicesChanged();
    }
    if (wasValid != manager()->isValid()) {
        Q_EMIT manager()->validChanged();
    }
}

// ==========================================================================
// InterfaceProxy
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

class InterfaceProxy: public QDBusAbstractInterface
{
    Q_OBJECT

public:
    InterfaceProxy(NetworkManager *parent) :
        QDBusAbstractInterface(CONNMAN_SERVICE, "/", "net.connman.Manager",
            QDBusConnection::systemBus(), parent) {}

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
    QDBusPendingReply<QDBusObjectPath> CreateService(const QString &type, const QString &device,
                                                     const QString &network, const StringPairArray &settings)
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

const QString NetworkManager::WifiTechnologyPath("/net/connman/technology/wifi");
const QString NetworkManager::CellularTechnologyPath("/net/connman/technology/cellular");
const QString NetworkManager::BluetoothTechnologyPath("/net/connman/technology/bluetooth");
const QString NetworkManager::GpsTechnologyPath("/net/connman/technology/gps");
const QString NetworkManager::EthernetTechnologyPath("/net/connman/technology/ethernet");

NetworkManager::NetworkManager(QObject* parent)
  : QObject(parent),
    m_priv(new Private(this))
{
    registerCommonDataTypes();
    QDBusServiceWatcher* watcher = new QDBusServiceWatcher(CONNMAN_SERVICE, QDBusConnection::systemBus(),
            QDBusServiceWatcher::WatchForRegistration |
            QDBusServiceWatcher::WatchForUnregistration, this);
    connect(watcher, SIGNAL(serviceRegistered(QString)), SLOT(onConnmanRegistered()));
    connect(watcher, SIGNAL(serviceUnregistered(QString)), SLOT(onConnmanUnregistered()));
    setConnmanAvailable(QDBusConnection::systemBus().interface()->isServiceRegistered(CONNMAN_SERVICE));
}

NetworkManager::~NetworkManager()
{
}

NetworkManager* NetworkManager::instance()
{
    qWarning() << "NetworkManager::instance() is deprecated. Use sharedInstance() instead.";
    return internalCreateInstance();
}

QSharedPointer<NetworkManager> NetworkManager::sharedInstance()
{
    static QWeakPointer<NetworkManager> sharedManager;

    QSharedPointer<NetworkManager> manager = sharedManager.toStrongRef();

    if (!manager) {
        manager = QSharedPointer<NetworkManager>::create();
        sharedManager = manager;
    }

    return manager;
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
    if (m_priv->m_available != available) {
        if (available) {
            if (connectToConnman()) {
                Q_EMIT availabilityChanged(m_priv->m_available = true);
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
            qCDebug(lcConnman) << "connman not AVAILABLE";
            Q_EMIT availabilityChanged(m_priv->m_available = false);
            disconnectFromConnman();
        }
    }
}

bool NetworkManager::connectToConnman()
{
    disconnectFromConnman();
    m_priv->m_proxy = new InterfaceProxy(this);
    if (!m_priv->m_proxy->isValid()) {
        qWarning() << m_priv->m_proxy->lastError();
        delete m_priv->m_proxy;
        m_priv->m_proxy = nullptr;
        return false;
    } else {
        connect(m_priv->m_proxy, SIGNAL(PropertyChanged(QString,QDBusVariant)),
                SLOT(propertyChanged(QString,QDBusVariant)));

        auto *getProperties = new QDBusPendingCallWatcher(m_priv->m_proxy->GetProperties(), m_priv->m_proxy);
        connect(getProperties, &QDBusPendingCallWatcher::finished,
                this, &NetworkManager::getPropertiesFinished);
        return true;
    }
}

void NetworkManager::disconnectFromConnman()
{
    delete m_priv->m_proxy;
    m_priv->m_proxy = nullptr;

    disconnectTechnologies();
    disconnectServices();
}

void NetworkManager::disconnectTechnologies()
{
    // Update availability and check whether validity changed
    bool wasValid = isValid();
    m_priv->setTechnologiesAvailable(false);

    if (m_priv->m_proxy) {
        disconnect(m_priv->m_proxy, SIGNAL(TechnologyAdded(QDBusObjectPath,QVariantMap)),
                   this, SLOT(technologyAdded(QDBusObjectPath,QVariantMap)));
        disconnect(m_priv->m_proxy, SIGNAL(TechnologyRemoved(QDBusObjectPath)),
                   this, SLOT(technologyRemoved(QDBusObjectPath)));
    }

    for (NetworkTechnology *tech : m_priv->m_technologiesCache) {
        tech->deleteLater();
    }

    if (!m_priv->m_technologiesCache.isEmpty()) {
        m_priv->m_technologiesCache.clear();
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
    if (m_priv->m_defaultRoute != m_priv->m_invalidDefaultRoute) {
        m_priv->m_defaultRoute = m_priv->m_invalidDefaultRoute;
        m_priv->m_defaultRouteIsVPN = false;
        emitDefaultRouteChanged = true;
    }

    bool emitConnectedWifiChanged = false;
    if (m_priv->m_connectedWifi) {
        m_priv->m_connectedWifi = nullptr;
        emitConnectedWifiChanged = true;
    }

    bool emitConnectedEthernetChanged = false;
    if (m_priv->m_connectedEthernet) {
        m_priv->m_connectedEthernet = nullptr;
        emitConnectedEthernetChanged = true;
    }

    if (m_priv->m_proxy) {
        disconnect(m_priv->m_proxy, SIGNAL(ServicesChanged(ConnmanObjectList,QList<QDBusObjectPath>)),
                   m_priv, SLOT(updateServices(ConnmanObjectList,QList<QDBusObjectPath>)));
    }

    for (NetworkService *service : m_priv->m_servicesCache) {
        service->deleteLater();
    }

    m_priv->m_servicesCache.clear();
    m_priv->m_servicesCacheHasUpdates = false;

    // Clear all lists before emitting the signals

    bool emitSavedServicesChanged = false;
    if (!m_priv->m_savedServicesOrder.isEmpty()) {
        m_priv->m_savedServicesOrder.clear();
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

    if (!m_priv->m_servicesOrder.isEmpty()) {
        m_priv->m_servicesOrder.clear();
        Q_EMIT servicesChanged();
    }

    if (emitDefaultRouteChanged) {
        Q_EMIT defaultRouteChanged(m_priv->m_defaultRoute);
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
    if (m_priv->m_proxy) {
        connect(m_priv->m_proxy, SIGNAL(TechnologyAdded(QDBusObjectPath,QVariantMap)),
                SLOT(technologyAdded(QDBusObjectPath,QVariantMap)));
        connect(m_priv->m_proxy, SIGNAL(TechnologyRemoved(QDBusObjectPath)),
                SLOT(technologyRemoved(QDBusObjectPath)));

        QDBusPendingCallWatcher *pendingCall
                = new QDBusPendingCallWatcher(m_priv->m_proxy->GetTechnologies(), m_priv->m_proxy);
        connect(pendingCall, SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(getTechnologiesFinished(QDBusPendingCallWatcher*)));
    }
}

void NetworkManager::setupServices()
{
    if (m_priv->m_proxy) {
        connect(m_priv->m_proxy, SIGNAL(ServicesChanged(ConnmanObjectList,QList<QDBusObjectPath>)),
                m_priv, SLOT(updateServices(ConnmanObjectList,QList<QDBusObjectPath>)));

        QDBusPendingCallWatcher *pendingCall
                = new QDBusPendingCallWatcher(m_priv->m_proxy->GetServices(), m_priv->m_proxy);
        connect(pendingCall, SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(getServicesFinished(QDBusPendingCallWatcher*)));
    }
}


void NetworkManager::propertyChanged(const QString &name, const QDBusVariant &value)
{
    propertyChanged(name, value.variant());
}

NetworkService* NetworkManager::selectDefaultRoute(const QString &path)
{
    NetworkService *newDefaultRoute = nullptr;
    bool isVPN = path.indexOf("vpn_") != -1;

    if (!m_priv->m_servicesCacheHasUpdates)
        return nullptr;

    // Avoid repetitive calls with this
    m_priv->m_servicesCacheHasUpdates = false;

    // Check if it is VPN -> old is the transport as this does not understand
    // or has not been designed VPNs in mind. So the default service from
    // networkmanager point of view is still the transport.
    // TODO implement a VPN support here as well
    if (isVPN) {
        m_priv->m_defaultRouteIsVPN = true;
        if (m_priv->m_defaultRoute && m_priv->m_defaultRoute != m_priv->m_invalidDefaultRoute) {
            qDebug() << "New default service is VPN, use old service " << m_priv->m_defaultRoute->name();
            return m_priv->m_defaultRoute;
        } else {
            qDebug() << "No old default service set, select next connected";
        }
    } else {
       if (m_priv->m_servicesOrder.contains(path)) {
            newDefaultRoute = m_priv->m_servicesCache.value(path, nullptr);
            if (newDefaultRoute && newDefaultRoute->connected()) {
                qDebug() << "Selected service" << newDefaultRoute->name() << "path" << path;
                return newDefaultRoute;
            } else {
                qDebug() << "Service" << (newDefaultRoute ? newDefaultRoute->name() : "NULL") << "not connected";
            }
        }
    }

    // No default route was set = VPN was connected before check use the
    // ordered list to get the next non-VPN service that is used as
    // transport. When VPN is set as the default service the transport is
    // always the next non-VPN service in the list. The order is defined
    // by the technology type when states are equal ethernet > wlan >
    // cellular.
    int i = 0;
    for (const QString &servicePath : m_priv->m_servicesOrder) {
        i++;

        newDefaultRoute = m_priv->m_servicesCache.value(servicePath, nullptr);
        /* No service for path, try next */
        if (!newDefaultRoute)
            continue;

        if (newDefaultRoute->connected()) {
            if (servicePath.indexOf("vpn_") != -1) {
                /* First connected service is VPN -> VPN is default route */
                if (i == 1) {
                    m_priv->m_defaultRouteIsVPN = true;
                    qDebug() << "VPN is set as default route";
                }

                continue;
            }

            qDebug() << "Selected service" << newDefaultRoute->name() << "path" << servicePath;
            return newDefaultRoute;
        } else {
            /* Stop when first not-connected is reached as the services are ordered based on state */
            break;
        }
    }

    qDebug() << "No transport service found";

    return nullptr;
}

void NetworkManager::updateDefaultRoute()
{
    if (!m_priv->m_defaultRoute || m_priv->m_defaultRoute == m_priv->m_invalidDefaultRoute) {
        if (!m_priv->m_servicesCacheHasUpdates)
            return;

        qDebug() << "No default route set, services:" << m_priv->m_servicesCache.count();

        m_priv->m_defaultRoute = selectDefaultRoute(QString());
        if (!m_priv->m_defaultRoute)
            m_priv->m_defaultRoute = m_priv->m_invalidDefaultRoute;
    }

    m_priv->m_servicesCacheHasUpdates = false;

    Q_EMIT defaultRouteChanged(m_priv->m_defaultRoute);
}

void NetworkManager::technologyAdded(const QDBusObjectPath &technology,
                                     const QVariantMap &properties)
{
    NetworkTechnology *tech = new NetworkTechnology(technology.path(), properties, this);

    m_priv->m_technologiesCache.insert(tech->type(), tech);
    Q_EMIT technologiesChanged();
}

void NetworkManager::technologyRemoved(const QDBusObjectPath &technology)
{
    // if we weren't storing by type() this loop would be unecessary
    // but since this function will be triggered rarely that's fine
    for (NetworkTechnology *net : m_priv->m_technologiesCache) {
        if (net->path() == technology.path()) {
            m_priv->m_technologiesCache.remove(net->type());
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
        qCDebug(lcConnman) << reply.error().message();
        return;
    }
    QVariantMap props = reply.value();

    for (QVariantMap::ConstIterator i = props.constBegin(); i != props.constEnd(); ++i)
        propertyChanged(i.key(), i.value());

    setupTechnologies();
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
        m_priv->m_technologiesCache.insert(tech->type(), tech);
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
        qWarning() << reply.error();
    } else {
        services = reply.value();
    }
    qDebug() << "Updating services as GetServices returns";
    m_priv->updateServices(services, QList<QDBusObjectPath>());
}

// Public API /////////////

// Getters


bool NetworkManager::isAvailable() const
{
    return m_priv->m_available;
}


QString NetworkManager::state() const
{
    return m_priv->m_propertiesCache[State].toString();
}

bool NetworkManager::offlineMode() const
{
    return m_priv->m_propertiesCache[OfflineMode].toBool();
}

NetworkService* NetworkManager::defaultRoute() const
{
    return m_priv->m_defaultRoute;
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
    return m_priv->m_technologiesCache.value(type);
}

QVector<NetworkTechnology *> NetworkManager::getTechnologies() const
{
    QVector<NetworkTechnology *> techs;

    for (NetworkTechnology *tech : m_priv->m_technologiesCache) {
        techs.push_back(tech);
    }

    return techs;
}

QVector<NetworkService*> NetworkManager::selectServices(const QStringList &list,
    ServiceSelector selector) const
{
    QVector<NetworkService *> services;
    for (const QString &path : list) {
        NetworkService *service = m_priv->m_servicesCache.value(path);
        if (selector(service)) {
            services.append(m_priv->m_servicesCache.value(path));
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
            services.append(m_priv->m_servicesCache.value(path));
        }
    } else {
        for (const QString &path : list) {
            NetworkService *service = m_priv->m_servicesCache.value(path);
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
        if (selector(m_priv->m_servicesCache.value(path))) {
            services.append(path);
        }
    }
    return services;
}

QStringList NetworkManager::selectServiceList(const QStringList &list, const QString &tech) const
{
    if (tech.isEmpty()) {
        return list;
    } else {
        QStringList services;
        for (const QString &path : list) {
            NetworkService *service = m_priv->m_servicesCache.value(path);
            if (service->type() == tech) {
                services.append(path);
            }
        }
        return services;
    }
}

QVector<NetworkService*> NetworkManager::getServices(const QString &tech) const
{
    if (tech == WifiType) {
        return selectServices(m_priv->m_wifiServicesOrder, QString());
    } else if (tech == CellularType) {
        return selectServices(m_priv->m_cellularServicesOrder, QString());
    } else if (tech == EthernetType) {
        return selectServices(m_priv->m_ethernetServicesOrder, QString());
    } else {
        return selectServices(m_priv->m_servicesOrder, tech);
    }
}

QVector<NetworkService*> NetworkManager::getSavedServices(const QString &tech) const
{
    // Choose smaller list to scan
    if (tech == WifiType) {
        if (m_priv->m_wifiServicesOrder.count() < m_priv->m_savedServicesOrder.count()) {
            return selectServices(m_priv->m_wifiServicesOrder, Private::selectSaved);
        }
    } else if (tech == CellularType) {
        if (m_priv->m_cellularServicesOrder.count() < m_priv->m_savedServicesOrder.count()) {
            return selectServices(m_priv->m_cellularServicesOrder, Private::selectSaved);
        }
    } else if (tech == EthernetType) {
        return selectServices(m_priv->m_ethernetServicesOrder, Private::selectSavedOrAvailable);
    }
    return selectServices(m_priv->m_savedServicesOrder, tech);
}

QVector<NetworkService*> NetworkManager::getAvailableServices(const QString &tech) const
{
    // Choose smaller list to scan
    if (tech == WifiType) {
        if (m_priv->m_wifiServicesOrder.count() < m_priv->m_availableServicesOrder.count()) {
            return selectServices(m_priv->m_wifiServicesOrder, Private::selectAvailable);
        }
    } else if (tech == CellularType) {
        if (m_priv->m_cellularServicesOrder.count() < m_priv->m_availableServicesOrder.count()) {
            return selectServices(m_priv->m_cellularServicesOrder, Private::selectAvailable);
        }
    } else if (tech == EthernetType) {
        return selectServices(m_priv->m_ethernetServicesOrder, Private::selectAvailable);
    }
    return selectServices(m_priv->m_availableServicesOrder, tech);
}

QStringList NetworkManager::servicesList(const QString &tech)
{
    if (tech == WifiType) {
        return m_priv->m_wifiServicesOrder;
    } else if (tech == CellularType) {
        return m_priv->m_cellularServicesOrder;
    } else if (tech == EthernetType) {
        return m_priv->m_ethernetServicesOrder;
    } else {
        return selectServiceList(m_priv->m_servicesOrder, tech);
    }
}

QStringList NetworkManager::savedServicesList(const QString &tech)
{
    // Choose smaller list to scan
    if (tech == WifiType) {
        if (m_priv->m_wifiServicesOrder.count() < m_priv->m_savedServicesOrder.count()) {
            return selectServiceList(m_priv->m_wifiServicesOrder, Private::selectSaved);
        }
    } else if (tech == CellularType) {
        if (m_priv->m_cellularServicesOrder.count() < m_priv->m_savedServicesOrder.count()) {
            return selectServiceList(m_priv->m_cellularServicesOrder, Private::selectSaved);
        }
    } else if (tech == EthernetType) {
        if (m_priv->m_ethernetServicesOrder.count() < m_priv->m_savedServicesOrder.count()) {
            return selectServiceList(m_priv->m_ethernetServicesOrder, Private::selectSaved);
        }
    }
    return selectServiceList(m_priv->m_savedServicesOrder, tech);
}

QStringList NetworkManager::availableServices(const QString &tech)
{
    // Choose smaller list to scan
    if (tech == WifiType) {
        if (m_priv->m_wifiServicesOrder.count() < m_priv->m_availableServicesOrder.count()) {
            return selectServiceList(m_priv->m_wifiServicesOrder, Private::selectAvailable);
        }
    } else if (tech == CellularType) {
        if (m_priv->m_cellularServicesOrder.count() < m_priv->m_availableServicesOrder.count()) {
            return selectServiceList(m_priv->m_cellularServicesOrder, Private::selectAvailable);
        }
    } else if (tech == EthernetType) {
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
    if (m_priv->m_proxy) {
        m_priv->m_proxy->SetProperty(OfflineMode, offline);
    }
}

  // these shouldn't crash even if connman isn't available
void NetworkManager::registerAgent(const QString &path)
{
    if (m_priv->m_proxy) {
        m_priv->m_proxy->RegisterAgent(path);
    }
}

void NetworkManager::unregisterAgent(const QString &path)
{
    if (m_priv->m_proxy) {
        m_priv->m_proxy->UnregisterAgent(path);
    }
}

void NetworkManager::registerCounter(const QString &path, quint32 accuracy, quint32 period)
{
    if (m_priv->m_proxy) {
        m_priv->m_proxy->RegisterCounter(path, accuracy, period);
    }
}

void NetworkManager::unregisterCounter(const QString &path)
{
    if (m_priv->m_proxy) {
        m_priv->m_proxy->UnregisterCounter(path);
    }
}

QDBusObjectPath NetworkManager::createSession(const QVariantMap &settings, const QString &path)
{
    if (m_priv->m_proxy) {
        return m_priv->m_proxy->CreateSession(settings, path).value();
    } else {
        return QDBusObjectPath();
    }
}

void NetworkManager::destroySession(const QString &path)
{
    if (m_priv->m_proxy) {
        m_priv->m_proxy->DestroySession(path);
    }
}

bool NetworkManager::createService(
        const QVariantMap &settings, const QString &tech, const QString &service, const QString &device)
{
    if (m_priv->m_proxy) {
        // The public type is QVariantMap for QML's benefit, covert to a string map now.
        StringPairArray settingsStrings;
        for (QVariantMap::const_iterator it = settings.begin(); it != settings.end(); ++it) {
            settingsStrings.append(qMakePair(it.key(), it.value().toString()));
        }

        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(
                    m_priv->m_proxy->CreateService(tech, device, service, settingsStrings), this);

        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *watcher) {
            watcher->deleteLater();

            QDBusReply<QDBusObjectPath> reply = *watcher;

            if (!reply.isValid()) {
                qWarning() << "NetworkManager: Failed to create service." << reply.error().name()
                           << reply.error().message();
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
    if (m_priv->m_proxy) {
        // The public type is QVariantMap for QML's benefit, covert to a string map now.
        StringPairArray settingsStrings;
        for (QVariantMap::const_iterator it = settings.begin(); it != settings.end(); ++it) {
            settingsStrings.append(qMakePair(it.key(), it.value().toString()));
        }
        QDBusPendingReply<QDBusObjectPath> reply = m_priv->m_proxy->CreateService(tech, device, service, settingsStrings);
        reply.waitForFinished();

        if (reply.isError()) {
            qWarning() << "NetworkManager: Failed to create service." << reply.error().name()
                       << reply.error().message();
        }

        return reply.value().path();
    } else {
        return QString();
    }
}

void NetworkManager::setSessionMode(bool)
{
    static bool warned = false;
    if (!warned) {
        qWarning() << "NetworkManager::setSessionMode() is deprecated, this call will be ignored";
        warned = true;
    }
}

void NetworkManager::propertyChanged(const QString &name, const QVariant &value)
{
    if (name == State) {
        m_priv->updateState(value.toString());
    } else if (name == DefaultService) {
        NetworkService* newDefaultRoute(NULL);
        QString path = value.toString();

        /* No change in default route */
        if (m_priv->m_defaultRoute && m_priv->m_defaultRoute->path() == path)
            return;

        m_priv->m_servicesCacheHasUpdates = true;

        qDebug() << "Default service changed to path \'" << path << "\'";

        newDefaultRoute = selectDefaultRoute(path);

        if (m_priv->m_defaultRoute != newDefaultRoute || m_priv->m_defaultRouteIsVPN) {
            qDebug() << "Updating default route";
            m_priv->m_defaultRoute = newDefaultRoute;

            /* Unset only when default is not VPN */
            if (path.indexOf("vpn_") == -1)
                m_priv->m_defaultRouteIsVPN = false;

            updateDefaultRoute();
        }
    } else {
        if (m_priv->m_propertiesCache.value(name) == value)
            return;

        m_priv->m_propertiesCache[name] = value;

        if (name == OfflineMode) {
            Q_EMIT offlineModeChanged(value.toBool());
        } else if (name == InputRequestTimeout) {
            Q_EMIT inputRequestTimeoutChanged();
        }
    }
}

bool NetworkManager::sessionMode() const
{
    static bool warned = false;
    if (!warned) {
        qWarning() << "NetworkManager::sessionMode() is deprecated, this will return hard-coded false";
        warned = true;
    }
    return false;
}

uint NetworkManager::inputRequestTimeout() const
{
    bool ok = false;
    uint value = m_priv->m_propertiesCache.value(InputRequestTimeout).toUInt(&ok);
    return (ok && value) ? value : DefaultInputRequestTimeout;
}

bool NetworkManager::servicesEnabled() const
{
    return true;
}

void NetworkManager::setServicesEnabled(bool)
{
    static bool warned = false;
    if (!warned) {
        qWarning() << "NetworkManager::setServicesEnabled() is deprecated, this call will be ignored";
        warned = true;
    }
}

bool NetworkManager::technologiesEnabled() const
{
    return true;
}

void NetworkManager::setTechnologiesEnabled(bool)
{
    static bool warned = false;
    if (!warned) {
        qWarning() << "NetworkManager::setTechnologiesEnabled() is deprecated, this call will be ignored";
        warned = true;
    }
}

bool NetworkManager::isValid() const
{
    return m_priv->m_servicesAvailable && m_priv->m_technologiesAvailable;
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

void NetworkManager::resetCountersForType(const QString &type)
{
    if (m_priv->m_proxy) {
        m_priv->m_proxy->ResetCounters(type);
    }
}

QString NetworkManager::technologyPathForService(const QString &servicePath)
{
    NetworkService *service = m_priv->m_servicesCache.value(servicePath, nullptr);
    if (!service)
        return QString();

    return technologyPathForType(service->type());
}

QString NetworkManager::technologyPathForType(const QString &techType)
{
    QHash<QString, NetworkTechnology *>::const_iterator i = m_priv->m_technologiesCache.constFind(techType);

    if (i != m_priv->m_technologiesCache.constEnd()) {
        return i.value()->path();
    }
    return QString();
}

QStringList NetworkManager::technologiesList()
{
    QStringList techList;
    for (NetworkTechnology *tech : m_priv->m_technologiesCache) {
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
