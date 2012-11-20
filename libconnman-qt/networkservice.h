/*
 * Copyright © 2010, Intel Corporation.
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#ifndef NETWORKSERVICE_H
#define NETWORKSERVICE_H

#include <QtDBus>

#define CONNECT_TIMEOUT 180000 // user is supposed to provide input for unconfigured networks
#define CONNECT_TIMEOUT_FAVORITE 60000

class Service;

class NetworkService : public QObject
{
    Q_OBJECT;

    Q_PROPERTY(QString name READ name NOTIFY nameChanged);
    Q_PROPERTY(QString state READ state NOTIFY stateChanged);
    Q_PROPERTY(QString type READ type);
    Q_PROPERTY(QStringList security READ security NOTIFY securityChanged);
    Q_PROPERTY(uint strength READ strength NOTIFY strengthChanged);
    Q_PROPERTY(bool favorite READ favorite NOTIFY favoriteChanged);
    Q_PROPERTY(QVariantMap ipv4 READ ipv4 NOTIFY ipv4Changed);
    Q_PROPERTY(QVariantMap ipv4Config READ ipv4Config WRITE setIpv4Config NOTIFY ipv4ConfigChanged);
    Q_PROPERTY(QStringList nameservers READ nameservers NOTIFY nameserversChanged);
    Q_PROPERTY(QStringList nameserversConfig READ nameserversConfig WRITE setNameserversConfig NOTIFY nameserversConfigChanged);
    Q_PROPERTY(QStringList domains READ domains NOTIFY domainsChanged);
    Q_PROPERTY(QStringList domainsConfig READ domainsConfig WRITE setDomainsConfig NOTIFY domainsConfigChanged);
    Q_PROPERTY(QVariantMap proxy READ proxy NOTIFY proxyChanged);
    Q_PROPERTY(QVariantMap proxyConfig READ proxyConfig WRITE setProxyConfig NOTIFY proxyConfigChanged);

public:
    NetworkService(const QString &path, const QVariantMap &properties, QObject* parent);
    NetworkService(QObject* parent = 0) { Q_ASSERT(false); };
    virtual ~NetworkService();

    const QString name() const;
    const QString type() const;
    const QString state() const;
    const QStringList security() const;
    const uint strength() const;
    const bool favorite() const;
    const QVariantMap ipv4() const;
    const QVariantMap ipv4Config() const;
    const QStringList nameservers() const;
    const QStringList nameserversConfig() const;
    const QStringList domains() const;
    const QStringList domainsConfig() const;
    const QVariantMap proxy() const;
    const QVariantMap proxyConfig() const;

signals:
    void nameChanged(const QString &name);
    void stateChanged(const QString &state);
    void securityChanged(const QStringList &security);
    void strengthChanged(const uint strength);
    void favoriteChanged(const bool &favorite);
    void ipv4Changed(const QVariantMap &ipv4);
    void ipv4ConfigChanged(const QVariantMap &ipv4);
    void nameserversChanged(const QStringList &nameservers);
    void nameserversConfigChanged(const QStringList &nameservers);
    void domainsChanged(const QStringList &domains);
    void domainsConfigChanged(const QStringList &domains);
    void proxyChanged(const QVariantMap &proxy);
    void proxyConfigChanged(const QVariantMap &proxy);

public slots:
    void requestConnect();
    void requestDisconnect();

    void setIpv4Config(const QVariantMap &ipv4);
    void setNameserversConfig(const QStringList &nameservers);
    void setDomainsConfig(const QStringList &domains);
    void setProxyConfig(const QVariantMap &proxy);

private:
    Service *m_service;
    QVariantMap m_propertiesCache;

    QDBusPendingCallWatcher *dbg_connectWatcher;

    static const QString Name;
    static const QString State;
    static const QString Type;
    static const QString Security;
    static const QString Strength;
    static const QString Error;
    static const QString Favorite;
    static const QString IPv4;
    static const QString IPv4Config;
    static const QString Nameservers;
    static const QString NameserversConfig;
    static const QString Domains;
    static const QString DomainsConfig;
    static const QString Proxy;
    static const QString ProxyConfig;

private slots:
    void propertyChanged(const QString &name, const QDBusVariant &value);
    void dbg_connectReply(QDBusPendingCallWatcher *call);

private:
    Q_DISABLE_COPY(NetworkService);
};

#endif // NETWORKSERVICE_H
