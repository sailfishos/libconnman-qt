/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2017 Jolla Ltd.
 * Contact: Slava Monich <slava.monich@jolla.com>
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0. The full text of the Apache License
 * is at http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QSettings>

#include "networkmanager.h"
#include "networkservice.h"
#include "libconnman_p.h"

#define COUNT(a) ((uint)(sizeof(a)/sizeof(a[0])))

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
    typedef void (NetworkService::*SignalEmitter)();

    static const QString EapMethodName[];
    static const QString SecurityTypeName[];

    static const QString Access;
    static const QString DefaultAccess;
    static const QString Identity;
    static const QString Passphrase;
    static const QString EAP;
    static const QString Available;
    static const QString Saved;

    static const QString PolicyPrefix;

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

    // The order and count of Signals must match the list of
    // signal emitters in emitQueuedSignals()
    enum Signal {
        NoSignal = -1,
        SignalNameChanged,
        SignalStateChanged,
        SignalErrorChanged,
        SignalSecurityChanged,
        SignalStrengthChanged,
        SignalFavoriteChanged,
        SignalAutoConnectChanged,
        SignalPathChanged,
        SignalIpv4Changed,
        SignalIpv4ConfigChanged,
        SignalIpv6Changed,
        SignalIpv6ConfigChanged,
        SignalNameserversChanged,
        SignalNameserversConfigChanged,
        SignalDomainsChanged,
        SignalDomainsConfigChanged,
        SignalProxyChanged,
        SignalProxyConfigChanged,
        SignalEthernetChanged,
        SignalTypeChanged,
        SignalRoamingChanged,
        SignalTimeserversChanged,
        SignalTimeserversConfigChanged,
        SignalConnectedChanged,
        SignalBssidChanged,
        SignalMaxRateChanged,
        SignalFrequencyChanged,
        SignalEncryptionModeChanged,
        SignalHiddenChanged,
        SignalManagedChanged,
        SignalPassphraseChanged,
        SignalPassphraseAvailableChanged,
        SignalIdentityChanged,
        SignalIdentityAvailableChanged,
        SignalSecurityTypeChanged,
        SignalEapMethodChanged,
        SignalEapMethodAvailableChanged,
        SignalAvailableChanged,
        SignalSavedChanged,
        SignalConnectingChanged,
        SignalLastConnectErrorChanged,
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

    static QVariantMap adaptToConnmanProperties(const QVariantMap& map);

    Private(NetworkService* parent);

    void deleteProxy();
    InterfaceProxy* createProxy(QString path);
    NetworkService* service();
    EapMethodMapRef eapMethodMap();
    EapMethod eapMethod();
    void updateSecurityType();
    void setEapMethod(EapMethod method);
    void setProperty(QString name, QVariant value);
    void setPropertyAvailable(const PropertyAccessInfo* prop, bool available);
    void setLastConnectError(QString error);
    void updateConnecting();
    void updateConnected();
    void updateState();
    void updateManaged();
    void queueSignal(Signal sig);
    void emitQueuedSignals();
    void dropQueuedSignals();

#if HAVE_LIBDBUSACCESS
    void policyCheck(QString rules);
#endif // HAVE_LIBDBUSACCESS

private:
    void updateConnecting(QString state);
    void updateConnected(QString state);
    void checkAccess();

private Q_SLOTS:
    void onRestrictedPropertyChanged(QString name);
    void onGetPropertyFinished(QDBusPendingCallWatcher* call);
    void onCheckAccessFinished(QDBusPendingCallWatcher* call);

private:
    quint64 m_queuedSignals;
    int m_firstQueuedSignal;

public:
    InterfaceProxy* m_proxy;
    QDBusPendingCallWatcher* m_connectWatcher;
    EapMethodMapRef m_eapMethodMapRef;
    SecurityType m_securityType;
    uint m_propGetFlags;
    uint m_propSetFlags;
    uint m_callFlags;
    bool m_managed;
    bool m_connecting;
    QString m_lastConnectError;
    QString m_state;
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
    QDBusPendingCall CheckAccess()
        { return asyncCall("CheckAccess"); }

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

const QString NetworkService::Private::Access("Access");
const QString NetworkService::Private::DefaultAccess("DefaultAccess");
const QString NetworkService::Private::Identity("Identity");
const QString NetworkService::Private::Passphrase("Passphrase");
const QString NetworkService::Private::EAP("EAP");
const QString NetworkService::Private::Available("Available");
const QString NetworkService::Private::Saved("Saved");

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

NetworkService::Private::Private(NetworkService* parent) :
    QObject(parent),
    m_queuedSignals(0),
    m_firstQueuedSignal(0),
    m_proxy(NULL),
    m_connectWatcher(NULL),
    m_securityType(SecurityNone),
    m_propGetFlags(PropertyEAP),
    m_propSetFlags(PropertyNone),
    m_callFlags(CallAll),
    m_managed(false),
    m_connecting(false)
{
    qRegisterMetaType<NetworkService *>();
}

inline void NetworkService::Private::dropQueuedSignals()
{
    m_queuedSignals = 0;
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
    // The order and count of signal emitters must match the Signal enum
    static const SignalEmitter emitSignal [] = {
        &NetworkService::nameChanged,
        &NetworkService::stateChanged,
        &NetworkService::errorChanged,
        &NetworkService::securityChanged,
        &NetworkService::strengthChanged,
        &NetworkService::favoriteChanged,
        &NetworkService::autoConnectChanged,
        &NetworkService::pathChanged,
        &NetworkService::ipv4Changed,
        &NetworkService::ipv4ConfigChanged,
        &NetworkService::ipv6Changed,
        &NetworkService::ipv6ConfigChanged,
        &NetworkService::nameserversChanged,
        &NetworkService::nameserversConfigChanged,
        &NetworkService::domainsChanged,
        &NetworkService::domainsConfigChanged,
        &NetworkService::proxyChanged,
        &NetworkService::proxyConfigChanged,
        &NetworkService::ethernetChanged,
        &NetworkService::typeChanged,
        &NetworkService::roamingChanged,
        &NetworkService::timeserversChanged,
        &NetworkService::timeserversConfigChanged,
        &NetworkService::connectedChanged,
        &NetworkService::bssidChanged,
        &NetworkService::maxRateChanged,
        &NetworkService::frequencyChanged,
        &NetworkService::encryptionModeChanged,
        &NetworkService::hiddenChanged,
        &NetworkService::managedChanged,
        &NetworkService::passphraseChanged,
        &NetworkService::passphraseAvailableChanged,
        &NetworkService::identityChanged,
        &NetworkService::identityAvailableChanged,
        &NetworkService::securityTypeChanged,
        &NetworkService::eapMethodChanged,
        &NetworkService::eapMethodAvailableChanged,
        &NetworkService::availableChanged,
        &NetworkService::savedChanged,
        &NetworkService::connectingChanged,
        &NetworkService::lastConnectErrorChanged
    };

    Q_STATIC_ASSERT(COUNT(emitSignal) == SignalCount);
    if (m_queuedSignals) {
        NetworkService *obj = service();
        for (int i = m_firstQueuedSignal; i < SignalCount && m_queuedSignals; i++) {
            const quint64 signalBit = (Q_UINT64_C(1) << i);
            if (m_queuedSignals & signalBit) {
                m_queuedSignals &= ~signalBit;
                Q_EMIT (obj->*(emitSignal[i]))();
            }
        }
    }
}

void NetworkService::Private::deleteProxy()
{
    delete m_proxy;
    m_proxy = 0;
}

NetworkService::Private::InterfaceProxy*
NetworkService::Private::createProxy(QString path)
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

void NetworkService::Private::onRestrictedPropertyChanged(QString name)
{
    DBG_(name);
    connect(new GetPropertyWatcher(name, m_proxy),
        SIGNAL(finished(QDBusPendingCallWatcher*)),
        SLOT(onGetPropertyFinished(QDBusPendingCallWatcher*)));
    if (name == Access) {
        checkAccess();
    }
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
        service()->updatePropertyCache(watcher->m_name, reply.value());
        emitQueuedSignals();
    }
}

