/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2017 Jolla Ltd.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0. The full text of the Apache License
 * is at http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QSettings>

#include "networkservice.h"
#include "libconnman_p.h"

#include "connman_manager_interface.h"

class NetworkService::Private: public QObject
{
    Q_OBJECT

public:
    class InterfaceProxy;
    class GetPropertyWatcher;
    typedef QHash<QString,EapMethod> EapMethodMap;
    typedef QSharedPointer<EapMethodMap> EapMethodMapRef;

    static const int NUM_EAP_METHODS = 4; // None, PEAP, TTLS, TLS
    static const QString EapMethodName[NUM_EAP_METHODS];

    static const int NUM_SECURITY_TYPES = 5; // Unknown, None, WEP, PSK, IEEE802.1x
    static const QString SecurityTypeName[NUM_SECURITY_TYPES];

    static const QString EAP;
    static const QString Identity;
    static const QString Passphrase;

    static QVariantMap adaptToConnmanProperties(const QVariantMap& map);

    Private(NetworkService* parent);

    void deleteProxy();
    InterfaceProxy* createProxy(QString path);
    NetworkService* service();
    EapMethodMapRef eapMethodMap();
    EapMethod eapMethod();
    bool updateSecurityType();
    void setEapMethod(EapMethod method);
    void setProperty(QString name, QVariant value);

private Q_SLOTS:
    void onRestrictedPropertyChanged(QString name);
    void onGetPropertyFinished(QDBusPendingCallWatcher* call);

public:
    InterfaceProxy* m_proxy;
    EapMethodMapRef m_eapMethodMapRef;
    SecurityType m_securityType;
    bool m_eapMethodAvailable;
    bool m_identityAvailable;
    bool m_passphraseAvailable;
};

// ==========================================================================
// NetworkService::Private::InterfaceProxy
//
// qdbusxml2cpp doesn't really do much, it's easier to write these proxies
// by hand. Basically, this is what we have here:
//
// <interface name="net.connman.Service">
//   <method name="GetProperties">
//     <arg name="properties" type="a{sv}" direction="out"/>
//   </method>
//   <method name="GetProperty">
//     <arg name="value" type="v" direction="out"/>
//   </method>
//   <method name="SetProperty">
//     <arg name="name" type="s"/>
//     <arg name="value" type="v" direction="out"/>
//   </method>
//   <method name="ClearProperty">
//     <arg name="name" type="s" />
//   </method>
//   <method name="Connect"/>
//   <method name="Disconnect"/>
//   <method name="Remove"/>
//   <method name="MoveBefore">
//     <arg name="service" type="o"/>
//   </method>
//   <method name="MoveAfter">
//     <arg name="service" type="o"/>
//   </method>
//   <method name="ResetCounters"/>
//   <signal name="PropertyChanged">
//     <arg name="name" type="s"/>
//     <arg name="value" type="v"/>
//   </signal>
//   <signal name="RestrictedPropertyChanged">
//     <arg name="name" type="s"/>
//   </signal>
// </interface>
//
// ==========================================================================

class NetworkService::Private::InterfaceProxy: public QDBusAbstractInterface
{
    Q_OBJECT

public:
    InterfaceProxy(QString path, NetworkService::Private* parent) :
        QDBusAbstractInterface(CONNMAN_SERVICE, path, "net.connman.Service",
            CONNMAN_BUS, parent) {}

public Q_SLOTS:
    QDBusPendingCall GetProperties()
        { return asyncCall("GetProperties"); }
    QDBusPendingCall GetProperty(QString name)
        { return asyncCall("GetProperty", name); }
    QDBusPendingCall SetProperty(QString name, QVariant value)
        { return asyncCall("SetProperty", name, qVariantFromValue(QDBusVariant(value))); }
    QDBusPendingCall ClearProperty(QString name)
        { return asyncCall("ClearProperty", name); }
    QDBusPendingCall Connect()
        { return asyncCall("Connect"); }
    QDBusPendingCall Disconnect()
        { return asyncCall("Disconnect"); }
    QDBusPendingCall Remove()
        { return asyncCall("Remove"); }
    QDBusPendingCall ResetCounters()
        { return asyncCall("ResetCounters"); }

Q_SIGNALS:
    void PropertyChanged(QString name, QDBusVariant value);
    void RestrictedPropertyChanged(QString name);
};

