/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2019 Jolla Ltd.
 * Contact: Slava Monich <slava.monich@jolla.com>
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0. The full text of the Apache License
 * is at http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef NETWORKSERVICE_H
#define NETWORKSERVICE_H

#include <QObject>
#include <QVariant>

class NetworkService : public QObject
{
    Q_OBJECT

    Q_ENUMS(EapMethod)
    Q_ENUMS(SecurityType)
    Q_ENUMS(ServiceState)

    Q_PROPERTY(bool valid READ isValid NOTIFY validChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(ServiceState serviceState READ serviceState NOTIFY serviceStateChanged)
    Q_PROPERTY(QString type READ type NOTIFY typeChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QStringList security READ security NOTIFY securityChanged)
    Q_PROPERTY(SecurityType securityType READ securityType NOTIFY securityTypeChanged)
    Q_PROPERTY(uint strength READ strength NOTIFY strengthChanged)
    Q_PROPERTY(bool favorite READ favorite NOTIFY favoriteChanged)
    Q_PROPERTY(bool autoConnect READ autoConnect WRITE setAutoConnect NOTIFY autoConnectChanged)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QVariantMap ipv4 READ ipv4 NOTIFY ipv4Changed)
    Q_PROPERTY(QVariantMap ipv4Config READ ipv4Config WRITE setIpv4Config NOTIFY ipv4ConfigChanged)
    Q_PROPERTY(QVariantMap ipv6 READ ipv6 NOTIFY ipv6Changed)
    Q_PROPERTY(QVariantMap ipv6Config READ ipv6Config WRITE setIpv6Config NOTIFY ipv6ConfigChanged)
    Q_PROPERTY(QStringList nameservers READ nameservers NOTIFY nameserversChanged)
    Q_PROPERTY(QStringList nameserversConfig READ nameserversConfig WRITE setNameserversConfig NOTIFY nameserversConfigChanged)
    Q_PROPERTY(QStringList domains READ domains NOTIFY domainsChanged)
    Q_PROPERTY(QStringList domainsConfig READ domainsConfig WRITE setDomainsConfig NOTIFY domainsConfigChanged)
    Q_PROPERTY(QVariantMap proxy READ proxy NOTIFY proxyChanged)
    Q_PROPERTY(QVariantMap proxyConfig READ proxyConfig WRITE setProxyConfig NOTIFY proxyConfigChanged)
    Q_PROPERTY(QVariantMap ethernet READ ethernet NOTIFY ethernetChanged)
    Q_PROPERTY(bool roaming READ roaming NOTIFY roamingChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QStringList timeservers READ timeservers NOTIFY timeserversChanged)
    Q_PROPERTY(QStringList timeserversConfig READ timeserversConfig WRITE setTimeserversConfig NOTIFY timeserversConfigChanged)