void NetworkService::Private::onCheckAccessFinished(QDBusPendingCallWatcher* call)
{
    QDBusPendingReply<uint,uint,uint> reply = *call;
    NetworkService *obj = service();
    call->deleteLater();
    if (reply.isError()) {
        DBG_(obj->path() << reply.error());
    } else {
        const uint get_props = reply.argumentAt<0>();
        const uint set_props = reply.argumentAt<1>();
        const uint calls = reply.argumentAt<2>();

        DBG_(get_props << set_props << calls);

        const uint prev = m_propGetFlags;
        const bool wasManaged = obj->managed();
        m_propGetFlags = get_props;
        m_propSetFlags = set_props;
        m_callFlags = calls;

        for (uint i=0; i<COUNT(Properties); i++) {
            const PropertyAccessInfo* p = Properties[i];
            if ((m_propGetFlags & p->flag) != (prev & p->flag)) {
                queueSignal(p->sig);
            }
        }

        if (obj->managed() != wasManaged) {
            DBG_(obj->path() << "managed:" << obj->managed());
            queueSignal(SignalManagedChanged);
        }

        emitQueuedSignals();
    }
}

void NetworkService::Private::updateSecurityType()
{
    SecurityType type = SecurityUnknown;
    const QStringList security = service()->security();
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
        EapMethodMap* map = new EapMethodMap;
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
    if (method >= EapNone && method < COUNT(EapMethodName)) {
        setProperty(EAP, EapMethodName[method]);
    }
}

