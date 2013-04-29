/*
 * Copyright © 2010, Intel Corporation.
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "networkservice.h"
#include "connman_service_interface.h"
#include "debug.h"

/*
 * JS returns arrays as QVariantList or a(v) in terms of D-Bus,
 * but ConnMan requires some properties to be lists of strings
 * or a(s) thus this function.
 */
QVariantMap adaptToConnmanProperties(const QVariantMap &map)
{
    QVariantMap buffer;
    foreach (const QString &key, map.keys()) {
        if (map.value(key).type() == QVariant::List) {
            QStringList strList;
            foreach (const QVariant &value, map.value(key).toList()) {
                strList.append(value.toString());
            }
            buffer.insert(key, strList);
        } else {
            buffer.insert(key, map.value(key));
        }
    }
    return buffer;
}

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

NetworkService::NetworkService(const QString &path, const QVariantMap &properties, QObject* parent)
  : QObject(parent),
    m_service(NULL),
    m_path(QString())
{
    qRegisterMetaType<NetworkService *>();

    Q_ASSERT(!path.isEmpty());
    m_propertiesCache = properties;
    setPath(path);
}

NetworkService::NetworkService(QObject* parent)
    : QObject(parent),
      m_service(NULL),
      m_path(QString())
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
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv4));
}

const QVariantMap NetworkService::ipv4Config() const
{
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv4Config));
}

const QVariantMap NetworkService::ipv6() const
{
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv6));
}

const QVariantMap NetworkService::ipv6Config() const
{
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(IPv6Config));
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
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(Proxy));
}

const QVariantMap NetworkService::proxyConfig() const
{
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(ProxyConfig));
}

const QVariantMap NetworkService::ethernet() const
{
    return qdbus_cast<QVariantMap>(m_propertiesCache.value(Ethernet));
}

bool NetworkService::roaming() const
{
    return m_propertiesCache.value(Roaming).toBool();
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

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(conn_reply, m_service);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            this,
            SLOT(handleConnectReply(QDBusPendingCallWatcher*)));
}

void NetworkService::requestDisconnect()
{
    m_service->Disconnect();
}

void NetworkService::remove()
{
    m_service->Remove();
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
    m_service->SetProperty(ProxyConfig, QDBusVariant(QVariant(adaptToConnmanProperties(proxy))));
}

void NetworkService::handleConnectReply(QDBusPendingCallWatcher *call)
{
    Q_ASSERT(call);
    QDBusPendingReply<> reply = *call;

    if (!reply.isFinished()) {
       pr_dbg() << "connect() not finished yet";
    }
    if (reply.isError()) {
        pr_dbg() << "Reply from service.connect(): " << reply.error().message();
        emit connectRequestFailed(reply.error().message());
    }

    call->deleteLater();
}

void NetworkService::updateProperty(const QString &name, const QDBusVariant &value)
{
    QVariant tmp = value.variant();

    Q_ASSERT(m_service);

    m_propertiesCache[name] = tmp;

    if (name == Name) {
        emit nameChanged(tmp.toString());
    } else if (name == Error) {
        emit errorChanged(tmp.toString());
    } else if (name == State) {
        emit stateChanged(tmp.toString());
    } else if (name == Security) {
        emit securityChanged(tmp.toStringList());
    } else if (name == Strength) {
        emit strengthChanged(tmp.toUInt());
    } else if (name == Favorite) {
        emit favoriteChanged(tmp.toBool());
    } else if (name == AutoConnect) {
        emit autoConnectChanged(tmp.toBool());
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
    } else if (name == QLatin1String("Type")) {
        Q_EMIT typeChanged(tmp.toString());
    } else if (name == Roaming) {
        emit roamingChanged(tmp.toBool());
    }
}

void NetworkService::setPath(const QString &path)
{
    if (path != m_path) {
        m_path = path;

        if (m_service) {
            delete m_service;
            m_service = 0;
            // TODO: After resetting the path iterate through old properties, compare their values
            //       with new ones and emit corresponding signals if changed.
            m_propertiesCache.clear();
        }
        m_service = new NetConnmanServiceInterface("net.connman", m_path,
            QDBusConnection::systemBus(), this);

        if (!m_service->isValid()) {
            pr_dbg() << "Invalid service: " << m_path;
            return;
        }

        if (m_propertiesCache.isEmpty()) {
            QDBusPendingReply<QVariantMap> reply = m_service->GetProperties();
            reply.waitForFinished();
            if (reply.isError()) {
                qDebug() << Q_FUNC_INFO << reply.error().message();
            } else {
                m_propertiesCache = reply.value();
            }
        }

        connect(m_service, SIGNAL(PropertyChanged(QString,QDBusVariant)),
                this, SLOT(updateProperty(QString,QDBusVariant)));
    }
}