// ==========================================================================
// NetworkService::Private::GetPropertyWatcher
// ==========================================================================

class NetworkService::Private::GetPropertyWatcher : public QDBusPendingCallWatcher {
public:
    GetPropertyWatcher(QString name, InterfaceProxy* proxy) :
        QDBusPendingCallWatcher(proxy->GetProperty(name), proxy),
        m_name(name) {}
    QString m_name;
};

// ==========================================================================
// NetworkService::Private
// ==========================================================================

const QString NetworkService::Private::EAP("EAP");
const QString NetworkService::Private::Identity("Identity");
const QString NetworkService::Private::Passphrase("Passphrase");

// The order must match EapMethod enum
const QString NetworkService::Private::EapMethodName[] = {
    QString(), "peap", "ttls", "tls"
};

// The order must match SecurityType enum
const QString NetworkService::Private::SecurityTypeName[] = {
    "", "none", "wep", "psk", "ieee8021x"
};

NetworkService::Private::Private(NetworkService* parent) :
    QObject(parent),
    m_proxy(NULL),
    m_securityType(SecurityNone),
    m_eapMethodAvailable(false),
    m_identityAvailable(false),
    m_passphraseAvailable(false)
{
}

void NetworkService::Private::deleteProxy()
{
    if (m_proxy) {
        delete m_proxy;
        m_proxy = NULL;
    }
}

NetworkService::Private::InterfaceProxy*
NetworkService::Private::createProxy(QString path)
{
    delete m_proxy;
    m_proxy = new InterfaceProxy(path, this);
    connect(m_proxy, SIGNAL(RestrictedPropertyChanged(QString)),
        SLOT(onRestrictedPropertyChanged(QString)));
    return m_proxy;
}

inline NetworkService* NetworkService::Private::service()
{
    return (NetworkService*)parent();
}

void NetworkService::Private::onRestrictedPropertyChanged(QString name)
{
    DBG_(name);
    connect(new GetPropertyWatcher(name, m_proxy),
        SIGNAL(finished(QDBusPendingCallWatcher*)),
        SLOT(onGetPropertyFinished(QDBusPendingCallWatcher*)));
}

void NetworkService::Private::onGetPropertyFinished(QDBusPendingCallWatcher* call)
{
    GetPropertyWatcher* watcher = (GetPropertyWatcher*)call;
    QDBusPendingReply<QVariant> reply = *call;
    call->deleteLater();
    if (reply.isError()) {
        DBG_(watcher->m_name << reply.error());
    } else {
        DBG_(watcher->m_name << "=" << reply.value());
        service()->emitPropertyChange(watcher->m_name, reply.value());
    }
}

bool NetworkService::Private::updateSecurityType()
{
    SecurityType type = SecurityUnknown;
    const QStringList security = service()->security();
    if (!security.isEmpty()) {
        // Start with 1 because 0 is SecurityUnknown
        for (int i=1; i<NUM_SECURITY_TYPES; i++) {
            if (security.contains(SecurityTypeName[i])) {
                type = (SecurityType)i;
                break;
            }
        }
    }
    if (m_securityType != type) {
        m_securityType = type;
        return true;
    } else {
        return false;
    }
}