void NetworkService::Private::setProperty(QString name, QVariant value)
{
    if (m_proxy) {
        m_proxy->SetProperty(name, value);
    }
}

void NetworkService::Private::setLastConnectError(QString error)
{
    if (m_lastConnectError != error) {
        m_lastConnectError = error;
        queueSignal(SignalLastConnectErrorChanged);
    }
}

void NetworkService::Private::updateConnecting(QString state)
{
    bool connecting = m_connectWatcher || ConnmanState::connecting(state);
    if (m_connecting != connecting) {
        m_connecting = connecting;
        queueSignal(SignalConnectingChanged);
    }
}

void NetworkService::Private::updateConnecting()
{
    updateConnecting(service()->m_propertiesCache.value(NetworkService::State).toString());
}

void NetworkService::Private::updateConnected(QString state)
{
    bool connected = !m_connectWatcher && ConnmanState::connected(state);
    NetworkService *obj = service();
    if (obj->m_connected != connected) {
        obj->m_connected = connected;
        queueSignal(SignalConnectedChanged);
    }
}

void NetworkService::Private::updateConnected()
{
    updateConnected(service()->m_propertiesCache.value(NetworkService::State).toString());
}

void NetworkService::Private::updateState()
{
    NetworkService *obj = service();
    QString state(obj->m_propertiesCache.value(NetworkService::State).toString());
    if (m_state != state) {
        m_state = state;
        queueSignal(SignalStateChanged);
        updateConnecting(state);
        updateConnected(state);
    }
}

