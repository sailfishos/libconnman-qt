/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2017 Jolla Ltd.
 * Contact: Slava Monich <slava.monich@jolla.com>
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0. The full text of the Apache License
 * is at http://www.apache.org/licenses/LICENSE-2.0
 */

#include "networkservice.h"
#include "networkmanager.h"
#include "libconnman_p.h"

#define COUNT(a) ((uint)(sizeof(a)/sizeof(a[0])))

#define NETWORK_SERVICE_PROPERTIES(ConnmanArg,ConnmanNoArg,ClassArg,ClassNoArg) \
    ClassArg(Path,path) \
    ClassArg(Connected,connected) \
    ClassNoArg(Connecting,connecting) \
    ClassNoArg(Managed,managed) \
    ClassNoArg(SecurityType,securityType) \
    ClassNoArg(EapMethod,eapMethod) \
    ClassNoArg(PassphraseAvailable,passphraseAvailable) \
    ClassNoArg(IdentityAvailable,identityAvailable) \
    ClassNoArg(EapMethodAvailable,eapMethodAvailable) \
    ClassNoArg(LastConnectError,lastConnectError) \
    ConnmanArg("Type",Type,type) \
    ConnmanArg("Name",Name,name) \
    ConnmanArg("State",State,state) \
    ConnmanArg("Error",Error,error) \
    ConnmanArg("Security",Security,security) \
    ConnmanArg("Strength",Strength,strength) \
    ConnmanArg("Favorite",Favorite,favorite) \
    ConnmanArg("AutoConnect",AutoConnect,autoConnect) \
    ConnmanArg("IPv4",Ipv4,ipv4) \
    ConnmanArg("IPv4.Configuration",Ipv4Config,ipv4Config) \
    ConnmanArg("IPv6",Ipv6,ipv6) \
    ConnmanArg("IPv6.Configuration",Ipv6Config,ipv6Config) \
    ConnmanArg("Nameservers",Nameservers,nameservers) \
    ConnmanArg("Nameservers.Configuration",NameserversConfig,nameserversConfig) \
    ConnmanArg("Domains",Domains,domains) \
    ConnmanArg("Domains.Configuration",DomainsConfig,domainsConfig) \
    ConnmanArg("Proxy",Proxy,proxy) \
    ConnmanArg("Proxy.Configuration",ProxyConfig,proxyConfig) \
    ConnmanArg("Ethernet",Ethernet,ethernet) \
    ConnmanArg("Roaming",Roaming,roaming) \
    ConnmanArg("Timeservers",Timeservers,timeservers) \
    ConnmanArg("Timeservers.Configuration",TimeserversConfig,timeserversConfig) \
    ConnmanArg("BSSID",Bssid,bssid) \
    ConnmanArg("MaxRate",MaxRate,maxRate) \
    ConnmanArg("Frequency",Frequency,frequency) \
    ConnmanArg("EncryptionMode",EncryptionMode,encryptionMode) \
    ConnmanArg("Hidden",Hidden,hidden) \
    ConnmanArg("Passphrase",Passphrase,passphrase) \
    ConnmanArg("Identity",Identity,identity) \
    ConnmanNoArg("Available",Available,available) \
    ConnmanNoArg("Saved",Saved,saved)

#define NETWORK_SERVICE_PROPERTIES2(Connman,Class) \
    NETWORK_SERVICE_PROPERTIES(Connman,Connman,Class,Class)

#define IGNORE(X,x)

// New private date and methods are added to NetworkService::Private
// whenever possible to avoid contaminating the public header with
// irrelevant information. The old redundant stuff (the one which
// existed before NetworkService::Private was introduced) is still
// kept in the public header, for backward ABI compatibility.

class NetworkService::Private: public QObject
{
    Q_OBJECT

public:
    class InterfaceProxy;
    class GetPropertyWatcher;
    typedef QHash<QString,EapMethod> EapMethodMap;
    typedef QSharedPointer<EapMethodMap> EapMethodMapRef;
    typedef void (NetworkService::Private::*SignalEmitter)(NetworkService*);

    static const QString EapMethodName[];
    static const QString SecurityTypeName[];
    static const QString PolicyPrefix;

#define DECLARE_STATIC_STRING(K,X,x) static const QString X;
    NETWORK_SERVICE_PROPERTIES2(DECLARE_STATIC_STRING,IGNORE)
    static const QString Access;
    static const QString DefaultAccess;
    static const QString EAP;

    enum PropertyFlags {
        PropertyNone          = 0x00000000,
        PropertyAccess        = 0x00000001,
        PropertyDefaultAccess = 0x00000002,
        PropertyPassphrase    = 0x00000004,
        PropertyIdentity      = 0x00000008,
        PropertyEAP           = 0x00000010,
        PropertyAll           = 0x0000001f
    };

    enum CallFlags {
        CallNone              = 0x00000000,
        CallClearProperty     = 0x00000001,
        CallConnect           = 0x00000002,
        CallDisconnect        = 0x00000004,
        CallRemove            = 0x00000008,
        CallResetCounters     = 0x00000010,
        CallGetProperties     = 0x00000020,
        CallGetProperty       = 0x00000040,
        CallSetProperty       = 0x00000080,
        CallAll               = 0x000000ff
    };

    enum Signal {
        NoSignal = -1,
#define SIGNAL_ID_(K,X,x) SIGNAL_ID(X,x)
#define SIGNAL_ID(X,x) Signal##X##Changed,
    NETWORK_SERVICE_PROPERTIES2(SIGNAL_ID_,SIGNAL_ID)
        SignalCount
    };