NetworkService::Private::EapMethodMapRef NetworkService::Private::eapMethodMap()
{
    static QWeakPointer<EapMethodMap> sharedInstance;
    m_eapMethodMapRef = sharedInstance;
    if (m_eapMethodMapRef.isNull()) {
        EapMethodMap* map = new EapMethodMap;
        // Start with 1 because 0 is EapNone
        for (int i=1; i<NUM_EAP_METHODS; i++) {
            const QString name = EapMethodName[i];
            map->insert(name.toLower(), (EapMethod)i);
            map->insert(name.toUpper(), (EapMethod)i);
        }
        m_eapMethodMapRef = EapMethodMapRef(map);
    }
    return m_eapMethodMapRef;
}

NetworkService::EapMethod NetworkService::Private::eapMethod()
{
    QString eap = service()->m_propertiesCache.value(EAP).toString();
    if (eap.isEmpty()) {
        return EapNone;
    } else {
        return eapMethodMap()->value(eap, EapNone);
    }
}

void NetworkService::Private::setEapMethod(EapMethod method)
{
    if (method >= EapNone && method < NUM_EAP_METHODS) {
        setProperty(EAP, EapMethodName[method]);
    }
}

void NetworkService::Private::setProperty(QString name, QVariant value)
{
    if (m_proxy) {
        m_proxy->SetProperty(name, value);
    }
}

/*
 * JS returns arrays as QVariantList or a(v) in terms of D-Bus,
 * but ConnMan requires some properties to be lists of strings
 * or a(s) thus this function.
 */
QVariantMap NetworkService::Private::adaptToConnmanProperties(const QVariantMap &map)
{
    QVariantMap buffer;
    Q_FOREACH (const QString &key, map.keys()) {
        if (map.value(key).type() == QVariant::List) {
            QStringList strList;
            Q_FOREACH (const QVariant &value, map.value(key).toList()) {
                strList.append(value.toString());
            }
            buffer.insert(key, strList);
        } else {
            buffer.insert(key, map.value(key));
        }
    }
    return buffer;
}

// ==========================================================================
// NetworkService
// ==========================================================================

const QString NetworkService::Name("Name");
const QString NetworkService::State("State");
const QString NetworkService::Type("Type");
const QString NetworkService::Security("Security");
const QString NetworkService::Strength("Strength");
const QString NetworkService::Error("Error");
const QString NetworkService::Favorite("Favorite");
const QString NetworkService::AutoConnect("AutoConnect");
const QString NetworkService::IPv4("IPv4");
const QString NetworkService::IPv4Config("IPv4.Configuration");
const QString NetworkService::IPv6("IPv6");
const QString NetworkService::IPv6Config("IPv6.Configuration");
const QString NetworkService::Nameservers("Nameservers");
const QString NetworkService::NameserversConfig("Nameservers.Configuration");
const QString NetworkService::Domains("Domains");
const QString NetworkService::DomainsConfig("Domains.Configuration");
const QString NetworkService::Proxy("Proxy");
const QString NetworkService::ProxyConfig("Proxy.Configuration");
const QString NetworkService::Ethernet("Ethernet");
const QString NetworkService::Roaming("Roaming");
//const QString NetworkService::Immutable("Immutable");
//const QString NetworkService::Privacy("Privacy");
const QString NetworkService::Timeservers("Timeservers");
const QString NetworkService::TimeserversConfig("Timeservers.Configuration");
//const QString NetworkService::Provider("Provider");

const QString NetworkService::BSSID("BSSID");
const QString NetworkService::MaxRate("MaxRate");
const QString NetworkService::Frequency("Frequency");
const QString NetworkService::EncryptionMode("EncryptionMode");
const QString NetworkService::Hidden("Hidden");

NetworkService::NetworkService(const QString &path, const QVariantMap &properties, QObject* parent)
  : QObject(parent),
    m_priv(new Private(this)),
    m_path(path),
    m_propertiesCache(properties),
    m_connected(false)
{
    qRegisterMetaType<NetworkService *>();

    m_priv->updateSecurityType();
    m_priv->m_eapMethodAvailable = m_propertiesCache.contains(Private::EAP);
    m_priv->m_identityAvailable = m_propertiesCache.contains(Private::Identity);
    m_priv->m_passphraseAvailable = m_propertiesCache.contains(Private::Passphrase);

    reconnectServiceInterface();
}