void NetworkService::Private::updateManaged()
{
    // This defines the criteria of being "managed"
    bool managed = !(m_callFlags & Private::CallRemove) && service()->saved();
    if (m_managed != managed) {
        m_managed = managed;
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

void NetworkService::Private::setPropertyAvailable(const PropertyAccessInfo* prop, bool available)
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
void NetworkService::Private::policyCheck(QString rules)
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
    DAPolicy* policy = da_policy_new_full(qPrintable(rules), calls);
    if (policy) {
        DASelf* self = da_self_new_shared();
        if (self) {
            int i;
            const DACred* cred = &self->cred;
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
                const PropertyAccessInfo* p = Properties[i];
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
const QString NetworkService::Timeservers("Timeservers");
const QString NetworkService::TimeserversConfig("Timeservers.Configuration");

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
    m_priv->updateSecurityType();

    // Make (reasonably) sure that NetworkManager is up to date when
    // we need to pull inputRequestTimeout out of it. By checking the
    // path for "/" we avoid infinite recursion - this path is passed
    // to some sort of special NetworkService created by NetworkManager
    // constructor itself.
    if (path != "/") {
        NetworkManagerFactory::createInstance();
    }

    // If the property is present in GetProperties output, it means that it's
    // at least gettable for us
    for (uint i=0; i<COUNT(Private::Properties); i++) {
        const Private::PropertyAccessInfo* prop = Private::Properties[i];
        if (m_propertiesCache.contains(prop->name)) {
            m_priv->m_propGetFlags |= prop->flag;
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
    QString access = m_propertiesCache.value(Private::Access).toString();
    if (access.isEmpty()) {
        access = m_propertiesCache.value(Private::DefaultAccess).toString();
    }
    if (access.startsWith(Private::PolicyPrefix)) {
        const int len = access.length()- Private::PolicyPrefix.length();
        m_priv->policyCheck(access.right(len));
    }
#endif // HAVE_LIBDBUSACCESS

    reconnectServiceInterface();
    DBG_(path << "managed:" << managed());
    m_priv->dropQueuedSignals();
}

NetworkService::NetworkService(QObject* parent)
    : QObject(parent),
      m_priv(new Private(this)),
      m_connected(false)
{
}

NetworkService::~NetworkService()
{
}

QString NetworkService::name() const
{
    return m_propertiesCache.value(Name).toString();
}

QString NetworkService::state() const
{
    return m_propertiesCache.value(State).toString();
}

QString NetworkService::error() const
{
    return m_propertiesCache.value(Error).toString();
}

QString NetworkService::type() const
{
    return m_propertiesCache.value(Type).toString();
}

QStringList NetworkService::security() const
{
    return m_propertiesCache.value(Security).toStringList();
}

uint NetworkService::strength() const
{
    // connman is not reporting signal strength if network is unavailable
    return available() ? m_propertiesCache.value(Strength).toUInt() : 0;
}

bool NetworkService::favorite() const
{
    return m_propertiesCache.value(Favorite).toBool();
}

bool NetworkService::autoConnect() const
{
    return m_propertiesCache.value(AutoConnect).toBool();
}

QString NetworkService::path() const
{
    return m_path;
}

QVariantMap NetworkService::ipv4() const
{
    if (m_propertiesCache.contains(IPv4)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv4));
    }
    return QVariantMap();
}

QVariantMap NetworkService::ipv4Config() const
{
    if (m_propertiesCache.contains(IPv4Config)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv4Config));
    }
    return QVariantMap();
}

QVariantMap NetworkService::ipv6() const
{
    if (m_propertiesCache.contains(IPv6)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv6));
    }
    return QVariantMap();
}

QVariantMap NetworkService::ipv6Config() const
{
    if (m_propertiesCache.contains(IPv6Config)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv6Config));
    }
    return QVariantMap();
}

QStringList NetworkService::nameservers() const
{
    return m_propertiesCache.value(Nameservers).toStringList();
}

QStringList NetworkService::nameserversConfig() const
{
    return m_propertiesCache.value(NameserversConfig).toStringList();
}

QStringList NetworkService::domains() const
{
    return m_propertiesCache.value(Domains).toStringList();
}

QStringList NetworkService::domainsConfig() const
{
    return m_propertiesCache.value(DomainsConfig).toStringList();
}

QVariantMap NetworkService::proxy() const
{
    if (m_propertiesCache.contains(Proxy)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(Proxy));
    }
    return QVariantMap();
}

QVariantMap NetworkService::proxyConfig() const
{
    if (m_propertiesCache.contains(ProxyConfig)) {
        return qdbus_cast<QVariantMap>(m_propertiesCache.value(ProxyConfig));
    }
    return QVariantMap();
}