    struct PropertyAccessInfo {
        const QString &name;
        PropertyFlags flag;
        Signal sig;
    };

    // Properties that are subject to access control
    static const PropertyAccessInfo* Properties[];
    static const PropertyAccessInfo PropAccess;
    static const PropertyAccessInfo PropDefaultAccess;
    static const PropertyAccessInfo PropIdentity;
    static const PropertyAccessInfo PropPassphrase;
    static const PropertyAccessInfo PropEAP;

    static QVariantMap adaptToConnmanProperties(const QVariantMap &map);

    Private(const QString &path, const QVariantMap &properties, NetworkService *parent);

    void deleteProxy();
    void setPath(const QString &path);
    InterfaceProxy* createProxy(const QString &path);
    NetworkService* service();
    EapMethodMapRef eapMethodMap();
    EapMethod eapMethod();
    uint uintValue(const QString &key);
    bool boolValue(const QString &key, bool defaultValue = false);
    QVariantMap variantMapValue(const QString &key);
    QStringList stringListValue(const QString &key);
    QString stringValue(const QString &key);
    QString state();
    bool managed();
    bool requestConnect();
    void updateSecurityType();
    void setEapMethod(EapMethod method);
    void setProperty(const QString &name, const QVariant &value);
    void setPropertyAvailable(const PropertyAccessInfo *prop, bool available);
    void setLastConnectError(const QString &error);
    void updateProperties(QVariantMap properties);
    void updateConnecting();
    void updateConnected();
    void updateState();
    void updateManaged();
    void queueSignal(Signal sig);
    void emitQueuedSignals();

#if HAVE_LIBDBUSACCESS
    void policyCheck(const QString &rules);
#endif // HAVE_LIBDBUSACCESS

private Q_SLOTS:
    void onPropertyChanged(const QString &name, const QDBusVariant &value);
    void onRestrictedPropertyChanged(const QString &name);
    void onGetPropertyFinished(QDBusPendingCallWatcher *call);
    void onGetPropertiesFinished(QDBusPendingCallWatcher *call);
    void onCheckAccessFinished(QDBusPendingCallWatcher *call);
    void onConnectFinished(QDBusPendingCallWatcher *call);

private:
    void updateConnecting(const QString &state);
    void updateConnected(const QString &state);
    void checkAccess();
    void resetProperties();
    void reconnectServiceInterface();
    void updatePropertyCache(const QString &name, const QVariant &value);

    // Wrappers for signal emitters
#define DEFINE_EMITTER_CONNMAN_ARG(K,X,x) DEFINE_EMITTER_ARG(X,x)
#define DEFINE_EMITTER_CONNMAN_NO_ARG(K,X,x) DEFINE_EMITTER_NO_ARG(X,x)
#define DEFINE_EMITTER_ARG(X,x) \
    void x##Changed(NetworkService *obj) { obj->x##Changed(obj->x()); }
#define DEFINE_EMITTER_NO_ARG(X,x) \
    void x##Changed(NetworkService *obj) { obj->x##Changed(); }
NETWORK_SERVICE_PROPERTIES(DEFINE_EMITTER_CONNMAN_ARG,\
    DEFINE_EMITTER_CONNMAN_NO_ARG,\
    DEFINE_EMITTER_ARG,\
    DEFINE_EMITTER_NO_ARG)

public:
    QString m_path;
    QVariantMap m_propertiesCache;
    InterfaceProxy *m_proxy;
    QDBusPendingCallWatcher *m_connectWatcher;
    EapMethodMapRef m_eapMethodMapRef;
    SecurityType m_securityType;
    uint m_propGetFlags;
    uint m_propSetFlags;
    uint m_callFlags;
    bool m_managed;
    bool m_connecting;
    bool m_connected;
    QString m_lastConnectError;
    QString m_state;

private:
    quint64 m_queuedSignals;
    int m_firstQueuedSignal;
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
//   <method name="CheckAccess">
//     <arg name="get_properties" type="u" direction="out"/>
//     <arg name="set_properties" type="u" direction="out"/>
//     <arg name="calls" type="u" direction="out"/>
//   </method>
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
    InterfaceProxy(const QString &path, NetworkService::Private *parent) :
        QDBusAbstractInterface(CONNMAN_SERVICE, path, "net.connman.Service",
            CONNMAN_BUS, parent) {}

public Q_SLOTS:
    QDBusPendingCall GetProperties()
        { return asyncCall("GetProperties"); }
    QDBusPendingCall GetProperty(const QString &name)
        { return asyncCall("GetProperty", name); }
    QDBusPendingCall SetProperty(const QString &name, QVariant value)
        { return asyncCall("SetProperty", name, qVariantFromValue(QDBusVariant(value))); }
    QDBusPendingCall ClearProperty(const QString &name)
        { return asyncCall("ClearProperty", name); }
    QDBusPendingCall Connect()
        { return asyncCall("Connect"); }
    QDBusPendingCall Disconnect()
        { return asyncCall("Disconnect"); }
    QDBusPendingCall Remove()
        { return asyncCall("Remove"); }
    QDBusPendingCall ResetCounters()
        { return asyncCall("ResetCounters"); }
    QDBusPendingCall CheckAccess()
        { return asyncCall("CheckAccess"); }

Q_SIGNALS:
    void PropertyChanged(const QString &name, const QDBusVariant &value);
    void RestrictedPropertyChanged(const QString &name);
};

// ==========================================================================
// NetworkService::Private::GetPropertyWatcher
// ==========================================================================

