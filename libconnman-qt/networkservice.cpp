/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2019 Jolla Ltd.
 * Copyright © 2025 Jolla Mobile Ltd
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0. The full text of the Apache License
 * is at http://www.apache.org/licenses/LICENSE-2.0
 */

#include "networkservice.h"
#include "networkmanager.h"
#include "commondbustypes.h"
#include "logging.h"

static QString ConnmanErrorInProgress = QStringLiteral("net.connman.Error.InProgress");

#define COUNT(a) ((uint)(sizeof(a)/sizeof(a[0])))

// connman = connman d-bus properties, class are local creations. arg depending whether signal includes the value.
#define NETWORK_SERVICE_PROPERTIES(ConnmanArg,ConnmanNoArg,ClassArg,ClassNoArg) \
    ClassArg(Path,path) \
    ClassArg(Connected,connected) \
    ClassArg(ServiceState,serviceState) \
    ClassNoArg(Connecting,connecting) \
    ClassNoArg(Managed,managed) \
    ClassNoArg(SecurityType,securityType) \
    ClassNoArg(EapMethod,eapMethod) \
    ClassNoArg(PeapVersion,peapVersion) \
    ClassNoArg(PassphraseAvailable,passphraseAvailable) \
    ClassNoArg(IdentityAvailable,identityAvailable) \
    ClassNoArg(EapMethodAvailable,eapMethodAvailable) \
    ClassNoArg(Phase2Available,phase2Available) \
    ClassNoArg(PrivateKeyAvailable,privateKeyAvailable) \
    ClassNoArg(PrivateKeyFileAvailable,privateKeyFileAvailable) \
    ClassNoArg(PrivateKeyPassphraseAvailable,privateKeyPassphraseAvailable) \
    ClassNoArg(CACertAvailable,caCertAvailable) \
    ClassNoArg(CACertFileAvailable,caCertFileAvailable) \
    ClassNoArg(DomainSuffixMatchAvailable,domainSuffixMatchAvailable) \
    ClassNoArg(AnonymousIdentityAvailable,anonymousIdentityAvailable) \
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
    ConnmanArg("Phase2",Phase2,phase2) \
    ConnmanArg("Passphrase",Passphrase,passphrase) \
    ConnmanArg("Identity",Identity,identity) \
    ConnmanArg("CACert",CACert,caCert) \
    ConnmanArg("CACertFile",CACertFile,caCertFile) \
    ConnmanArg("DomainSuffixMatch",DomainSuffixMatch,domainSuffixMatch) \
    ConnmanArg("ClientCert",ClientCert,clientCert) \
    ConnmanArg("ClientCertFile",ClientCertFile,clientCertFile) \
    ConnmanArg("PrivateKey",PrivateKey,privateKey) \
    ConnmanArg("PrivateKeyFile",PrivateKeyFile,privateKeyFile) \
    ConnmanArg("PrivateKeyPassphrase",PrivateKeyPassphrase,privateKeyPassphrase) \
    ConnmanArg("AnonymousIdentity",AnonymousIdentity,anonymousIdentity) \
    ConnmanNoArg("Available",Available,available) \
    ConnmanNoArg("Saved",Saved,saved) \
    ClassNoArg(Valid,valid) \
    ConnmanArg("mDNS", MDNS, mDNS) \
    ConnmanArg("mDNS.Configuration", MDNSConfiguration, mDNSConfiguration)

#define NETWORK_SERVICE_PROPERTIES2(Connman,Class) \
    NETWORK_SERVICE_PROPERTIES(Connman,Connman,Class,Class)

#define IGNORE(X,x)

class NetworkService::Private: public QObject
{
    Q_OBJECT

public:
    class InterfaceProxy;
    class GetPropertyWatcher;
    typedef QHash<QString,QPair<EapMethod,int> > EapMethodMap;
    typedef QSharedPointer<EapMethodMap> EapMethodMapRef;
    typedef void (NetworkService::Private::*SignalEmitter)(NetworkService*);

