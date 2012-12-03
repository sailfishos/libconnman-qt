/*
 * Copyright © 2010, Intel Corporation.
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "service.h"
#include "networkservice.h"
#include "debug.h"

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

NetworkService::NetworkService(const QString &path, const QVariantMap &properties, QObject* parent)
  : QObject(parent),
    m_service(NULL)
{
    Q_ASSERT(!path.isEmpty());
    m_service = new Service("net.connman", path, QDBusConnection::systemBus(), this);

    if (!m_service->isValid()) {
        pr_dbg() << "Invalid service: " << path;
        throw -1; // FIXME
    }

    m_propertiesCache = properties;

    connect(m_service,
            SIGNAL(PropertyChanged(const QString&, const QDBusVariant&)),
            this,
            SLOT(propertyChanged(const QString&, const QDBusVariant&)));
}

NetworkService::~NetworkService() {}

const QString NetworkService::name() const {
    return m_propertiesCache.value(Name).toString();
}

const QString NetworkService::state() const {
    return m_propertiesCache.value(State).toString();
}

const QString NetworkService::type() const {
    return m_propertiesCache.value(Type).toString();
}

const QStringList NetworkService::security() const {
    return m_propertiesCache.value(Security).toStringList();
}

const uint NetworkService::strength() const {
    return m_propertiesCache.value(Strength).toUInt();
}

const bool NetworkService::favorite() const {
    return m_propertiesCache.value(Favorite).toBool();
}

const bool NetworkService::autoConnect() const {
    return m_propertiesCache.value(AutoConnect).toBool();
}

const QVariantMap NetworkService::ipv4() const {
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv4));
}

const QVariantMap NetworkService::ipv4Config() const {
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv4Config));
}

const QVariantMap NetworkService::ipv6() const {
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv6));
}

const QVariantMap NetworkService::ipv6Config() const {
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv6Config));
}

const QStringList NetworkService::nameservers() const {
    return m_propertiesCache.value(Nameservers).toStringList();
}

const QStringList NetworkService::nameserversConfig() const {
    return m_propertiesCache.value(NameserversConfig).toStringList();
}

const QStringList NetworkService::domains() const {
    return m_propertiesCache.value(Domains).toStringList();
}

const QStringList NetworkService::domainsConfig() const {
    return m_propertiesCache.value(DomainsConfig).toStringList();
}

const QVariantMap NetworkService::proxy() const {
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(Proxy));
}

const QVariantMap NetworkService::proxyConfig() const {
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(ProxyConfig));
}

const QVariantMap NetworkService::ethernet() const {
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(Ethernet));
}

void NetworkService::requestConnect()
{
    // increase reply timeout when connecting
    int timeout = CONNECT_TIMEOUT_FAVORITE;
    int old_timeout = m_service->timeout();
    if (!favorite()) {
        timeout = CONNECT_TIMEOUT;
    }
    m_service->setTimeout(timeout);
    QDBusPendingReply<> conn_reply = m_service->Connect();
    m_service->setTimeout(old_timeout);

    dbg_connectWatcher = new QDBusPendingCallWatcher(conn_reply, m_service);
    connect(dbg_connectWatcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            this,
            SLOT(dbg_connectReply(QDBusPendingCallWatcher*)));
}

void NetworkService::requestDisconnect()
{
    m_service->Disconnect();
}

void NetworkService::setAutoConnect(const bool autoconnect)
{
    // QDBusPendingReply<void> reply =
    m_service->SetProperty(AutoConnect, QDBusVariant(QVariant(autoconnect)));
}

void NetworkService::setIpv4Config(const QVariantMap &ipv4)
{
    // QDBusPendingReply<void> reply =
    m_service->SetProperty(IPv4Config, QDBusVariant(QVariant(ipv4)));
}

void NetworkService::setIpv6Config(const QVariantMap &ipv6)
{
    // QDBusPendingReply<void> reply =
    m_service->SetProperty(IPv6Config, QDBusVariant(QVariant(ipv6)));
}

void NetworkService::setNameserversConfig(const QStringList &nameservers)
{
    // QDBusPendingReply<void> reply =
    m_service->SetProperty(NameserversConfig, QDBusVariant(QVariant(nameservers)));
}

void NetworkService::setDomainsConfig(const QStringList &domains)
{
    // QDBusPendingReply<void> reply =
    m_service->SetProperty(DomainsConfig, QDBusVariant(QVariant(domains)));
}

void NetworkService::setProxyConfig(const QVariantMap &proxy)
{
    // QDBusPendingReply<void> reply =
    m_service->SetProperty(ProxyConfig, QDBusVariant(QVariant(proxy)));
}

/* this slot is used for debugging */
void NetworkService::dbg_connectReply(QDBusPendingCallWatcher *call){
    pr_dbg() << "Got something from service.connect()";
    Q_ASSERT(call);
    QDBusPendingReply<> reply = *call;
    if (!reply.isFinished()) {
       pr_dbg() << "connect() not finished yet";
    }
    if (reply.isError()) {
        pr_dbg() << "Reply from service.connect(): " << reply.error().message();
    }
}

void NetworkService::propertyChanged(const QString &name, const QDBusVariant &value)
{
    QVariant tmp = value.variant();

    Q_ASSERT(m_service);

    pr_dbg() << m_service->path() << "property" << name << "changed from"
             << m_propertiesCache[name].toString() << "to" << tmp.toString();

    m_propertiesCache[name] = tmp;

    if (name == Name) {
        emit nameChanged(tmp.toString());
    } else if (name == State) {
        emit stateChanged(tmp.toString());
    } else if (name == Security) {
        emit securityChanged(tmp.toStringList());
    } else if (name == Strength) {
        emit strengthChanged(tmp.toUInt());
    } else if (name == Favorite) {
        emit favoriteChanged(tmp.toBool());
    } else if (name == IPv4) {
        emit ipv4Changed(qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv4)));
    } else if (name == IPv4Config) {
        emit ipv4ConfigChanged(qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv4Config)));
    } else if (name == IPv6) {
        emit ipv6Changed(qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv6)));
    } else if (name == IPv6Config) {
        emit ipv6ConfigChanged(qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv6Config)));
    } else if (name == Nameservers) {
        emit nameserversChanged(tmp.toStringList());
    } else if (name == NameserversConfig) {
        emit nameserversConfigChanged(tmp.toStringList());
    } else if (name == Domains) {
        emit domainsChanged(tmp.toStringList());
    } else if (name == DomainsConfig) {
        emit domainsConfigChanged(tmp.toStringList());
    } else if (name == Proxy) {
        emit proxyChanged(qdbus_cast<QVariantMap>(m_propertiesCache.value(Proxy)));
    } else if (name == ProxyConfig) {
        emit proxyConfigChanged(qdbus_cast<QVariantMap>(m_propertiesCache.value(ProxyConfig)));
    } else if (name == Ethernet) {
        emit ethernetChanged(qdbus_cast<QVariantMap>(m_propertiesCache.value(Ethernet)));
    }
}