class NetworkService::Private::GetPropertyWatcher : public QDBusPendingCallWatcher {
public:
    GetPropertyWatcher(const QString &name, InterfaceProxy *proxy) :
        QDBusPendingCallWatcher(proxy->GetProperty(name), proxy),
        m_name(name) {}
    QString m_name;
};

// ==========================================================================
// NetworkService::Private
// ==========================================================================

#define DEFINE_STATIC_STRING(K,X,x) const QString NetworkService::Private::X(K);
NETWORK_SERVICE_PROPERTIES2(DEFINE_STATIC_STRING,IGNORE)
const QString NetworkService::Private::Access("Access");
const QString NetworkService::Private::DefaultAccess("DefaultAccess");
const QString NetworkService::Private::EAP("EAP");

const QString NetworkService::Private::PolicyPrefix("sailfish:");

const NetworkService::Private::PropertyAccessInfo NetworkService::Private::PropAccess =
    { NetworkService::Private::Access, NetworkService::Private::PropertyAccess,
      NetworkService::Private::NoSignal };
const NetworkService::Private::PropertyAccessInfo NetworkService::Private::PropDefaultAccess =
    { NetworkService::Private::DefaultAccess, NetworkService::Private::PropertyDefaultAccess,
      NetworkService::Private::NoSignal };
const NetworkService::Private::PropertyAccessInfo NetworkService::Private::PropIdentity =
    { NetworkService::Private::Identity, NetworkService::Private::PropertyPassphrase,
      NetworkService::Private::SignalIdentityAvailableChanged };
const NetworkService::Private::PropertyAccessInfo NetworkService::Private::PropPassphrase =
    { NetworkService::Private::Passphrase, NetworkService::Private::PropertyIdentity,
      NetworkService::Private::SignalPassphraseAvailableChanged };
const NetworkService::Private::PropertyAccessInfo NetworkService::Private::PropEAP =
    { NetworkService::Private::EAP, NetworkService::Private::PropertyEAP,
      NetworkService::Private::SignalEapMethodAvailableChanged };

const NetworkService::Private::PropertyAccessInfo* NetworkService::Private::Properties[] = {
    &NetworkService::Private::PropAccess,
    &NetworkService::Private::PropDefaultAccess,
    &NetworkService::Private::PropIdentity,
    &NetworkService::Private::PropPassphrase,
    &NetworkService::Private::PropEAP
};

// The order must match EapMethod enum
const QString NetworkService::Private::EapMethodName[] = {
    QString(), "peap", "ttls", "tls"
};

// The order must match SecurityType enum
const QString NetworkService::Private::SecurityTypeName[] = {
    QString(), "none", "wep", "psk", "ieee8021x"
};

NetworkService::Private::Private(const QString &path, const QVariantMap &props, NetworkService *parent) :
    QObject(parent),
    m_path(path),
    m_propertiesCache(props),
    m_proxy(NULL),
    m_connectWatcher(NULL),
    m_securityType(SecurityNone),
    m_propGetFlags(PropertyEAP),
    m_propSetFlags(PropertyNone),
    m_callFlags(CallAll),
    m_managed(false),
    m_connecting(false),
    m_connected(false),
    m_queuedSignals(0),
    m_firstQueuedSignal(0)
{
    qRegisterMetaType<NetworkService *>();
    updateSecurityType();

    // Make (reasonably) sure that NetworkManager is up to date when
    // we need to pull inputRequestTimeout out of it. By checking the
    // path for "/" we avoid infinite recursion - this path is passed
    // to some sort of special NetworkService created by NetworkManager
    // constructor itself.
    if (m_path != "/") {
        NetworkManagerFactory::createInstance();
    }

    // If the property is present in GetProperties output, it means that it's
    // at least gettable for us
    for (uint i=0; i<COUNT(Properties); i++) {
        const PropertyAccessInfo *prop = Properties[i];
        if (m_propertiesCache.contains(prop->name)) {
            m_propGetFlags |= prop->flag;
        }
    }

#if HAVE_LIBDBUSACCESS
    // Pre-check the access locally, to initialize access control flags
    // to the right values from the beginning. Otherwise we would have
    // to live with the default values until CheckAccess call completes
    // (which results in unpleasant UI flickering during initialization).

    // Note that CheckAccess call will be made anyway but most likely
    // it will returns exactly the same flags as we figure out ourself
    // (and there won't be any unnecessary property changes causing UI
    // to flicker).
    QString access = stringValue(Access);
    if (access.isEmpty()) {
        access = stringValue(DefaultAccess);
    }
    if (access.startsWith(PolicyPrefix)) {
        const int len = access.length()- PolicyPrefix.length();
        policyCheck(access.right(len));
    }
#endif // HAVE_LIBDBUSACCESS

    reconnectServiceInterface();
    updateManaged();
    DBG_(m_path << "managed:" << m_managed);

    // Reset the signal mask (the above calls may have set some bits)
    m_queuedSignals = 0;
}

bool NetworkService::Private::managed()
{
    // This defines the criteria of being "managed"
    return !(m_callFlags & CallRemove) && boolValue(Saved);
}

inline void NetworkService::Private::queueSignal(Signal sig)
{
    if (sig > NoSignal && sig < SignalCount) {
        const quint64 signalBit = (Q_UINT64_C(1) << sig);
        if (m_queuedSignals) {
            m_queuedSignals |= signalBit;
            if (m_firstQueuedSignal > sig) {
                m_firstQueuedSignal = sig;
            }
        } else {
            m_queuedSignals = signalBit;
            m_firstQueuedSignal = sig;
        }
    }
}