    static const QString EapMethodName[];
    static const QString PeapMethodName[];
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
        PropertyPhase2        = 0x00000020,
        PropertyPrivateKey    = 0x00000040,
        PropertyPrivateKeyFile= 0x00000080,
        PropertyPrivateKeyPassphrase    = 0x00000100,
        PropertyCACert        = 0x00000200,
        PropertyCACertFile    = 0x00000400,
        PropertyDomainSuffixMatch = 0x00000800,
        PropertyAnonymousIdentity = 0x00001000,
        PropertyAll           = 0x00001fff
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
        CallMoveAfter         = 0x00000100,
        CallMoveBefore        = 0x00000200,
        CallAll               = 0x00000fff
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
    static const PropertyAccessInfo PropPhase2;
    static const PropertyAccessInfo PropPrivateKey;
    static const PropertyAccessInfo PropPrivateKeyFile;
    static const PropertyAccessInfo PropPrivateKeyPassphrase;
    static const PropertyAccessInfo PropCACert;
    static const PropertyAccessInfo PropCACertFile;
    static const PropertyAccessInfo PropDomainSuffixMatch;
    static const PropertyAccessInfo PropAnonymousIdentity;

    static QVariantMap adaptToConnmanProperties(const QVariantMap &map);

    Private(const QString &path, const QVariantMap &properties, NetworkService *parent);

    void init();

    void deleteProxy();
    void setPath(const QString &path);
    InterfaceProxy* createProxy(const QString &path);
    NetworkService* service();
    EapMethodMapRef eapMethodMap();
    EapMethod eapMethod();
    int peapVersion();
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
    void setPeapVersion(int version);
    void setProperty(const QString &name, const QVariant &value);
    void setPropertyAvailable(const PropertyAccessInfo *prop, bool available);
    void setLastConnectError(const QString &error);
    void updateProperties(QVariantMap properties);
    void updateState();
    void updateManaged();
    void queueSignal(Signal sig);
    void emitQueuedSignals();
    void remove();

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
    bool m_valid;
    NetworkService::ServiceState m_serviceState;
    QString m_path;
    QVariantMap m_propertiesCache;
    InterfaceProxy *m_proxy;
    QPointer<QDBusPendingCallWatcher> m_connectWatcher;
    EapMethodMapRef m_eapMethodMapRef;
    SecurityType m_securityType;
    uint m_propGetFlags;
    uint m_propSetFlags;
    uint m_callFlags;
    bool m_managed;
    QString m_lastConnectError;
    int m_peapVersion;
    QSharedPointer<NetworkManager> m_networkManager;
    bool m_emitting = false;

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
            QDBusConnection::systemBus(), parent) {}

public Q_SLOTS:
    QDBusPendingCall GetProperties()
        { return asyncCall("GetProperties"); }
    QDBusPendingCall GetProperty(const QString &name)
        { return asyncCall("GetProperty", name); }
    QDBusPendingCall SetProperty(const QString &name, QVariant value)
        { return asyncCall("SetProperty", name, QVariant::fromValue(QDBusVariant(value))); }
    QDBusPendingCall ClearProperty(const QString &name)
        { return asyncCall("ClearProperty", name); }
    QDBusPendingCall Connect()
        { return asyncCall("Connect"); }
    QDBusPendingCall Disconnect()
        { return asyncCall("Disconnect"); }
    QDBusPendingCall Remove()
        { return asyncCall("Remove"); }
    QDBusPendingCall MoveBefore(const QDBusObjectPath &service)
        { return asyncCall("MoveBefore", QVariant::fromValue(service)); }
    QDBusPendingCall MoveAfter(const QDBusObjectPath &service)
        { return asyncCall("MoveAfter", QVariant::fromValue(service)); }
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
const NetworkService::Private::PropertyAccessInfo NetworkService::Private::PropPhase2 =
    { NetworkService::Private::Phase2, NetworkService::Private::PropertyPhase2,
      NetworkService::Private::SignalPhase2AvailableChanged };