NetworkService::NetworkService(QObject* parent)
    : QObject(parent),
      m_priv(new Private(this)),
      m_connected(false)
{
    qRegisterMetaType<NetworkService *>();
}

NetworkService::~NetworkService() {}

const QString NetworkService::name() const
{
    return m_propertiesCache.value(Name).toString();
}

const QString NetworkService::state() const
{
    return m_propertiesCache.value(State).toString();
}

const QString NetworkService::error() const
{
    return m_propertiesCache.value(Error).toString();
}

const QString NetworkService::type() const
{
    return m_propertiesCache.value(Type).toString();
}

const QStringList NetworkService::security() const
{
    return m_propertiesCache.value(Security).toStringList();
}

uint NetworkService::strength() const
{
    return m_propertiesCache.value(Strength).toUInt();
}

bool NetworkService::favorite() const
{
    return m_propertiesCache.value(Favorite).toBool();
}

bool NetworkService::autoConnect() const
{
    return m_propertiesCache.value(AutoConnect).toBool();
}

const QString NetworkService::path() const
{
    return m_path;
}

const QVariantMap NetworkService::ipv4() const
{
    if (m_propertiesCache.contains(IPv4)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv4));
    }
    return QVariantMap();
}

const QVariantMap NetworkService::ipv4Config() const
{
    if (m_propertiesCache.contains(IPv4Config)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv4Config));
    }
    return QVariantMap();
}

const QVariantMap NetworkService::ipv6() const
{
    if (m_propertiesCache.contains(IPv6)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv6));
    }
    return QVariantMap();
}

const QVariantMap NetworkService::ipv6Config() const
{
    if (m_propertiesCache.contains(IPv6Config)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv6Config));
    }
    return QVariantMap();
}

const QStringList NetworkService::nameservers() const
{
    return m_propertiesCache.value(Nameservers).toStringList();
}

const QStringList NetworkService::nameserversConfig() const
{
    return m_propertiesCache.value(NameserversConfig).toStringList();
}

const QStringList NetworkService::domains() const
{
    return m_propertiesCache.value(Domains).toStringList();
}

const QStringList NetworkService::domainsConfig() const
{
    return m_propertiesCache.value(DomainsConfig).toStringList();
}

const QVariantMap NetworkService::proxy() const
{
    if (m_propertiesCache.contains(Proxy)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(Proxy));
    }
    return QVariantMap();
}

const QVariantMap NetworkService::proxyConfig() const
{
    if (m_propertiesCache.contains(ProxyConfig)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(ProxyConfig));
    }
    return QVariantMap();
}

const QVariantMap NetworkService::ethernet() const
{
    if (m_propertiesCache.contains(Ethernet)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(Ethernet));
    }
    return QVariantMap();
}

bool NetworkService::roaming() const
{
    return m_propertiesCache.value(Roaming).toBool();
}

bool NetworkService::hidden() const
{
    return m_propertiesCache.value(Hidden).toBool();
}

void NetworkService::requestConnect()
{
    Private::InterfaceProxy* service = m_priv->m_proxy;

    if (!service) {
        return;
    }
    if (connected()) {
        Q_EMIT connectRequestFailed("Already connected");
        return;
    }

    Q_EMIT serviceConnectionStarted();

    // If the service is in the failure state clear the Error property so that we get notified of
    // errors on subsequent connection attempts.
    if (state() == QLatin1String("failure"))
        service->ClearProperty(QLatin1String("Error"));

    // This is wrong, it should be somehow fetched over D-Bus
    QSettings connmanConfig("/etc/connman/main.conf", QSettings::IniFormat);
    int configTimeout = connmanConfig.value("InputRequestTimeout").toInt() * 1000;
    if (configTimeout == 0)
        configTimeout = CONNECT_TIMEOUT;

    // increase reply timeout when connecting
    int timeout = CONNECT_TIMEOUT_FAVORITE;
    int old_timeout = service->timeout();
    timeout = configTimeout;

    service->setTimeout(timeout);
    QDBusPendingCall conn_reply = m_priv->m_proxy->Connect();
    service->setTimeout(old_timeout);

    connect(new QDBusPendingCallWatcher(conn_reply, service),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(handleConnectReply(QDBusPendingCallWatcher*)));
}