void NetworkService::Private::emitQueuedSignals()
{
    static const SignalEmitter emitSignal [] = {
#define REFERENCE_EMITTER_(K,X,x) REFERENCE_EMITTER(X,x)
#define REFERENCE_EMITTER(X,x) &NetworkService::Private::x##Changed,
    NETWORK_SERVICE_PROPERTIES2(REFERENCE_EMITTER_,REFERENCE_EMITTER)
    };

    Q_STATIC_ASSERT(COUNT(emitSignal) == SignalCount);
    if (m_queuedSignals) {
        NetworkService *obj = service();
        for (int i = m_firstQueuedSignal; i < SignalCount && m_queuedSignals; i++) {
            const quint64 signalBit = (Q_UINT64_C(1) << i);
            if (m_queuedSignals & signalBit) {
                m_queuedSignals &= ~signalBit;
                Q_EMIT (this->*(emitSignal[i]))(obj);
            }
        }
    }
}

void NetworkService::Private::setPath(const QString &path)
{
    if (m_path != path) {
        m_path = path;
        queueSignal(SignalPathChanged);
        resetProperties();
        reconnectServiceInterface();
        emitQueuedSignals();
    }
}

void NetworkService::Private::deleteProxy()
{
    delete m_proxy;
    m_proxy = 0;
}

NetworkService::Private::InterfaceProxy*
NetworkService::Private::createProxy(const QString &path)
{
    delete m_proxy;
    m_proxy = new InterfaceProxy(path, this);
    connect(m_proxy, SIGNAL(RestrictedPropertyChanged(QString)),
        SLOT(onRestrictedPropertyChanged(QString)));
    checkAccess();
    return m_proxy;
}

inline NetworkService* NetworkService::Private::service()
{
    return (NetworkService*)parent();
}

void NetworkService::Private::checkAccess()
{
    connect(new QDBusPendingCallWatcher(m_proxy->CheckAccess(), m_proxy),
        SIGNAL(finished(QDBusPendingCallWatcher*)),
        SLOT(onCheckAccessFinished(QDBusPendingCallWatcher*)));
}

void NetworkService::Private::onRestrictedPropertyChanged(const QString &name)
{
    DBG_(name);
    connect(new GetPropertyWatcher(name, m_proxy),
        SIGNAL(finished(QDBusPendingCallWatcher*)),
        SLOT(onGetPropertyFinished(QDBusPendingCallWatcher*)));
    if (name == Access) {
        checkAccess();
    }
}

void NetworkService::Private::onGetPropertyFinished(QDBusPendingCallWatcher *call)
{
    GetPropertyWatcher *watcher = (GetPropertyWatcher*)call;
    QDBusPendingReply<QVariant> reply = *call;
    call->deleteLater();
    if (reply.isError()) {
        DBG_(watcher->m_name << reply.error());
    } else {
        DBG_(watcher->m_name << "=" << reply.value());
        updatePropertyCache(watcher->m_name, reply.value());
        emitQueuedSignals();
    }
}

void NetworkService::Private::onCheckAccessFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<uint,uint,uint> reply = *call;
    call->deleteLater();
    if (reply.isError()) {
        DBG_(m_path << reply.error());
    } else {
        const uint get_props = reply.argumentAt<0>();
        const uint set_props = reply.argumentAt<1>();
        const uint calls = reply.argumentAt<2>();

        DBG_(get_props << set_props << calls);

        const uint prev = m_propGetFlags;
        const bool wasManaged = managed();
        m_propGetFlags = get_props;
        m_propSetFlags = set_props;
        m_callFlags = calls;

        for (uint i=0; i<COUNT(Properties); i++) {
            const PropertyAccessInfo *p = Properties[i];
            if ((m_propGetFlags & p->flag) != (prev & p->flag)) {
                queueSignal(p->sig);
            }
        }

        m_managed = managed();
        if (m_managed != wasManaged) {
            DBG_(m_path << "managed:" << m_managed);
            queueSignal(SignalManagedChanged);
        }

        emitQueuedSignals();
    }
}

void NetworkService::Private::updateSecurityType()
{
    SecurityType type = SecurityUnknown;
    const QStringList security = stringListValue(Security);
    if (!security.isEmpty()) {
        // Start with 1 because 0 is SecurityUnknown
        for (uint i=1; i<COUNT(SecurityTypeName); i++) {
            if (security.contains(SecurityTypeName[i])) {
                type = (SecurityType)i;
                break;
            }
        }
    }
    if (m_securityType != type) {
        m_securityType = type;
        queueSignal(SignalSecurityTypeChanged);
    }
}

NetworkService::Private::EapMethodMapRef NetworkService::Private::eapMethodMap()
{
    static QWeakPointer<EapMethodMap> sharedInstance;
    m_eapMethodMapRef = sharedInstance;
    if (m_eapMethodMapRef.isNull()) {
        EapMethodMap *map = new EapMethodMap;
        // Start with 1 because 0 is EapNone
        for (uint i=1; i<COUNT(EapMethodName); i++) {
            const QString name = EapMethodName[i];
            map->insert(name.toLower(), (EapMethod)i);
            map->insert(name.toUpper(), (EapMethod)i);
        }
        m_eapMethodMapRef = EapMethodMapRef(map);
    }
    return m_eapMethodMapRef;
}

inline uint NetworkService::Private::uintValue(const QString &key)
{
    return m_propertiesCache.value(key).toUInt();
}

inline bool NetworkService::Private::boolValue(const QString &key, bool defaultValue)
{
    return m_propertiesCache.value(key, defaultValue).toBool();
}