const NetworkService::Private::PropertyAccessInfo NetworkService::Private::PropPrivateKey =
    { NetworkService::Private::PrivateKey, NetworkService::Private::PropertyPrivateKey,
      NetworkService::Private::SignalPrivateKeyAvailableChanged };
const NetworkService::Private::PropertyAccessInfo NetworkService::Private::PropPrivateKeyFile =
    { NetworkService::Private::PrivateKeyFile, NetworkService::Private::PropertyPrivateKeyFile,
      NetworkService::Private::SignalPrivateKeyFileAvailableChanged };
const NetworkService::Private::PropertyAccessInfo NetworkService::Private::PropPrivateKeyPassphrase =
    { NetworkService::Private::PrivateKeyPassphrase, NetworkService::Private::PropertyPrivateKeyPassphrase,
      NetworkService::Private::SignalPrivateKeyPassphraseAvailableChanged };
const NetworkService::Private::PropertyAccessInfo NetworkService::Private::PropCACert =
    { NetworkService::Private::CACert, NetworkService::Private::PropertyCACert,
      NetworkService::Private::SignalCACertAvailableChanged };
const NetworkService::Private::PropertyAccessInfo NetworkService::Private::PropCACertFile =
    { NetworkService::Private::CACertFile, NetworkService::Private::PropertyCACertFile,
      NetworkService::Private::SignalCACertFileAvailableChanged };
const NetworkService::Private::PropertyAccessInfo NetworkService::Private::PropDomainSuffixMatch =
    { NetworkService::Private::DomainSuffixMatch, NetworkService::Private::PropertyDomainSuffixMatch,
      NetworkService::Private::SignalDomainSuffixMatchAvailableChanged };
const NetworkService::Private::PropertyAccessInfo NetworkService::Private::PropAnonymousIdentity =
    { NetworkService::Private::AnonymousIdentity, NetworkService::Private::PropertyAnonymousIdentity,
      NetworkService::Private::SignalAnonymousIdentityChanged };

const NetworkService::Private::PropertyAccessInfo* NetworkService::Private::Properties[] = {
    &NetworkService::Private::PropAccess,
    &NetworkService::Private::PropDefaultAccess,
    &NetworkService::Private::PropIdentity,
    &NetworkService::Private::PropPassphrase,
    &NetworkService::Private::PropEAP,
    &NetworkService::Private::PropPhase2,
    &NetworkService::Private::PropPrivateKey,
    &NetworkService::Private::PropPrivateKeyFile,
    &NetworkService::Private::PropPrivateKeyPassphrase,
    &NetworkService::Private::PropCACert,
    &NetworkService::Private::PropCACertFile,
    &NetworkService::Private::PropDomainSuffixMatch,
    &NetworkService::Private::PropAnonymousIdentity,
};

// The order must match EapMethod enum
const QString NetworkService::Private::EapMethodName[] = {
    QString(), "peap", "ttls", "tls"
};

// Special versions of peap method names
const QString NetworkService::Private::PeapMethodName[] = {
    "PEAPv0", "PEAPv1"
};

// The order must match SecurityType enum
const QString NetworkService::Private::SecurityTypeName[] = {
    QString(), "none", "wep", "psk", "ieee8021x"
};

NetworkService::Private::Private(const QString &path, const QVariantMap &props, NetworkService *parent) :
    QObject(parent),
    m_valid(!props.isEmpty()),
    m_serviceState(NetworkService::UnknownState),
    m_path(path),
    m_propertiesCache(props),
    m_proxy(NULL),
    m_securityType(SecurityNone),
    m_propGetFlags(PropertyEAP),
    m_propSetFlags(PropertyNone),
    m_callFlags(CallAll),
    m_managed(false),
    m_peapVersion(-1),
    m_queuedSignals(0),
    m_firstQueuedSignal(0)
{
}