QVariantMap NetworkService::ethernet() const
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
    if (service) {
        // If the service is in the failure state clear the Error property
        // so that we get notified of errors on subsequent connection attempts.
        if (state() == ConnmanState::Failure) {
            service->ClearProperty(Error);
        }

        int old_timeout = service->timeout();
        service->setTimeout(NetworkManager::instance()->inputRequestTimeout());
        QDBusPendingCall call = service->Connect();
        service->setTimeout(old_timeout);

        delete m_priv->m_connectWatcher;
        m_priv->m_connectWatcher = new QDBusPendingCallWatcher(call, service);

        m_priv->setLastConnectError(QString());
        m_priv->updateConnecting();
        m_priv->updateConnected();

        connect(m_priv->m_connectWatcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(handleConnectReply(QDBusPendingCallWatcher*)));

        m_priv->emitQueuedSignals();
        Q_EMIT serviceConnectionStarted();
    }
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
        service->Remove();
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
    QDBusPendingReply<> reply = *call;

    if (reply.isError()) {
        QDBusError error(reply.error());
        QString errorName(error.name());
        DBG_(error);

        // InProgress means that somebody has already asked this service
        // to get connected. That's fine, we will keep watching the status.
        m_priv->setLastConnectError((errorName == ConnmanError::InProgress) ?
            QString() : errorName);

        Q_EMIT connectRequestFailed(error.message());
    } else {
        // Reset the last error on success
        m_priv->setLastConnectError(QString());
    }

    m_priv->m_connectWatcher = NULL;
    m_priv->updateConnecting();
    m_priv->updateConnected();
    m_priv->emitQueuedSignals();
    call->deleteLater();
}

void NetworkService::resetProperties()
{
    QMutableMapIterator<QString, QVariant> i(m_propertiesCache);
    while (i.hasNext()) {
        i.next();

        const QString &key = i.key();
        const QVariant &value = i.value();
        i.remove();

        if (key == Name) {
            m_priv->queueSignal(Private::SignalNameChanged);
        } else if (key == Error) {
            m_priv->queueSignal(Private::SignalErrorChanged);
        } else if (key == State) {
            m_priv->updateState();
        } else if (key == Security) {
            m_priv->queueSignal(Private::SignalSecurityChanged);
            m_priv->updateSecurityType();
        } else if (key == Strength) {
            m_priv->queueSignal(Private::SignalStrengthChanged);
        } else if (key == Favorite) {
            if (value.toBool()) {
                m_priv->queueSignal(Private::SignalFavoriteChanged);
            }
        } else if (key == AutoConnect) {
            if (value.toBool()) {
                m_priv->queueSignal(Private::SignalAutoConnectChanged);
            }
        } else if (key == IPv4) {
            m_priv->queueSignal(Private::SignalIpv4Changed);
        } else if (key == IPv4Config) {
            m_priv->queueSignal(Private::SignalIpv4ConfigChanged);
        } else if (key == IPv6) {
            m_priv->queueSignal(Private::SignalIpv6Changed);
        } else if (key == IPv6Config) {
            m_priv->queueSignal(Private::SignalIpv6ConfigChanged);
        } else if (key == Nameservers) {
            m_priv->queueSignal(Private::SignalNameserversChanged);
        } else if (key == NameserversConfig) {
            m_priv->queueSignal(Private::SignalNameserversConfigChanged);
        } else if (key == Domains) {
            m_priv->queueSignal(Private::SignalDomainsChanged);
        } else if (key == DomainsConfig) {
            m_priv->queueSignal(Private::SignalDomainsConfigChanged);
        } else if (key == Proxy) {
            m_priv->queueSignal(Private::SignalProxyChanged);
        } else if (key == ProxyConfig) {
            m_priv->queueSignal(Private::SignalProxyConfigChanged);
        } else if (key == Ethernet) {
            m_priv->queueSignal(Private::SignalEthernetChanged);
        } else if (key == Type) {
            m_priv->queueSignal(Private::SignalTypeChanged);
        } else if (key == Roaming) {
            if (value.toBool()) {
                m_priv->queueSignal(Private::SignalRoamingChanged);
            }
        } else if (key == Timeservers) {
            m_priv->queueSignal(Private::SignalTimeserversChanged);
        } else if (key == TimeserversConfig) {
            m_priv->queueSignal(Private::SignalTimeserversConfigChanged);
        } else if (key == BSSID) {
            m_priv->queueSignal(Private::SignalBssidChanged);
        } else if (key == MaxRate) {
            m_priv->queueSignal(Private::SignalMaxRateChanged);
        } else if (key == Frequency) {
            m_priv->queueSignal(Private::SignalFrequencyChanged);
        } else if (key == EncryptionMode) {
            m_priv->queueSignal(Private::SignalEncryptionModeChanged);
        } else if (key == Hidden) {
            m_priv->queueSignal(Private::SignalHiddenChanged);
        } else if (key == Private::Available) {
            if (value.toBool()) {
                m_priv->queueSignal(Private::SignalAvailableChanged);
            }
        } else if (key == Private::Saved) {
            if (value.toBool()) {
                m_priv->queueSignal(Private::SignalSavedChanged);
            }
        } else if (key == Private::Access) {
            m_priv->setPropertyAvailable(&Private::PropAccess, false);
        } else if (key == Private::DefaultAccess) {
            m_priv->setPropertyAvailable(&Private::PropDefaultAccess, false);
        } else if (key == Private::Passphrase) {
            m_priv->queueSignal(Private::SignalPassphraseChanged);
            m_priv->setPropertyAvailable(&Private::PropPassphrase, false);
        } else if (key == Private::Identity) {
            m_priv->queueSignal(Private::SignalIdentityChanged);
            m_priv->setPropertyAvailable(&Private::PropIdentity, false);
        } else if (key == Private::EAP) {
            m_priv->queueSignal(Private::SignalEapMethodChanged);
            m_priv->setPropertyAvailable(&Private::PropEAP, false);
        }
    }
    m_priv->updateManaged();
    m_priv->emitQueuedSignals();
}