inline QVariantMap NetworkService::Private::variantMapValue(const QString &key)
{
    if (m_propertiesCache.contains(key)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(key));
    }
    return QVariantMap();
}

inline QStringList NetworkService::Private::stringListValue(const QString &key)
{
    return m_propertiesCache.value(key).toStringList();
}

inline QString NetworkService::Private::stringValue(const QString &key)
{
    return m_propertiesCache.value(key).toString();
}

inline QString NetworkService::Private::state()
{
    return stringValue(State);
}

NetworkService::EapMethod NetworkService::Private::eapMethod()
{
    QString eap = stringValue(EAP);
    if (eap.isEmpty()) {
        return EapNone;
    } else {
        return eapMethodMap()->value(eap, EapNone);
    }
}

void NetworkService::Private::setEapMethod(EapMethod method)
{
    if (method >= EapNone && method < COUNT(EapMethodName)) {
        setProperty(EAP, EapMethodName[method]);
    }
}

void NetworkService::Private::setProperty(const QString &name, const QVariant &value)
{
    if (m_proxy) {
        m_proxy->SetProperty(name, value);
    }
}

void NetworkService::Private::setLastConnectError(const QString &error)
{
    if (m_lastConnectError != error) {
        m_lastConnectError = error;
        queueSignal(SignalLastConnectErrorChanged);
    }
}

void NetworkService::Private::updateConnecting(const QString &state)
{
    bool connecting = m_connectWatcher || ConnmanState::connecting(state);
    if (m_connecting != connecting) {
        m_connecting = connecting;
        queueSignal(SignalConnectingChanged);
    }
}

void NetworkService::Private::updateConnecting()
{
    updateConnecting(state());
}

void NetworkService::Private::updateConnected(const QString &state)
{
    bool connected = !m_connectWatcher && ConnmanState::connected(state);
    if (m_connected != connected) {
        m_connected = connected;
        queueSignal(SignalConnectedChanged);
    }
}

void NetworkService::Private::updateConnected()
{
    updateConnected(state());
}

void NetworkService::Private::updateState()
{
    QString currentState(state());
    if (m_state != currentState) {
        m_state = currentState;
        queueSignal(SignalStateChanged);
        updateConnecting(currentState);
        updateConnected(currentState);
    }
}

void NetworkService::Private::updateManaged()
{
    bool isManaged = managed();
    if (m_managed != isManaged) {
        m_managed = isManaged;
        queueSignal(SignalManagedChanged);
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

void NetworkService::Private::setPropertyAvailable(const PropertyAccessInfo *prop, bool available)
{
    if (available) {
        if (!(m_propGetFlags & prop->flag)) {
            m_propGetFlags |= prop->flag;
            queueSignal(prop->sig);
        }
    } else {
        if (m_propGetFlags & prop->flag) {
            m_propGetFlags &= ~prop->flag;
            queueSignal(prop->sig);
        }
    }
}

void NetworkService::Private::reconnectServiceInterface()
{
    deleteProxy();

    if (m_path.isEmpty())
        return;

    if (m_path == QStringLiteral("/")) {
        // This is a dummy invalidDefaultRoute created by NetworkManager
        QTimer::singleShot(500, service(), SIGNAL(propertiesReady()));
    } else {
        InterfaceProxy *service = createProxy(m_path);
        connect(service, SIGNAL(PropertyChanged(QString,QDBusVariant)),
            SLOT(onPropertyChanged(QString,QDBusVariant)));
        connect(new QDBusPendingCallWatcher(service->GetProperties(), service),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onGetPropertiesFinished(QDBusPendingCallWatcher*)));
    }
}

void NetworkService::Private::onPropertyChanged(const QString &name, const QDBusVariant &value)
{
    updatePropertyCache(name, value.variant());
    emitQueuedSignals();
}

void NetworkService::Private::onGetPropertiesFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QVariantMap> reply = *call;
    call->deleteLater();
    if (!reply.isError()) {
        updateProperties(reply.value());
        emitQueuedSignals();
        Q_EMIT service()->propertiesReady();
    } else {
        DBG_(m_path << reply.error());
    }
}

bool NetworkService::Private::requestConnect()
{
    if (m_proxy) {
        // If the service is in the failure state clear the Error property
        // so that we get notified of errors on subsequent connection attempts.
        if (state() == ConnmanState::Failure) {
            m_proxy->ClearProperty(Error);
        }

        const int old_timeout = m_proxy->timeout();
        m_proxy->setTimeout(NetworkManager::instance()->inputRequestTimeout());
        QDBusPendingCall call = m_proxy->Connect();
        m_proxy->setTimeout(old_timeout);

        delete m_connectWatcher;
        m_connectWatcher = new QDBusPendingCallWatcher(call, m_proxy);

        setLastConnectError(QString());
        updateConnecting();
        updateConnected();

        connect(m_connectWatcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(onConnectFinished(QDBusPendingCallWatcher*)));

        emitQueuedSignals();
        return true;
    } else {
        return false;
    }
}

void NetworkService::Private::onConnectFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<> reply = *call;
    m_connectWatcher = NULL;
    call->deleteLater();

    if (reply.isError()) {
        QDBusError error(reply.error());
        QString errorName(error.name());
        DBG_(error);

        // InProgress means that somebody has already asked this service
        // to get connected. That's fine, we will keep watching the status.
        setLastConnectError((errorName == ConnmanError::InProgress) ? QString() : errorName);
        Q_EMIT service()->connectRequestFailed(error.message());
    } else {
        // Reset the last error on success
        setLastConnectError(QString());
    }

    updateConnecting();
    updateConnected();
    emitQueuedSignals();
}