    Q_PROPERTY(EapMethod eapMethod READ eapMethod WRITE setEapMethod NOTIFY eapMethodChanged)
    Q_PROPERTY(int peapVersion READ peapVersion WRITE setPeapVersion NOTIFY peapVersionChanged)
    Q_PROPERTY(QString identity READ identity WRITE setIdentity NOTIFY identityChanged)
    Q_PROPERTY(QString passphrase READ passphrase WRITE setPassphrase NOTIFY passphraseChanged)
    Q_PROPERTY(bool eapMethodAvailable READ eapMethodAvailable NOTIFY eapMethodAvailableChanged)
    Q_PROPERTY(bool identityAvailable READ identityAvailable NOTIFY identityAvailableChanged)
    Q_PROPERTY(bool passphraseAvailable READ passphraseAvailable NOTIFY passphraseAvailableChanged)
    Q_PROPERTY(bool phase2Available READ phase2Available NOTIFY phase2AvailableChanged)
    Q_PROPERTY(bool caCertAvailable READ caCertAvailable NOTIFY caCertAvailableChanged)
    Q_PROPERTY(bool caCertFileAvailable READ caCertFileAvailable NOTIFY caCertFileAvailableChanged)
    Q_PROPERTY(bool domainSuffixMatchAvailable READ domainSuffixMatchAvailable NOTIFY domainSuffixMatchAvailableChanged)
    Q_PROPERTY(bool privateKeyAvailable READ privateKeyAvailable NOTIFY privateKeyAvailableChanged)
    Q_PROPERTY(bool privateKeyFileAvailable READ privateKeyFileAvailable NOTIFY privateKeyFileAvailableChanged)
    Q_PROPERTY(bool privateKeyPassphraseAvailable READ privateKeyPassphraseAvailable NOTIFY privateKeyPassphraseAvailableChanged)
    Q_PROPERTY(bool anonymousIdentityAvailable READ anonymousIdentityAvailable NOTIFY anonymousIdentityAvailableChanged)
    Q_PROPERTY(QString bssid READ bssid NOTIFY bssidChanged)
    Q_PROPERTY(quint32 maxRate READ maxRate NOTIFY maxRateChanged)
    Q_PROPERTY(quint16 frequency READ frequency NOTIFY frequencyChanged)
    Q_PROPERTY(QString encryptionMode READ encryptionMode NOTIFY encryptionModeChanged)
    Q_PROPERTY(bool hidden READ hidden NOTIFY hiddenChanged)
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(bool managed READ managed NOTIFY managedChanged)
    Q_PROPERTY(bool saved READ saved NOTIFY savedChanged)
    Q_PROPERTY(bool connecting READ connecting NOTIFY connectingChanged)
    Q_PROPERTY(QString lastConnectError READ lastConnectError NOTIFY lastConnectErrorChanged)
    Q_PROPERTY(QString caCert READ caCert WRITE setCACert NOTIFY caCertChanged)
    Q_PROPERTY(QString caCertFile READ caCertFile WRITE setCACertFile NOTIFY caCertFileChanged)
    Q_PROPERTY(QString clientCert READ clientCert WRITE setClientCert NOTIFY clientCertChanged)
    Q_PROPERTY(QString clientCertFile READ clientCertFile WRITE setClientCertFile NOTIFY clientCertFileChanged)
    Q_PROPERTY(QString privateKey READ privateKey WRITE setPrivateKey NOTIFY privateKeyChanged)
    Q_PROPERTY(QString privateKeyFile READ privateKeyFile WRITE setPrivateKeyFile NOTIFY privateKeyFileChanged)
    Q_PROPERTY(QString privateKeyPassphrase READ privateKeyPassphrase WRITE setPrivateKeyPassphrase NOTIFY privateKeyPassphraseChanged)
    Q_PROPERTY(QString domainSuffixMatch READ domainSuffixMatch WRITE setDomainSuffixMatch NOTIFY domainSuffixMatchChanged)
    Q_PROPERTY(QString phase2 READ phase2 WRITE setPhase2 NOTIFY phase2Changed)
    Q_PROPERTY(QString anonymousIdentity READ anonymousIdentity WRITE setAnonymousIdentity NOTIFY anonymousIdentityChanged)

    class Private;
    friend class Private;

public:
    enum ServiceState {
        UnknownState,
        IdleState,
        FailureState,
        AssociationState,
        ConfigurationState,
        ReadyState,
        DisconnectState,
        OnlineState
    };

    enum SecurityType {
        SecurityUnknown,
        SecurityNone,
        SecurityWEP,
        SecurityPSK,
        SecurityIEEE802
    };

    enum EapMethod {
        EapNone,
        EapPEAP,
        EapTTLS,
        EapTLS
    };

    NetworkService(const QString &path, const QVariantMap &properties, QObject* parent);
    NetworkService(QObject* parent = 0);

    virtual ~NetworkService();

    QString name() const;
    QString type() const;
    QString state() const; // deprecated, use serviceState()
    ServiceState serviceState() const;
    QString error() const;
    QStringList security() const;
    SecurityType securityType() const;
    bool autoConnect() const;
    uint strength() const;
    bool favorite() const;
    QString path() const;
    QVariantMap ipv4() const;
    QVariantMap ipv4Config() const;
    QVariantMap ipv6() const;
    QVariantMap ipv6Config() const;
    QStringList nameservers() const;
    QStringList nameserversConfig() const;
    QStringList domains() const;
    QStringList domainsConfig() const;
    QVariantMap proxy() const;
    QVariantMap proxyConfig() const;
    QVariantMap ethernet() const;
    bool roaming() const;
    QString caCert() const;
    QString caCertFile() const;
    QString clientCert() const;
    QString clientCertFile() const;
    QString domainSuffixMatch() const;
    QString phase2() const;
    QString anonymousIdentity() const;

    void setPath(const QString &path);
    void updateProperties(const QVariantMap &properties);

    bool isValid() const;
    bool connected() const;
    bool available() const;
    bool managed() const;
    bool saved() const;
    bool connecting() const;
    QString lastConnectError() const;

    QStringList timeservers() const;
    QStringList timeserversConfig() const;
    void setTimeserversConfig(const QStringList &servers);

    bool hidden() const;
    QString bssid() const;
    quint32 maxRate() const;
    quint16 frequency() const;
    QString encryptionMode() const;

    QString passphrase() const;
    void setPassphrase(QString passphrase);
    bool passphraseAvailable() const;

    QString privateKey() const;
    void setPrivateKey(const QString &privateKey);
    bool privateKeyAvailable() const;

    QString privateKeyFile() const;
    void setPrivateKeyFile(const QString &privateKeyFile);
    bool privateKeyFileAvailable() const;