void NetworkService::requestDisconnect()
{
    Private::InterfaceProxy* service = m_priv->m_proxy;
    if (service) {
        Q_EMIT serviceDisconnectionStarted();
        service->Disconnect();
    }
}

void NetworkService::remove()
{
    Private::InterfaceProxy* service = m_priv->m_proxy;
    if (service) {
        connect(new QDBusPendingCallWatcher(service->Remove(), service),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(handleRemoveReply(QDBusPendingCallWatcher*)));
    }
}

void NetworkService::setAutoConnect(bool autoConnected)
{
    Private::InterfaceProxy* service = m_priv->m_proxy;
    if (service) {
         connect(new QDBusPendingCallWatcher(
            service->SetProperty(AutoConnect, autoConnected), service),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(handleAutoConnectReply(QDBusPendingCallWatcher*)));
    }
}

void NetworkService::handleAutoConnectReply(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<> reply = *watcher;
    watcher->deleteLater();

    if (reply.isError())
        DBG_(reply.error().message());
    // propertyChange should handle emit
}

void NetworkService::setIpv4Config(const QVariantMap &ipv4)
{
    m_priv->setProperty(IPv4Config, ipv4);
}

void NetworkService::setIpv6Config(const QVariantMap &ipv6)
{
    m_priv->setProperty(IPv6Config, ipv6);
}

void NetworkService::setNameserversConfig(const QStringList &nameservers)
{
    m_priv->setProperty(NameserversConfig, nameservers);
}

void NetworkService::setDomainsConfig(const QStringList &domains)
{
    m_priv->setProperty(DomainsConfig, domains);
}

void NetworkService::setProxyConfig(const QVariantMap &proxy)
{
    m_priv->setProperty(ProxyConfig, Private::adaptToConnmanProperties(proxy));
}

void NetworkService::resetCounters()
{
    if (m_priv->m_proxy) {
        m_priv->m_proxy->ResetCounters();
    }
}

void NetworkService::handleConnectReply(QDBusPendingCallWatcher *call)
{
    Q_ASSERT(call);
    QDBusPendingReply<> reply = *call;

    if (!reply.isFinished()) {
       DBG_("connect() not finished yet");
    }
    if (reply.isError()) {
        DBG_("Reply from service.connect(): " << reply.error().message());
        Q_EMIT connectRequestFailed(reply.error().message());
    }

    call->deleteLater();
}

void NetworkService::handleRemoveReply(QDBusPendingCallWatcher *watcher)
{
    QDBusPendingReply<> reply = *watcher;
    watcher->deleteLater();

    if (reply.isError() && reply.error().type() == QDBusError::UnknownObject) {
        // Service is probably out of range trying RemoveSavedService.
        NetConnmanManagerInterface manager(QLatin1String("net.connman"), QLatin1String("/"),
                                           QDBusConnection::systemBus());

        // Remove /net/connman/service/ from front of string.
        manager.RemoveSavedService(m_path.mid(21));
    }
}

