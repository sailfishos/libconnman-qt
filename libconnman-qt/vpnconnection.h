/*
 * Copyright (c) 2016 - 2019 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#ifndef VPNCONNECTION_H
#define VPNCONNECTION_H

#include <QObject>
#include <QVariantMap>

class VpnConnectionPrivate;

// The userRoutes and serverRoutes properties are QVariants containing a
// QList<RouteStructure> structure
struct RouteStructure
{
    int protocolFamily;
    QString network;
    QString netmask;
    QString gateway;
};
Q_DECLARE_METATYPE(RouteStructure)

class VpnConnection : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(VpnConnection)
    Q_DISABLE_COPY(VpnConnection)

    Q_PROPERTY(QString path READ path CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(QString domain READ domain WRITE setDomain NOTIFY domainChanged)
    Q_PROPERTY(bool autoConnect READ autoConnect WRITE setAutoConnect NOTIFY autoConnectChanged)
    Q_PROPERTY(bool storeCredentials READ storeCredentials WRITE setStoreCredentials NOTIFY storeCredentialsChanged)
    Q_PROPERTY(ConnectionState state READ state NOTIFY stateChanged)

    Q_PROPERTY(QString type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(bool immutable READ immutable WRITE setImmutable NOTIFY immutableChanged)
    Q_PROPERTY(int index READ index WRITE setIndex NOTIFY indexChanged)
    Q_PROPERTY(QVariantMap ipv4 READ ipv4 WRITE setIpv4 NOTIFY ipv4Changed)
    Q_PROPERTY(QVariantMap ipv6 READ ipv6 WRITE setIpv6 NOTIFY ipv6Changed)
    Q_PROPERTY(QStringList nameservers READ nameservers WRITE setNameservers NOTIFY nameserversChanged)
    Q_PROPERTY(QVariant userRoutes READ userRoutes WRITE setUserRoutes NOTIFY userRoutesChanged)
    Q_PROPERTY(QVariant serverRoutes READ serverRoutes WRITE setServerRoutes NOTIFY serverRoutesChanged)
    Q_PROPERTY(bool defaultRoute READ defaultRoute WRITE setDefaultRoute NOTIFY defaultRouteChanged)

    Q_PROPERTY(QVariantMap properties READ properties WRITE setProperties NOTIFY propertiesChanged)
    Q_PROPERTY(QVariantMap providerProperties READ providerProperties WRITE setProviderProperties NOTIFY providerPropertiesChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

public:
    enum ConnectionState {
        Idle,
        Failure,
        Configuration,
        Ready,
        Disconnect
    };
    Q_ENUM(ConnectionState)

    explicit VpnConnection(QObject *parent = nullptr);
    explicit VpnConnection(const QString &path, QObject *parent = nullptr);
    explicit VpnConnection(VpnConnectionPrivate &dd, QObject *parent);
    virtual ~VpnConnection();

    void modifyConnection(const QVariantMap &properties);
    void activate();
    void deactivate();
    void update(const QVariantMap &updateProperties);
    int connected() const;

    QString path() const;

    QString name() const;
    void setName(const QString &name);

    QString host() const;
    void setHost(const QString &host);

    QString domain() const;
    void setDomain(const QString &domain);

    bool autoConnect() const;
    void setAutoConnect(bool autoConnect);

    bool storeCredentials() const;
    void setStoreCredentials(bool storeCredentials);

    ConnectionState state() const;

    QString type() const;
    void setType(const QString &type);

    bool immutable() const;
    void setImmutable(bool immutable);

    int index() const;
    void setIndex(int index);

    QVariantMap ipv4() const;
    void setIpv4(const QVariantMap &iPv4);

    QVariantMap ipv6() const;
    void setIpv6(const QVariantMap &iPv6);

    QStringList nameservers() const;
    void setNameservers(const QStringList &nameservers);

    QVariant userRoutes() const;
    void setUserRoutes(const QVariant &userRoutes);

    QVariant serverRoutes() const;
    void setServerRoutes(const QVariant &serverRoutes);

    bool defaultRoute() const;
    void setDefaultRoute(bool defaultRoute);

    QVariantMap properties() const;
    void setProperties(const QVariantMap properties);

    QVariantMap providerProperties() const;
    void setProviderProperties(const QVariantMap &providerProperties);

signals:
    void nameChanged();
    void hostChanged();
    void domainChanged();
    void autoConnectChanged();
    void storeCredentialsChanged();
    void stateChanged();
    void typeChanged();
    void immutableChanged();
    void indexChanged();
    void ipv4Changed();
    void ipv6Changed();
    void nameserversChanged();
    void userRoutesChanged();
    void serverRoutesChanged();
    void defaultRouteChanged();
    void propertiesChanged();
    void providerPropertiesChanged();
    void connectedChanged();

private:
    QScopedPointer<VpnConnectionPrivate> d_ptr;
};

#endif // VPNCONNECTION_H
