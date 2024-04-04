/*
 * Copyright © 2013, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "connmannetworkproxyfactory.h"

class ConnmanNetworkProxyFactoryPrivate
{
public:
    ConnmanNetworkProxyFactoryPrivate();

    QPointer<NetworkService> m_defaultRoute;
    QList<QNetworkProxy> m_cachedProxies_all;
    QList<QNetworkProxy> m_cachedProxies_udpSocketOrTcpServerCapable;
    QSharedPointer<NetworkManager> m_networkManager;
};

ConnmanNetworkProxyFactoryPrivate::ConnmanNetworkProxyFactoryPrivate()
    : m_networkManager(NetworkManager::sharedInstance())
{
}

ConnmanNetworkProxyFactory::ConnmanNetworkProxyFactory(QObject *parent)
    : QObject(parent)
    , d_ptr(new ConnmanNetworkProxyFactoryPrivate)
{
    connect(d_ptr->m_networkManager.data(), &NetworkManager::defaultRouteChanged,
            this, &ConnmanNetworkProxyFactory::onDefaultRouteChanged);
    onDefaultRouteChanged(d_ptr->m_networkManager->defaultRoute());
}

ConnmanNetworkProxyFactory::~ConnmanNetworkProxyFactory()
{
    delete d_ptr;
    d_ptr = nullptr;
}

QList<QNetworkProxy> ConnmanNetworkProxyFactory::queryProxy(const QNetworkProxyQuery & query)
{
    return (query.queryType() == QNetworkProxyQuery::UdpSocket
            || query.queryType() == QNetworkProxyQuery::TcpServer)
        ? d_ptr->m_cachedProxies_udpSocketOrTcpServerCapable
        : d_ptr->m_cachedProxies_all;
}

void ConnmanNetworkProxyFactory::onDefaultRouteChanged(NetworkService *defaultRoute)
{
    if (d_ptr->m_defaultRoute) {
        d_ptr->m_defaultRoute->disconnect(this);
        d_ptr->m_defaultRoute = nullptr;
    }

    d_ptr->m_cachedProxies_all = QList<QNetworkProxy>() << QNetworkProxy::NoProxy;
    d_ptr->m_cachedProxies_udpSocketOrTcpServerCapable = QList<QNetworkProxy>() << QNetworkProxy::NoProxy;

    if (defaultRoute) {
        d_ptr->m_defaultRoute = defaultRoute;
        connect(d_ptr->m_defaultRoute, SIGNAL(proxyChanged(QVariantMap)),
                this, SLOT(onProxyChanged(QVariantMap)));
        onProxyChanged(d_ptr->m_defaultRoute->proxy());
    }
}

void ConnmanNetworkProxyFactory::onProxyChanged(const QVariantMap &proxy)
{
    d_ptr->m_cachedProxies_all.clear();
    d_ptr->m_cachedProxies_udpSocketOrTcpServerCapable.clear();

    QList<QUrl> proxyUrls;
    if (proxy.value("Method").toString() == QLatin1String("auto")) {
        const QUrl proxyUrl = proxy.value("URL").toUrl();
        if (!proxyUrl.isEmpty()) {
            proxyUrls.append(proxyUrl);
        }
    } else if (proxy.value("Method").toString() == QLatin1String("manual")) {
        const QStringList proxyUrlStrings = proxy.value("Servers").toStringList();
        for (const QString &proxyUrlString : proxyUrlStrings) {
            proxyUrls.append(QUrl(proxyUrlString));
        }
    }

    for (const QUrl &url : proxyUrls) {
        if (url.scheme() == QLatin1String("socks5")) {
            QNetworkProxy proxy(QNetworkProxy::Socks5Proxy, url.host(),
                                url.port() ? url.port() : 1080,
                                url.userName(), url.password());
            d_ptr->m_cachedProxies_all.append(proxy);
            d_ptr->m_cachedProxies_udpSocketOrTcpServerCapable.append(proxy);
        } else if (url.scheme() == QLatin1String("socks5h")) {
            QNetworkProxy proxy(QNetworkProxy::Socks5Proxy, url.host(),
                                url.port() ? url.port() : 1080,
                                url.userName(), url.password());
            proxy.setCapabilities(QNetworkProxy::HostNameLookupCapability);
            d_ptr->m_cachedProxies_all.append(proxy);
            d_ptr->m_cachedProxies_udpSocketOrTcpServerCapable.append(proxy);
        } else if (url.scheme() == QLatin1String("http") || url.scheme().isEmpty()) {
            QNetworkProxy proxy(QNetworkProxy::HttpProxy, url.host(),
                                url.port() ? url.port() : 8080,
                                url.userName(), url.password());
            d_ptr->m_cachedProxies_all.append(proxy);
        }
    }

    if (d_ptr->m_cachedProxies_all.isEmpty()) {
        d_ptr->m_cachedProxies_all.append(QNetworkProxy::NoProxy);
    }

    if (d_ptr->m_cachedProxies_udpSocketOrTcpServerCapable.isEmpty()) {
        d_ptr->m_cachedProxies_udpSocketOrTcpServerCapable.append(QNetworkProxy::NoProxy);
    }
}