void NetworkService::Private::init()
{
    qRegisterMetaType<NetworkService *>();
    updateSecurityType();

    // Make (reasonably) sure that NetworkManager is up to date when
    // we need to pull inputRequestTimeout out of it. By checking the
    // path for "/" we avoid infinite recursion - this path is passed
    // to some sort of special NetworkService created by NetworkManager
    // constructor itself.
    if (m_path != "/") {
        m_networkManager = NetworkManager::sharedInstance();
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
    updateState();
    qCDebug(lcConnman) << m_path << "managed:" << m_managed;

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
        m_emitting = true;
        for (int i = m_firstQueuedSignal; i < SignalCount && m_queuedSignals; i++) {
            const quint64 signalBit = (Q_UINT64_C(1) << i);
            if (m_queuedSignals & signalBit) {
                m_queuedSignals &= ~signalBit;
                Q_EMIT (this->*(emitSignal[i]))(obj);
            }
        }
        m_emitting = false;
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
    return static_cast<NetworkService*>(parent());
}

void NetworkService::Private::checkAccess()
{
    auto *pendingCheck = new QDBusPendingCallWatcher(m_proxy->CheckAccess(), m_proxy);
    connect(pendingCheck, &QDBusPendingCallWatcher::finished,
            this, &NetworkService::Private::onCheckAccessFinished);
}

void NetworkService::Private::onRestrictedPropertyChanged(const QString &name)
{
    qCDebug(lcConnman) << name;
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
        qCDebug(lcConnman) << watcher->m_name << reply.error();
    } else {
        qCDebug(lcConnman) << watcher->m_name << "=" << reply.value();
        updatePropertyCache(watcher->m_name, reply.value());
        emitQueuedSignals();
    }
}

void NetworkService::Private::onCheckAccessFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<uint,uint,uint> reply = *call;
    call->deleteLater();
    if (reply.isError()) {
        qCDebug(lcConnman) << m_path << reply.error();
    } else {
        const uint get_props = reply.argumentAt<0>();
        const uint set_props = reply.argumentAt<1>();
        const uint calls = reply.argumentAt<2>();

        qCDebug(lcConnman) << get_props << set_props << calls;

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
            qCDebug(lcConnman) << m_path << "managed:" << m_managed;
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
            map->insert(name.toLower(), QPair<EapMethod, int>((EapMethod)i, -1));
            map->insert(name.toUpper(), QPair<EapMethod, int>((EapMethod)i, -1));
        }
        for (uint i=0; i<COUNT(PeapMethodName); i++) {
            const QString name = PeapMethodName[i];
            map->insert(name, QPair<EapMethod, int>(EapPEAP, i));
            map->insert(name.toLower(), QPair<EapMethod, int>(EapPEAP, i));
            map->insert(name.toUpper(), QPair<EapMethod, int>(EapPEAP, i));
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
        return eapMethodMap()->value(eap, QPair<EapMethod, int>(EapNone, -1)).first;
    }
}

void NetworkService::Private::setEapMethod(EapMethod method)
{
    if (method == EapPEAP && m_peapVersion != -1) {
        setProperty(EAP, PeapMethodName[m_peapVersion]);
    } else if (method >= EapNone && method < COUNT(EapMethodName)) {
        setProperty(EAP, EapMethodName[method]);
        m_peapVersion = -1;
    }
}

int NetworkService::Private::peapVersion()
{
    QString eap = stringValue(EAP);
    if (m_peapVersion != -1) {
        return m_peapVersion;
    } else if (eap.isEmpty()) {
        return -1;
    } else {
        return eapMethodMap()->value(eap, QPair<EapMethod, int>(EapNone, -1)).second;
    }
}