void NetworkService::Private::updateProperties(QVariantMap properties)
{
    QMapIterator<QString, QVariant> it(properties);
    while (it.hasNext()) {
        it.next();
        updatePropertyCache(it.key(), it.value());
    }
}

void NetworkService::Private::resetProperties()
{
    QMutableMapIterator<QString, QVariant> it(m_propertiesCache);
    while (it.hasNext()) {
        it.next();
        QString key = it.key();
        QVariant value = it.value();
        it.remove();

        if (key == Name) {
            queueSignal(SignalNameChanged);
        } else if (key == Error) {
            queueSignal(SignalErrorChanged);
        } else if (key == State) {
            updateState();
        } else if (key == Security) {
            queueSignal(SignalSecurityChanged);
            updateSecurityType();
        } else if (key == Strength) {
            queueSignal(SignalStrengthChanged);
        } else if (key == Favorite) {
            if (value.toBool()) {
                queueSignal(SignalFavoriteChanged);
            }
        } else if (key == AutoConnect) {
            if (value.toBool()) {
                queueSignal(SignalAutoConnectChanged);
            }
        } else if (key == Ipv4) {
            queueSignal(SignalIpv4Changed);
        } else if (key == Ipv4Config) {
            queueSignal(SignalIpv4ConfigChanged);
        } else if (key == Ipv6) {
            queueSignal(SignalIpv6Changed);
        } else if (key == Ipv6Config) {
            queueSignal(SignalIpv6ConfigChanged);
        } else if (key == Nameservers) {
            queueSignal(SignalNameserversChanged);
        } else if (key == NameserversConfig) {
            queueSignal(SignalNameserversConfigChanged);
        } else if (key == Domains) {
            queueSignal(SignalDomainsChanged);
        } else if (key == DomainsConfig) {
            queueSignal(SignalDomainsConfigChanged);
        } else if (key == Proxy) {
            queueSignal(SignalProxyChanged);
        } else if (key == ProxyConfig) {
            queueSignal(SignalProxyConfigChanged);
        } else if (key == Ethernet) {
            queueSignal(SignalEthernetChanged);
        } else if (key == Type) {
            queueSignal(SignalTypeChanged);
        } else if (key == Roaming) {
            if (value.toBool()) {
                queueSignal(SignalRoamingChanged);
            }
        } else if (key == Timeservers) {
            queueSignal(SignalTimeserversChanged);
        } else if (key == TimeserversConfig) {
            queueSignal(SignalTimeserversConfigChanged);
        } else if (key == Bssid) {
            queueSignal(SignalBssidChanged);
        } else if (key == MaxRate) {
            queueSignal(SignalMaxRateChanged);
        } else if (key == Frequency) {
            queueSignal(SignalFrequencyChanged);
        } else if (key == EncryptionMode) {
            queueSignal(SignalEncryptionModeChanged);
        } else if (key == Hidden) {
            queueSignal(SignalHiddenChanged);
        } else if (key == Available) {
            if (value.toBool()) {
                queueSignal(SignalAvailableChanged);
            }
        } else if (key == Saved) {
            if (value.toBool()) {
                queueSignal(SignalSavedChanged);
            }
        } else if (key == Access) {
            setPropertyAvailable(&PropAccess, false);
        } else if (key == DefaultAccess) {
            setPropertyAvailable(&PropDefaultAccess, false);
        } else if (key == Passphrase) {
            queueSignal(SignalPassphraseChanged);
            setPropertyAvailable(&PropPassphrase, false);
        } else if (key == Identity) {
            queueSignal(SignalIdentityChanged);
            setPropertyAvailable(&PropIdentity, false);
        } else if (key == EAP) {
            queueSignal(SignalEapMethodChanged);
            setPropertyAvailable(&PropEAP, false);
        }
    }
    updateManaged();
    emitQueuedSignals();
}

void NetworkService::Private::updatePropertyCache(const QString &name, const QVariant& value)
{
    if (m_propertiesCache.value(name) == value)
        return;

    m_propertiesCache.insert(name, value);

    if (name == Name) {
        queueSignal(SignalNameChanged);
    } else if (name == Error) {
        queueSignal(SignalErrorChanged);
    } else if (name == State) {
        updateState();
    } else if (name == Security) {
        queueSignal(SignalSecurityChanged);
        updateSecurityType();
    } else if (name == Strength) {
        queueSignal(SignalStrengthChanged);
    } else if (name == Favorite) {
        queueSignal(SignalFavoriteChanged);
    } else if (name == AutoConnect) {
        queueSignal(SignalAutoConnectChanged);
    } else if (name == Ipv4) {
        queueSignal(SignalIpv4Changed);
    } else if (name == Ipv4Config) {
        queueSignal(SignalIpv4ConfigChanged);
    } else if (name == Ipv6) {
        queueSignal(SignalIpv6Changed);
    } else if (name == Ipv6Config) {
        queueSignal(SignalIpv6ConfigChanged);
    } else if (name == Nameservers) {
        queueSignal(SignalNameserversChanged);
    } else if (name == NameserversConfig) {
        queueSignal(SignalNameserversConfigChanged);
    } else if (name == Domains) {
        queueSignal(SignalDomainsChanged);
    } else if (name == DomainsConfig) {
        queueSignal(SignalDomainsConfigChanged);
    } else if (name == Proxy) {
        queueSignal(SignalProxyChanged);
    } else if (name == ProxyConfig) {
        queueSignal(SignalProxyConfigChanged);
    } else if (name == Ethernet) {
        queueSignal(SignalEthernetChanged);
    } else if (name == Type) {
        queueSignal(SignalTypeChanged);
    } else if (name == Roaming) {
        queueSignal(SignalRoamingChanged);
    } else if (name == Timeservers) {
        queueSignal(SignalTimeserversChanged);
    } else if (name == TimeserversConfig) {
        queueSignal(SignalTimeserversConfigChanged);
    } else if (name == Bssid) {
        queueSignal(SignalBssidChanged);
    } else if (name == MaxRate) {
        queueSignal(SignalMaxRateChanged);
    } else if (name == Frequency) {
        queueSignal(SignalFrequencyChanged);
    } else if (name == EncryptionMode) {
        queueSignal(SignalEncryptionModeChanged);
    } else if (name == Hidden) {
        queueSignal(SignalHiddenChanged);
    } else if (name == Available) {
        // We need to signal both, see NetworkService::strength()
        queueSignal(SignalAvailableChanged);
        queueSignal(SignalStrengthChanged);
    } else if (name == Saved) {
        queueSignal(SignalSavedChanged);
    } else if (name == Access) {
        setPropertyAvailable(&PropAccess, true);
    } else if (name == DefaultAccess) {
        setPropertyAvailable(&PropDefaultAccess, true);
    } else if (name == Passphrase) {
        queueSignal(SignalPassphraseChanged);
        setPropertyAvailable(&PropPassphrase, true);
    } else if (name == Identity) {
        queueSignal(SignalIdentityChanged);
        setPropertyAvailable(&PropIdentity, true);
    } else if (name == EAP) {
        queueSignal(SignalEapMethodChanged);
        setPropertyAvailable(&PropEAP, true);
    }

    updateManaged();
}