void NetworkService::resetProperties()
{
    QMutableMapIterator<QString, QVariant> i(m_propertiesCache);
    while (i.hasNext()) {
        i.next();

        const QString key = i.key();
        i.remove();

        if (key == Name) {
            Q_EMIT nameChanged(name());
        } else if (key == Error) {
            Q_EMIT errorChanged(error());
        } else if (key == State) {
            Q_EMIT stateChanged(state());
            if (m_connected != connected()) {
                m_connected = connected();
                Q_EMIT connectedChanged(m_connected);
            }
        } else if (key == Security) {
            Q_EMIT securityChanged(security());
            if (m_priv->updateSecurityType()) {
                Q_EMIT securityTypeChanged();
            }
        } else if (key == Strength) {
            Q_EMIT strengthChanged(strength());
        } else if (key == Favorite) {
            Q_EMIT favoriteChanged(favorite());
        } else if (key == AutoConnect) {
            Q_EMIT autoConnectChanged(autoConnect());
        } else if (key == IPv4) {
            Q_EMIT ipv4Changed(ipv4());
        } else if (key == IPv4Config) {
            Q_EMIT ipv4ConfigChanged(ipv4Config());
        } else if (key == IPv6) {
            Q_EMIT ipv6Changed(ipv6());
        } else if (key == IPv6Config) {
            Q_EMIT ipv6ConfigChanged(ipv6Config());
        } else if (key == Nameservers) {
            Q_EMIT nameserversChanged(nameservers());
        } else if (key == NameserversConfig) {
            Q_EMIT nameserversConfigChanged(nameserversConfig());
        } else if (key == Domains) {
            Q_EMIT domainsChanged(domains());
        } else if (key == DomainsConfig) {
            Q_EMIT domainsConfigChanged(domainsConfig());
        } else if (key == Proxy) {
            Q_EMIT proxyChanged(proxy());
        } else if (key == ProxyConfig) {
            Q_EMIT proxyConfigChanged(proxyConfig());
        } else if (key == Ethernet) {
            Q_EMIT ethernetChanged(ethernet());
        } else if (key == Type) {
            Q_EMIT typeChanged(type());
        } else if (key == Roaming) {
            Q_EMIT roamingChanged(roaming());
        } else if (key == Timeservers) {
            Q_EMIT timeserversChanged(timeservers());
        } else if (key == TimeserversConfig) {
            Q_EMIT timeserversConfigChanged(timeserversConfig());
        } else if (key == BSSID) {
            Q_EMIT bssidChanged(bssid());
        } else if (key == MaxRate) {
            Q_EMIT maxRateChanged(maxRate());
        } else if (key == Frequency) {
            Q_EMIT frequencyChanged(frequency());
        } else if (key == EncryptionMode) {
            Q_EMIT encryptionModeChanged(encryptionMode());
        } else if (key == Hidden) {
            Q_EMIT hiddenChanged(hidden());
        } else if (key == Private::EAP) {
            if (m_priv->m_eapMethodAvailable) {
                m_priv->m_eapMethodAvailable = false;
                Q_EMIT eapMethodAvailableChanged(false);
            }
            Q_EMIT eapMethodChanged();
        } else if (key == Private::Identity) {
            if (m_priv->m_identityAvailable) {
                m_priv->m_identityAvailable = false;
                Q_EMIT identityAvailableChanged(false);
            }
            Q_EMIT identityChanged(identity());
        } else if (key == Private::Passphrase) {
            if (m_priv->m_passphraseAvailable) {
                m_priv->m_passphraseAvailable = false;
                Q_EMIT passphraseAvailableChanged(false);
            }
            Q_EMIT passphraseChanged(passphrase());
        }
    }
}

void NetworkService::reconnectServiceInterface()
{
    m_priv->deleteProxy();

    if (m_path.isEmpty())
        return;

    Private::InterfaceProxy* service = m_priv->createProxy(m_path);
    connect(service, SIGNAL(PropertyChanged(QString,QDBusVariant)),
        SLOT(updateProperty(QString,QDBusVariant)));

    if (state().isEmpty() || m_path == QStringLiteral("/")) {
        // saved services have an empty state and cached properties
        QTimer::singleShot(500, this, SIGNAL(propertiesReady()));
    } else {
        connect(new QDBusPendingCallWatcher(service->GetProperties(), service),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(getPropertiesFinished(QDBusPendingCallWatcher*)));
    }
}