void NetworkService::Private::setPeapVersion(int version)
{
    if (version >= 0 && static_cast<uint>(version) > COUNT(PeapMethodName))
        return;
    if (version < 0)
        version = -1;
    if (eapMethod() != EapPEAP) {
        m_peapVersion = version;
    } else if (version < 0) {
        setProperty(EAP, EapMethodName[EapPEAP]);
    } else {
        setProperty(EAP, PeapMethodName[version].toLower());
        m_peapVersion = -1;
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

static NetworkService::ServiceState stateStringToEnum(const QString &state)
{
    if (state == QStringLiteral("idle")) {
        return NetworkService::IdleState;
    } else if (state == QStringLiteral("failure")) {
        return NetworkService::FailureState;
    } else if (state == QStringLiteral("association")) {
        return NetworkService::AssociationState;
    } else if (state == QStringLiteral("configuration")) {
        return NetworkService::ConfigurationState;
    } else if (state == QStringLiteral("ready")) {
        return NetworkService::ReadyState;
    } else if (state == QStringLiteral("disconnect")) {
        return NetworkService::DisconnectState;
    } else if (state == QStringLiteral("online")) {
        return NetworkService::OnlineState;
    }

    return NetworkService::UnknownState;
}

void NetworkService::Private::updateState()
{
    QString currentState = state();
    NetworkService::ServiceState newState = stateStringToEnum(currentState);

    if (m_serviceState == newState) {
        return;
    }

    bool wasConnecting = service()->connecting();
    bool wasConnected = service()->connected();

    m_serviceState = newState;

    queueSignal(SignalStateChanged);
    queueSignal(SignalServiceStateChanged);

    if (service()->connecting() != wasConnecting) {
        queueSignal(SignalConnectingChanged);
    }

    if (service()->connected() != wasConnected) {
        queueSignal(SignalConnectedChanged);
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
    for (const QString &key : map.keys()) {
        if (map.value(key).type() == QVariant::List) {
            QStringList strList;
            for (const QVariant &value : map.value(key).toList()) {
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
        connect(service, SIGNAL(RestrictedPropertyChanged(QString)),
            SLOT(onRestrictedPropertyChanged(QString)));
        auto *pendingProperties = new QDBusPendingCallWatcher(service->GetProperties(), service);
        connect(pendingProperties, &QDBusPendingCallWatcher::finished,
                this, &NetworkService::Private::onGetPropertiesFinished);
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
        qCDebug(lcConnman) << m_path << reply.error();
    }
}

bool NetworkService::Private::requestConnect()
{
    if (m_proxy) {
        // If the service is in the failure state clear the Error property
        // so that we get notified of errors on subsequent connection attempts.
        if (service()->serviceState() == NetworkService::FailureState) {
            m_proxy->ClearProperty(Error);
        }

        const int old_timeout = m_proxy->timeout();
        if (m_networkManager) {
            m_proxy->setTimeout(m_networkManager->inputRequestTimeout());
        }

        QDBusPendingCall call = m_proxy->Connect();

        if (m_networkManager) {
            m_proxy->setTimeout(old_timeout);
        }

        bool wasConnecting = service()->connecting();
        bool wasConnected = service()->connected();

        delete m_connectWatcher.data();
        m_connectWatcher = new QDBusPendingCallWatcher(call, m_proxy);

        setLastConnectError(QString());

        if (service()->connecting() != wasConnecting) {
            queueSignal(SignalConnectingChanged);
        }

        if (service()->connected() != wasConnected) {
            queueSignal(SignalConnectedChanged);
        }

        connect(m_connectWatcher.data(), &QDBusPendingCallWatcher::finished,
                this, &NetworkService::Private::onConnectFinished);

        emitQueuedSignals();
        return true;
    } else {
        return false;
    }
}

void NetworkService::Private::onConnectFinished(QDBusPendingCallWatcher *call)
{
    bool wasConnecting = service()->connecting();
    bool wasConnected = service()->connected();

    QDBusPendingReply<> reply = *call;
    m_connectWatcher.clear();
    call->deleteLater();

    if (reply.isError()) {
        QDBusError error(reply.error());
        QString errorName(error.name());
        qCDebug(lcConnman) << error;

        // InProgress means that somebody has already asked this service
        // to get connected. That's fine, we will keep watching the status.
        setLastConnectError(errorName == ConnmanErrorInProgress ? QString() : errorName);
        Q_EMIT service()->connectRequestFailed(error.message());
    } else {
        // Reset the last error on success
        setLastConnectError(QString());
    }

    if (service()->connecting() != wasConnecting) {
        queueSignal(SignalConnectingChanged);
    }

    if (service()->connected() != wasConnected) {
        queueSignal(SignalConnectedChanged);
    }

    emitQueuedSignals();
}

void NetworkService::Private::updateProperties(QVariantMap properties)
{
    QMapIterator<QString, QVariant> it(properties);
    while (it.hasNext()) {
        it.next();
        updatePropertyCache(it.key(), it.value());
    }
    if (!m_valid) {
        m_valid = true;
        queueSignal(SignalValidChanged);
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
            queueSignal(SignalPeapVersionChanged);
            setPropertyAvailable(&PropEAP, false);
        } else if (key == Phase2) {
            queueSignal(SignalPhase2Changed);
            setPropertyAvailable(&PropPhase2, false);
        } else if (key == PrivateKeyPassphrase) {
            queueSignal(SignalPrivateKeyPassphraseChanged);
            setPropertyAvailable(&PropPrivateKeyPassphrase, false);
        } else if (key == CACert) {
            queueSignal(SignalCACertChanged);
            setPropertyAvailable(&PropCACert, false);
        } else if (key == CACertFile) {
            queueSignal(SignalCACertFileChanged);
            setPropertyAvailable(&PropCACertFile, false);
        } else if (key == DomainSuffixMatch) {
            queueSignal(SignalDomainSuffixMatchChanged);
            setPropertyAvailable(&PropDomainSuffixMatch, false);
        } else if (key == ClientCert) {
            queueSignal(SignalClientCertChanged);
        } else if (key == ClientCertFile) {
            queueSignal(SignalClientCertFileChanged);
        } else if (key == PrivateKey) {
            queueSignal(SignalPrivateKeyChanged);
            setPropertyAvailable(&PropPrivateKey, false);
        } else if (key == PrivateKeyFile) {
            queueSignal(SignalPrivateKeyFileChanged);
            setPropertyAvailable(&PropPrivateKeyFile, false);
        } else if (key == AnonymousIdentity) {
            queueSignal(SignalAnonymousIdentityChanged);
            setPropertyAvailable(&PropAnonymousIdentity, false);
        } else if (key == MDNS) {
            if (value.toBool())
                queueSignal(SignalMDNSChanged);
        } else if (key == MDNSConfiguration) {
            if (value.toBool())
                queueSignal(SignalMDNSConfigurationChanged);
        }
    }
    updateManaged();
    if (m_valid) {
        m_valid = false;
        queueSignal(SignalValidChanged);
    }
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
        queueSignal(SignalPeapVersionChanged);
        setPropertyAvailable(&PropEAP, true);
    } else if (name == Phase2) {
        queueSignal(SignalPhase2Changed);
        setPropertyAvailable(&PropPhase2, true);
    } else if (name == CACert) {
        queueSignal(SignalCACertChanged);
        setPropertyAvailable(&PropCACert, true);
    } else if (name == CACertFile) {
        queueSignal(SignalCACertFileChanged);
        setPropertyAvailable(&PropCACertFile, true);
    } else if (name == ClientCert) {
        queueSignal(SignalClientCertChanged);
    } else if (name == ClientCertFile) {
        queueSignal(SignalClientCertFileChanged);
    } else if (name == DomainSuffixMatch) {
        queueSignal(SignalDomainSuffixMatchChanged);
        setPropertyAvailable(&PropDomainSuffixMatch, true);
    } else if (name == PrivateKey) {
        queueSignal(SignalPrivateKeyChanged);
        setPropertyAvailable(&PropPrivateKey, true);
    } else if (name == PrivateKeyFile) {
        queueSignal(SignalPrivateKeyFileChanged);
        setPropertyAvailable(&PropPrivateKeyFile, true);
    } else if (name == PrivateKeyPassphrase) {
        queueSignal(SignalPrivateKeyPassphraseChanged);
        setPropertyAvailable(&PropPrivateKeyPassphrase, true);
    } else if (name == AnonymousIdentity) {
        queueSignal(SignalAnonymousIdentityChanged);
        setPropertyAvailable(&PropAnonymousIdentity, true);
    } else if (name == MDNS) {
        queueSignal(SignalMDNSChanged);
    } else if (name == MDNSConfiguration) {
    	queueSignal(SignalMDNSConfigurationChanged);
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
        { "MoveAfter",     CallMoveAfter,     0 },
        { "MoveBefore",    CallMoveBefore,    0 },
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
        qCDebug(lcConnman) << "Failed to parse" << rules;
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
    m_priv->init();
}

NetworkService::NetworkService(QObject *parent) :
    QObject(parent),
    m_priv(new Private(QString(), QVariantMap(), this))
{
    m_priv->init();
}

NetworkService::~NetworkService()
{
}

QString NetworkService::name() const
{
    return m_priv->stringValue(Private::Name);
}

// deprecated
QString NetworkService::state() const
{
    static bool warned = false;
    if (!warned && !m_priv->m_emitting) {
        qWarning() << "NetworkService::state() is deprecated. Use serviceState() or matching property";
        warned = true;
    }

    return m_priv->state();
}

NetworkService::ServiceState NetworkService::serviceState() const
{
    return m_priv->m_serviceState;
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

void NetworkService::moveBefore(const QString &service)
{
    if (m_priv->m_proxy) {
        m_priv->m_proxy->MoveBefore(QDBusObjectPath(service));
    }
}

void NetworkService::moveAfter(const QString &service)
{
    if (m_priv->m_proxy) {
        m_priv->m_proxy->MoveAfter(QDBusObjectPath(service));
    }
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
    m_priv->remove();
}

void NetworkService::Private::remove()
{
    if (m_proxy) {
        m_proxy->Remove();
        updatePropertyCache(Private::Saved, false);
        emitQueuedSignals();
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

bool NetworkService::isValid() const
{
    return m_priv->m_valid;
}

bool NetworkService::connected() const
{
    if (m_priv->m_connectWatcher) {
        return false;
    }

    ServiceState state = serviceState();
    return state == OnlineState || state == ReadyState;
}

bool NetworkService::connecting() const
{
    if (m_priv->m_connectWatcher) {
        return true;
    }
    ServiceState state = serviceState();
    return state == AssociationState || state == ConfigurationState;
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

QString NetworkService::privateKeyPassphrase() const
{
    return m_priv->stringValue(Private::PrivateKeyPassphrase);
}

void NetworkService::setPrivateKeyPassphrase(const QString &passphrase)
{
    m_priv->setProperty(Private::PrivateKeyPassphrase, passphrase);
}

bool NetworkService::privateKeyPassphraseAvailable() const
{
    return (m_priv->m_propGetFlags & Private::PropertyPrivateKeyPassphrase) != 0;
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

int NetworkService::peapVersion() const
{
    return m_priv->peapVersion();
}

void NetworkService::setPeapVersion(int version)
{
    m_priv->setPeapVersion(version);
}

bool NetworkService::eapMethodAvailable() const
{
    return (m_priv->m_propGetFlags & Private::PropertyEAP) != 0;
}

QString NetworkService::phase2() const
{
    return m_priv->stringValue(Private::Phase2);
}

void NetworkService::setPhase2(const QString &phase2)
{
    m_priv->setProperty(Private::Phase2, phase2);
}

bool NetworkService::phase2Available() const
{
    return (m_priv->m_propGetFlags & Private::PropertyPhase2) != 0;
}

QString NetworkService::caCert() const
{
    return m_priv->stringValue(Private::CACert);
}

void NetworkService::setCACert(const QString &caCert)
{
    m_priv->setProperty(Private::CACert, caCert);
}

bool NetworkService::caCertAvailable() const
{
    return (m_priv->m_propGetFlags & Private::PropertyCACert) != 0;
}

QString NetworkService::caCertFile() const
{
    return m_priv->stringValue(Private::CACertFile);
}

void NetworkService::setCACertFile(const QString &caCertFile)
{
    m_priv->setProperty(Private::CACertFile, caCertFile);
}

bool NetworkService::caCertFileAvailable() const
{
    return (m_priv->m_propGetFlags & Private::PropertyCACertFile) != 0;
}

QString NetworkService::domainSuffixMatch() const
{
    return m_priv->stringValue(Private::DomainSuffixMatch);
}

void NetworkService::setDomainSuffixMatch(const QString &domainSuffixMatch)
{
    m_priv->setProperty(Private::DomainSuffixMatch, domainSuffixMatch);
}

bool NetworkService::domainSuffixMatchAvailable() const
{
    return (m_priv->m_propGetFlags & Private::PropertyDomainSuffixMatch) != 0;
}

bool NetworkService::anonymousIdentityAvailable() const
{
    return (m_priv->m_propGetFlags & Private::PropertyAnonymousIdentity) != 0;
}

QString NetworkService::clientCert() const
{
    return m_priv->stringValue(Private::ClientCert);
}

void NetworkService::setClientCert(const QString &clientCert)
{
    m_priv->setProperty(Private::ClientCert, clientCert);
}

QString NetworkService::clientCertFile() const
{
    return m_priv->stringValue(Private::ClientCertFile);
}

void NetworkService::setClientCertFile(const QString &clientCertFile)
{
    m_priv->setProperty(Private::ClientCertFile, clientCertFile);
}

QString NetworkService::privateKey() const
{
    return m_priv->stringValue(Private::PrivateKey);
}

void NetworkService::setPrivateKey(const QString &privateKey)
{
    m_priv->setProperty(Private::PrivateKey, privateKey);
}

bool NetworkService::privateKeyAvailable() const
{
    return (m_priv->m_propGetFlags & Private::PropertyPrivateKey) != 0;
}

QString NetworkService::privateKeyFile() const
{
    return m_priv->stringValue(Private::PrivateKeyFile);
}

void NetworkService::setPrivateKeyFile(const QString &privateKeyFile)
{
    m_priv->setProperty(Private::PrivateKeyFile, privateKeyFile);
}

bool NetworkService::privateKeyFileAvailable() const
{
    return (m_priv->m_propGetFlags & Private::PropertyPrivateKeyFile) != 0;
}

QString NetworkService::anonymousIdentity() const
{
    return m_priv->stringValue(Private::AnonymousIdentity);
}

void NetworkService::setAnonymousIdentity(const QString &anonymousIdentity)
{
    m_priv->setProperty(Private::AnonymousIdentity, anonymousIdentity);
}

bool NetworkService::mDNS() const
{
    return m_priv->boolValue(Private::MDNS);
}

bool NetworkService::mDNSConfiguration() const
{
    return m_priv->boolValue(Private::MDNSConfiguration);
}

void NetworkService::setmDNSConfiguration(bool mDNSConfiguration)
{
    Private::InterfaceProxy *service = m_priv->m_proxy;
    if (service) {
        service->SetProperty(Private::MDNSConfiguration, mDNSConfiguration);
    }
}

#include "networkservice.moc"