#if HAVE_LIBDBUSACCESS
#include <dbusaccess_self.h>
#include <dbusaccess_policy.h>

// This is a local policy check which is still based on some assumptions
// like what's enabled or disabled by default. The real access rights are
// reported by the (asynchronous) CheckAccess call. However, if our guess
// turns out to be right, we can avoid a few unnecessary property change
// events when CheckAccess comes back with the accurate answer. It's not
// so much about optimization, mostly to avoid UI flickering (but should
// slightly optimize things too).
void NetworkService::Private::policyCheck(const QString &rules)
{
    static const DA_ACTION calls [] = {
        { "GetProperty",   CallGetProperty,   1 },
        { "get",           CallGetProperty,   1 },
        { "SetProperty",   CallSetProperty,   1 },
        { "set",           CallSetProperty,   1 },
        { "ClearProperty", CallClearProperty, 1 },
        { "Connect",       CallConnect,       0 },
        { "Disconnect",    CallDisconnect,    0 },
        { "Remove",        CallRemove,        0 },
        { "ResetCounters", CallResetCounters, 0 },
        { 0, 0, 0 }
    };
    DAPolicy *policy = da_policy_new_full(qPrintable(rules), calls);
    if (policy) {
        DASelf *self = da_self_new_shared();
        if (self) {
            int i;
            const DACred *cred = &self->cred;
            // Method calls (assume that they are enabled by default)
            for (i=0; calls[i].name; i++) {
                const uint id = calls[i].id;
                if (da_policy_check(policy, cred, id, "",
                    DA_ACCESS_ALLOW) == DA_ACCESS_ALLOW) {
                    m_callFlags |= id;
                } else {
                    m_callFlags &= ~id;
                }
            }
            // Properties (assume that they are disabled by default)
            for (uint i=0; i<COUNT(Properties); i++) {
                const PropertyAccessInfo *p = Properties[i];
                if (da_policy_check(policy, cred, CallGetProperty,
                    qPrintable(p->name), DA_ACCESS_DENY) == DA_ACCESS_ALLOW) {
                    m_propGetFlags |= p->flag;
                } else {
                    m_propGetFlags &= ~p->flag;
                }
                if (da_policy_check(policy, cred, CallSetProperty,
                    qPrintable(p->name), DA_ACCESS_DENY) == DA_ACCESS_ALLOW) {
                    m_propSetFlags |= p->flag;
                } else {
                    m_propSetFlags &= ~p->flag;
                }
            }
            da_self_unref(self);
        }
        da_policy_unref(policy);
    } else {
        DBG_("Failed to parse" << rules);
    }
}

#endif // HAVE_LIBDBUSACCESS

// ==========================================================================
// NetworkService
// ==========================================================================

NetworkService::NetworkService(const QString &path, const QVariantMap &properties, QObject *parent) :
    QObject(parent),
    m_priv(new Private(path, properties, this))
{
}

NetworkService::NetworkService(QObject *parent) :
    QObject(parent),
    m_priv(new Private(QString(), QVariantMap(), this))
{
}

NetworkService::~NetworkService()
{
}

QString NetworkService::name() const
{
    return m_priv->stringValue(Private::Name);
}

QString NetworkService::state() const
{
    return m_priv->state();
}

QString NetworkService::error() const
{
    return m_priv->stringValue(Private::Error);
}

QString NetworkService::type() const
{
    return m_priv->stringValue(Private::Type);
}

QStringList NetworkService::security() const
{
    return m_priv->stringListValue(Private::Security);
}

uint NetworkService::strength() const
{
    // connman is not reporting signal strength if network is unavailable
    return available() ? m_priv->uintValue(Private::Strength) : 0;
}

bool NetworkService::favorite() const
{
    return m_priv->boolValue(Private::Favorite);
}

bool NetworkService::autoConnect() const
{
    return m_priv->boolValue(Private::AutoConnect);
}

QString NetworkService::path() const
{
    return m_priv->m_path;
}