    QString privateKeyPassphrase() const;
    void setPrivateKeyPassphrase(const QString &passphrase);
    bool privateKeyPassphraseAvailable() const;

    QString identity() const;
    void setIdentity(QString identity);
    bool identityAvailable() const;

    EapMethod eapMethod() const;
    void setEapMethod(EapMethod method);
    bool eapMethodAvailable() const;
    bool phase2Available() const;
    bool caCertAvailable() const;
    bool caCertFileAvailable() const;
    bool domainSuffixMatchAvailable() const;
    bool anonymousIdentityAvailable() const;

    int peapVersion() const;
    void setPeapVersion(int version);

    void moveBefore(const QString &service);
    void moveAfter(const QString &service);

Q_SIGNALS:
    void validChanged();
    void nameChanged(const QString &name);
    void stateChanged(const QString &state);
    void serviceStateChanged(ServiceState state);
    void errorChanged(const QString &error);
    void securityChanged(const QStringList &security);
    void strengthChanged(uint strength);
    void favoriteChanged(bool favorite);
    void autoConnectChanged(bool autoconnect);
    void pathChanged(const QString &path);
    void ipv4Changed(const QVariantMap &ipv4);
    void ipv4ConfigChanged(const QVariantMap &ipv4);
    void ipv6Changed(const QVariantMap &ipv6);
    void ipv6ConfigChanged(const QVariantMap &ipv6);
    void nameserversChanged(const QStringList &nameservers);
    void nameserversConfigChanged(const QStringList &nameservers);
    void domainsChanged(const QStringList &domains);
    void domainsConfigChanged(const QStringList &domains);
    void proxyChanged(const QVariantMap &proxy);
    void proxyConfigChanged(const QVariantMap &proxy);
    void caCertChanged(const QString &caCert);
    void caCertFileChanged(const QString &caCertFile);
    void clientCertChanged(const QString &clientCert);
    void clientCertFileChanged(const QString &clientCertFile);
    void privateKeyChanged(const QString &privateKey);
    void privateKeyFileChanged(const QString &privateKeyFile);
    void privateKeyPassphraseChanged(const QString &privateKeyFile);
    void domainSuffixMatchChanged(const QString &domainSuffixMatch);
    void phase2Changed(const QString &phase2);
    void anonymousIdentityChanged(const QString &anonymousIdentity);
    void ethernetChanged(const QVariantMap &ethernet);
    void connectRequestFailed(const QString &error);
    void typeChanged(const QString &type);
    void roamingChanged(bool roaming);
    void timeserversChanged(const QStringList &timeservers);
    void timeserversConfigChanged(const QStringList &timeservers);

    void serviceConnectionStarted();
    void serviceDisconnectionStarted();
    void connectedChanged(bool connected);

    void propertiesReady();

    void bssidChanged(const QString &bssid);
    void maxRateChanged(quint32 rate);
    void frequencyChanged(quint16 frequency);
    void encryptionModeChanged(const QString &mode);
    void hiddenChanged(bool);

    void managedChanged();
    void passphraseChanged(QString);
    void passphraseAvailableChanged();
    void identityChanged(QString);
    void identityAvailableChanged();
    void securityTypeChanged();
    void eapMethodChanged();
    void peapVersionChanged();
    void eapMethodAvailableChanged();
    void phase2AvailableChanged();
    void privateKeyAvailableChanged();
    void privateKeyFileAvailableChanged();
    void privateKeyPassphraseAvailableChanged();
    void caCertAvailableChanged();
    void caCertFileAvailableChanged();
    void domainSuffixMatchAvailableChanged();
    void anonymousIdentityAvailableChanged();
    void availableChanged();
    void savedChanged();
    void connectingChanged();
    void lastConnectErrorChanged();

public Q_SLOTS:
    void requestConnect();
    void requestDisconnect();
    void remove();

    void setAutoConnect(bool autoconnect);
    void setIpv4Config(const QVariantMap &ipv4);
    void setIpv6Config(const QVariantMap &ipv6);
    void setNameserversConfig(const QStringList &nameservers);
    void setDomainsConfig(const QStringList &domains);
    void setProxyConfig(const QVariantMap &proxy);
    void setCACert(const QString &caCert);
    void setCACertFile(const QString &caCertFile);
    void setClientCert(const QString &clientCert);
    void setClientCertFile(const QString &clientCertFile);
    void setDomainSuffixMatch(const QString &domainSuffixMatch);
    void setPhase2(const QString &phase2);
    void setAnonymousIdentity(const QString &anonymousIdentity);

    void resetCounters();

private:
    Private *m_priv;

private:
    Q_DISABLE_COPY(NetworkService)
};

Q_DECLARE_METATYPE(NetworkService*)

#endif // NETWORKSERVICE_H