void NetworkService::reconnectServiceInterface()
{
    m_priv->deleteProxy();

    if (m_path.isEmpty())
        return;

    if (m_path == QStringLiteral("/")) {
        // This is a dummy invalidDefaultRoute created by NetworkManager
        QTimer::singleShot(500, this, SIGNAL(propertiesReady()));
    } else {
        Private::InterfaceProxy* service = m_priv->createProxy(m_path);
        connect(service, SIGNAL(PropertyChanged(QString,QDBusVariant)),
            SLOT(updateProperty(QString,QDBusVariant)));
        connect(new QDBusPendingCallWatcher(service->GetProperties(), service),
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            SLOT(getPropertiesFinished(QDBusPendingCallWatcher*)));
    }
}

void NetworkService::updatePropertyCache(const QString &name, const QVariant &value)
{
    if (m_propertiesCache.value(name) == value)
        return;

    m_propertiesCache.insert(name, value);

    if (name == Name) {
        m_priv->queueSignal(Private::SignalNameChanged);
    } else if (name == Error) {
        m_priv->queueSignal(Private::SignalErrorChanged);
    } else if (name == State) {
        m_priv->updateState();
    } else if (name == Security) {
        m_priv->queueSignal(Private::SignalSecurityChanged);
        m_priv->updateSecurityType();
    } else if (name == Strength) {
        m_priv->queueSignal(Private::SignalStrengthChanged);
    } else if (name == Favorite) {
        m_priv->queueSignal(Private::SignalFavoriteChanged);
    } else if (name == AutoConnect) {
        m_priv->queueSignal(Private::SignalAutoConnectChanged);
    } else if (name == IPv4) {
        m_priv->queueSignal(Private::SignalIpv4Changed);
    } else if (name == IPv4Config) {
        m_priv->queueSignal(Private::SignalIpv4ConfigChanged);
    } else if (name == IPv6) {
        m_priv->queueSignal(Private::SignalIpv6Changed);
    } else if (name == IPv6Config) {
        m_priv->queueSignal(Private::SignalIpv6ConfigChanged);
    } else if (name == Nameservers) {
        m_priv->queueSignal(Private::SignalNameserversChanged);
    } else if (name == NameserversConfig) {
        m_priv->queueSignal(Private::SignalNameserversConfigChanged);
    } else if (name == Domains) {
        m_priv->queueSignal(Private::SignalDomainsChanged);
    } else if (name == DomainsConfig) {
        m_priv->queueSignal(Private::SignalDomainsConfigChanged);
    } else if (name == Proxy) {
        m_priv->queueSignal(Private::SignalProxyChanged);
    } else if (name == ProxyConfig) {
        m_priv->queueSignal(Private::SignalProxyConfigChanged);
    } else if (name == Ethernet) {
        m_priv->queueSignal(Private::SignalEthernetChanged);
    } else if (name == Type) {
        m_priv->queueSignal(Private::SignalTypeChanged);
    } else if (name == Roaming) {
        m_priv->queueSignal(Private::SignalRoamingChanged);
    } else if (name == Timeservers) {
        m_priv->queueSignal(Private::SignalTimeserversChanged);
    } else if (name == TimeserversConfig) {
        m_priv->queueSignal(Private::SignalTimeserversConfigChanged);
    } else if (name == BSSID) {
        m_priv->queueSignal(Private::SignalBssidChanged);
    } else if (name == MaxRate) {
        m_priv->queueSignal(Private::SignalMaxRateChanged);
    } else if (name == Frequency) {
        m_priv->queueSignal(Private::SignalFrequencyChanged);
    } else if (name == EncryptionMode) {
        m_priv->queueSignal(Private::SignalEncryptionModeChanged);
    } else if (name == Hidden) {
        m_priv->queueSignal(Private::SignalHiddenChanged);
    } else if (name == Private::Available) {
        // We need to signal both, see NetworkService::strength()
        m_priv->queueSignal(Private::SignalAvailableChanged);
        m_priv->queueSignal(Private::SignalStrengthChanged);
    } else if (name == Private::Saved) {
        m_priv->queueSignal(Private::SignalSavedChanged);
    } else if (name == Private::Access) {
        m_priv->setPropertyAvailable(&Private::PropAccess, true);
    } else if (name == Private::DefaultAccess) {
        m_priv->setPropertyAvailable(&Private::PropDefaultAccess, true);
    } else if (name == Private::Passphrase) {
        m_priv->queueSignal(Private::SignalPassphraseChanged);
        m_priv->setPropertyAvailable(&Private::PropPassphrase, true);
    } else if (name == Private::Identity) {
        m_priv->queueSignal(Private::SignalIdentityChanged);
        m_priv->setPropertyAvailable(&Private::PropIdentity, true);
    } else if (name == Private::EAP) {
        m_priv->queueSignal(Private::SignalEapMethodChanged);
        m_priv->setPropertyAvailable(&Private::PropEAP, true);
    }

    m_priv->updateManaged();
}

void NetworkService::getPropertiesFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QVariantMap> reply = *call;
    call->deleteLater();

    if (!reply.isError()) {
        updateProperties(reply.value());
    } else {
        DBG_(m_path << reply.error());
    }
}

void NetworkService::updateProperty(const QString &name, const QDBusVariant &value)
{
    updatePropertyCache(name, value.variant());
    m_priv->emitQueuedSignals();
}

void NetworkService::updateProperties(const QVariantMap &properties)
{
    QVariantMap::const_iterator it = properties.constBegin(), end = properties.constEnd();
    for ( ; it != end; ++it) {
        updatePropertyCache(it.key(), it.value());
    }
    m_priv->emitQueuedSignals();
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
    return m_connected;
}

bool NetworkService::connected() const
{
    return m_connected;
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
    // This defines the criteria of being "managed"
    return !(m_priv->m_callFlags & Private::CallRemove) && saved();
}

bool NetworkService::available() const
{
    return m_propertiesCache.value(Private::Available, true).toBool();
}

bool NetworkService::saved() const
{
    return m_propertiesCache.value(Private::Saved).toBool();
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
    return (m_priv->m_propGetFlags & Private::PropertyPassphrase) != 0;
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