QVariantMap NetworkService::ipv4() const
{
    return m_priv->variantMapValue(Private::Ipv4);
}

QVariantMap NetworkService::ipv4Config() const
{
    return m_priv->variantMapValue(Private::Ipv4Config);
}

QVariantMap NetworkService::ipv6() const
{
    return m_priv->variantMapValue(Private::Ipv6);
}

QVariantMap NetworkService::ipv6Config() const
{
    return m_priv->variantMapValue(Private::Ipv6Config);
}

QStringList NetworkService::nameservers() const
{
    return m_priv->stringListValue(Private::Nameservers);
}

QStringList NetworkService::nameserversConfig() const
{
    return m_priv->stringListValue(Private::NameserversConfig);
}

QStringList NetworkService::domains() const
{
    return m_priv->stringListValue(Private::Domains);
}

QStringList NetworkService::domainsConfig() const
{
    return m_priv->stringListValue(Private::DomainsConfig);
}

QVariantMap NetworkService::proxy() const
{
    return m_priv->variantMapValue(Private::Proxy);
}

QVariantMap NetworkService::proxyConfig() const
{
    return m_priv->variantMapValue(Private::ProxyConfig);
}

QVariantMap NetworkService::ethernet() const
{
    return m_priv->variantMapValue(Private::Ethernet);
}

bool NetworkService::roaming() const
{
    return m_priv->boolValue(Private::Roaming);
}

bool NetworkService::hidden() const
{
    return m_priv->boolValue(Private::Hidden);
}

void NetworkService::requestConnect()
{
    if (m_priv->requestConnect()) {
        Q_EMIT serviceConnectionStarted();
    }
}

void NetworkService::requestDisconnect()
{
    Private::InterfaceProxy *service = m_priv->m_proxy;
    if (service) {
        Q_EMIT serviceDisconnectionStarted();
        service->Disconnect();
    }
}

void NetworkService::remove()
{
    Private::InterfaceProxy *service = m_priv->m_proxy;
    if (service) {
        service->Remove();
    }
}

void NetworkService::setAutoConnect(bool autoConnected)
{
    Private::InterfaceProxy *service = m_priv->m_proxy;
    if (service) {
        service->SetProperty(Private::AutoConnect, autoConnected);
    }
}

void NetworkService::setIpv4Config(const QVariantMap &ipv4)
{
    m_priv->setProperty(Private::Ipv4Config, ipv4);
}

void NetworkService::setIpv6Config(const QVariantMap &ipv6)
{
    m_priv->setProperty(Private::Ipv6Config, ipv6);
}

void NetworkService::setNameserversConfig(const QStringList &nameservers)
{
    m_priv->setProperty(Private::NameserversConfig, nameservers);
}

void NetworkService::setDomainsConfig(const QStringList &domains)
{
    m_priv->setProperty(Private::DomainsConfig, domains);
}

void NetworkService::setProxyConfig(const QVariantMap &proxy)
{
    m_priv->setProperty(Private::ProxyConfig, Private::adaptToConnmanProperties(proxy));
}

void NetworkService::resetCounters()
{
    if (m_priv->m_proxy) {
        m_priv->m_proxy->ResetCounters();
    }
}

void NetworkService::updateProperties(const QVariantMap &properties)
{
    m_priv->updateProperties(properties);
    m_priv->emitQueuedSignals();
}

void NetworkService::setPath(const QString &path)
{
    m_priv->setPath(path);
}

bool NetworkService::connected() const
{
    return m_priv->m_connected;
}

bool NetworkService::connecting() const
{
    return m_priv->m_connecting;
}

QString NetworkService::lastConnectError() const
{
    return m_priv->m_lastConnectError;
}

bool NetworkService::managed() const
{
    return m_priv->m_managed;
}

bool NetworkService::available() const
{
    return m_priv->boolValue(Private::Available, true);
}

bool NetworkService::saved() const
{
    return m_priv->boolValue(Private::Saved);
}

QStringList NetworkService::timeservers() const
{
    return m_priv->stringListValue(Private::Timeservers);
}

QStringList NetworkService::timeserversConfig() const
{
    return m_priv->stringListValue(Private::TimeserversConfig);
}

void NetworkService::setTimeserversConfig(const QStringList &servers)
{
    m_priv->setProperty(Private::TimeserversConfig, servers);
}

QString NetworkService::bssid() const
{
    return m_priv->stringValue(Private::Bssid);
}

quint32 NetworkService::maxRate() const
{
    return m_priv->uintValue(Private::MaxRate);
}

quint16 NetworkService::frequency() const
{
    return m_priv->uintValue(Private::Frequency);
}

QString NetworkService::encryptionMode() const
{
    return m_priv->stringValue(Private::EncryptionMode);
}

QString NetworkService::passphrase() const
{
    return m_priv->stringValue(Private::Passphrase);
}

void NetworkService::setPassphrase(QString passphrase)
{
    m_priv->setProperty(Private::Passphrase, passphrase);
}

bool NetworkService::passphraseAvailable() const
{
    return (m_priv->m_propGetFlags & Private::PropertyPassphrase) != 0;
}

QString NetworkService::identity() const
{
    return m_priv->stringValue(Private::Identity);
}

void NetworkService::setIdentity(QString identity)
{
    m_priv->setProperty(Private::Identity, identity);
}

bool NetworkService::identityAvailable() const
{
    return (m_priv->m_propGetFlags & Private::PropertyIdentity) != 0;
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
    return (m_priv->m_propGetFlags & Private::PropertyEAP) != 0;
}

#include "networkservice.moc"