void NetworkService::emitPropertyChange(const QString &name, const QVariant &value)
{
    if (m_propertiesCache.value(name) == value)
        return;

    m_propertiesCache[name] = value;

    if (name == Name) {
        Q_EMIT nameChanged(value.toString());
    } else if (name == Error) {
        Q_EMIT errorChanged(value.toString());
    } else if (name == State) {
        Q_EMIT stateChanged(value.toString());
        if (m_connected != connected()) {
            m_connected = connected();
            Q_EMIT connectedChanged(m_connected);
        }
    } else if (name == Security) {
        Q_EMIT securityChanged(value.toStringList());
        if (m_priv->updateSecurityType()) {
            Q_EMIT securityTypeChanged();
        }
    } else if (name == Strength) {
        Q_EMIT strengthChanged(value.toUInt());
    } else if (name == Favorite) {
        Q_EMIT favoriteChanged(value.toBool());
    } else if (name == AutoConnect) {
        Q_EMIT autoConnectChanged(value.toBool());
    } else if (name == IPv4) {
        Q_EMIT ipv4Changed(qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv4)));
    } else if (name == IPv4Config) {
        Q_EMIT ipv4ConfigChanged(qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv4Config)));
    } else if (name == IPv6) {
        Q_EMIT ipv6Changed(qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv6)));
    } else if (name == IPv6Config) {
        Q_EMIT ipv6ConfigChanged(qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv6Config)));
    } else if (name == Nameservers) {
        Q_EMIT nameserversChanged(value.toStringList());
    } else if (name == NameserversConfig) {
        Q_EMIT nameserversConfigChanged(value.toStringList());
    } else if (name == Domains) {
        Q_EMIT domainsChanged(value.toStringList());
    } else if (name == DomainsConfig) {
        Q_EMIT domainsConfigChanged(value.toStringList());
    } else if (name == Proxy) {
        Q_EMIT proxyChanged(qdbus_cast<QVariantMap>(m_propertiesCache.value(Proxy)));
    } else if (name == ProxyConfig) {
        Q_EMIT proxyConfigChanged(qdbus_cast<QVariantMap>(m_propertiesCache.value(ProxyConfig)));
    } else if (name == Ethernet) {
        Q_EMIT ethernetChanged(qdbus_cast<QVariantMap>(m_propertiesCache.value(Ethernet)));
    } else if (name == QLatin1String("Type")) {
        Q_EMIT typeChanged(value.toString());
    } else if (name == Roaming) {
        Q_EMIT roamingChanged(value.toBool());
    } else if (name == Timeservers) {
        Q_EMIT timeserversChanged(value.toStringList());
    } else if (name == TimeserversConfig) {
        Q_EMIT timeserversConfigChanged(value.toStringList());
    } else if (name == BSSID) {
        Q_EMIT bssidChanged(value.toString());
    } else if (name == MaxRate) {
        Q_EMIT maxRateChanged(value.toUInt());
    } else if (name == Frequency) {
        Q_EMIT frequencyChanged(value.toUInt());
    } else if (name == EncryptionMode) {
        Q_EMIT encryptionModeChanged(value.toString());
    } else if (name == Hidden) {
        Q_EMIT hiddenChanged(value.toBool());
    } else if (name == Private::EAP) {
        Q_EMIT eapMethodChanged();
        if (!m_priv->m_eapMethodAvailable) {
            m_priv->m_eapMethodAvailable = true;
            Q_EMIT eapMethodAvailableChanged(true);
        }
    } else if (name == Private::Identity) {
        Q_EMIT identityChanged(value.toString());
        if (!m_priv->m_identityAvailable) {
            m_priv->m_identityAvailable = true;
            Q_EMIT identityAvailableChanged(true);
        }
    } else if (name == Private::Passphrase) {
        Q_EMIT passphraseChanged(value.toString());
        if (!m_priv->m_passphraseAvailable) {
            m_priv->m_passphraseAvailable = true;
            Q_EMIT passphraseAvailableChanged(true);
        }
    }
}

void NetworkService::getPropertiesFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QVariantMap> reply = *call;
    call->deleteLater();

    if (!reply.isError()) {
        updateProperties(reply.value());
    } else {
        DBG_(reply.error().message());
    }
    Q_EMIT propertiesReady();
}

void NetworkService::updateProperty(const QString &name, const QDBusVariant &value)
{
    emitPropertyChange(name, value.variant());
}

void NetworkService::updateProperties(const QVariantMap &properties)
{
    QVariantMap::const_iterator it = properties.constBegin(), end = properties.constEnd();
    for ( ; it != end; ++it) {
        emitPropertyChange(it.key(), it.value());
    }
    Q_EMIT propertiesReady();
}

void NetworkService::setPath(const QString &path)
{
    if (path == m_path)
        return;

    m_path = path;
    emit pathChanged(m_path);

    resetProperties();
    reconnectServiceInterface();
}

bool NetworkService::connected()
{
    if (m_propertiesCache.contains(State)) {
        QString state = m_propertiesCache.value(State).toString();
        if (state == "online" || state == "ready")
            return true;
    }
    return false;
}

QStringList NetworkService::timeservers() const
{
    return m_propertiesCache.value(Timeservers).toStringList();
}

QStringList NetworkService::timeserversConfig() const
{
    if (m_propertiesCache.contains(TimeserversConfig))
        return m_propertiesCache.value(TimeserversConfig).toStringList();
    return QStringList();
}

void NetworkService::setTimeserversConfig(const QStringList &servers)
{
    m_priv->setProperty(TimeserversConfig, servers);
}

const QString NetworkService::bssid()
{
    return m_propertiesCache.value(BSSID).toString();
}

quint32 NetworkService::maxRate()
{
    return m_propertiesCache.value(MaxRate).toUInt();
}

quint16 NetworkService::frequency()
{
    return m_propertiesCache.value(Frequency).toUInt();
}

const QString NetworkService::encryptionMode()
{
    return m_propertiesCache.value(EncryptionMode).toString();
}

QString NetworkService::bssid() const
{
    return m_propertiesCache.value(BSSID).toString();
}

quint32 NetworkService::maxRate() const
{
    return m_propertiesCache.value(MaxRate).toUInt();
}

quint16 NetworkService::frequency() const
{
    return m_propertiesCache.value(Frequency).toUInt();
}

QString NetworkService::encryptionMode() const
{
    return m_propertiesCache.value(EncryptionMode).toString();
}

QString NetworkService::passphrase() const
{
    return m_propertiesCache.value(Private::Passphrase).toString();
}

void NetworkService::setPassphrase(QString passphrase)
{
    m_priv->setProperty(Private::Passphrase, passphrase);
}

bool NetworkService::passphraseAvailable() const
{
    return m_priv->m_passphraseAvailable;
}

QString NetworkService::identity() const
{
    return m_propertiesCache.value(Private::Identity).toString();
}

void NetworkService::setIdentity(QString identity)
{
    m_priv->setProperty(Private::Identity, identity);
}

bool NetworkService::identityAvailable() const
{
    return m_priv->m_identityAvailable;
}

NetworkService::SecurityType NetworkService::securityType() const
{
    return m_priv->m_securityType;
}

NetworkService::EapMethod NetworkService::eapMethod() const
{
    return m_priv->eapMethod();
}

void NetworkService::setEapMethod(EapMethod method)
{
    m_priv->setEapMethod(method);
}

bool NetworkService::eapMethodAvailable() const
{
    return m_priv->m_eapMethodAvailable;
}

#include "networkservice.moc"
