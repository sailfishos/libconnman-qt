#ifndef DECLARATIVENETWORKMANAGER_H
#define DECLARATIVENETWORKMANAGER_H

#include "networkmanager.h"

class DeclarativeNetworkManager;

class DeclarativeNetworkManagerFactory : public QObject
{
    Q_OBJECT
    Q_PROPERTY(DeclarativeNetworkManager* instance READ instance CONSTANT)
public:
    DeclarativeNetworkManagerFactory(QObject *parent = nullptr);
    ~DeclarativeNetworkManagerFactory();

    DeclarativeNetworkManager *instance();

private:
    DeclarativeNetworkManager *m_instance;
};

class DeclarativeNetworkManager: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ isAvailable NOTIFY availabilityChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool offlineMode READ offlineMode WRITE setOfflineModeProperty NOTIFY offlineModeChanged)
    Q_PROPERTY(NetworkService* defaultRoute READ defaultRoute NOTIFY defaultRouteChanged)
    Q_PROPERTY(NetworkService* connectedWifi READ connectedWifi NOTIFY connectedWifiChanged)
    Q_PROPERTY(NetworkService* connectedEthernet READ connectedEthernet NOTIFY connectedEthernetChanged)
    Q_PROPERTY(bool connectingWifi READ connectingWifi NOTIFY connectingWifiChanged)

    Q_PROPERTY(bool sessionMode READ sessionMode WRITE setSessionModeProperty NOTIFY sessionModeChanged)
    Q_PROPERTY(uint inputRequestTimeout READ inputRequestTimeout NOTIFY inputRequestTimeoutChanged)

    Q_PROPERTY(bool servicesEnabled READ servicesEnabled WRITE setServicesEnabled NOTIFY servicesEnabledChanged)
    Q_PROPERTY(bool technologiesEnabled READ technologiesEnabled WRITE setTechnologiesEnabled NOTIFY technologiesEnabledChanged)
    Q_PROPERTY(bool valid READ isValid NOTIFY validChanged)

    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(bool connecting READ connecting NOTIFY connectingChanged)

    Q_PROPERTY(QString WifiTechnology READ wifiTechnologyPath CONSTANT)
    Q_PROPERTY(QString CellularTechnology READ cellularTechnologyPath CONSTANT)
    Q_PROPERTY(QString BluetoothTechnology READ bluetoothTechnologyPath CONSTANT)
    Q_PROPERTY(QString GpsTechnology READ gpsTechnologyPath CONSTANT)
    Q_PROPERTY(QString EthernetTechnology READ ethernetTechnologyPath CONSTANT)

public:
    DeclarativeNetworkManager(QObject *parent = 0);
    virtual ~DeclarativeNetworkManager();

    bool isAvailable() const;
    QString state() const;
    bool offlineMode() const;
    NetworkService* defaultRoute() const;
    NetworkService* connectedWifi() const;
    NetworkService *connectedEthernet() const;
    bool connectingWifi() const;

    bool sessionMode() const;
    uint inputRequestTimeout() const;

    bool servicesEnabled() const;
    void setServicesEnabled(bool enabled);

    bool technologiesEnabled() const;
    void setTechnologiesEnabled(bool enabled);

    bool isValid() const;
    bool connected() const;
    bool connecting() const;

    QString wifiTechnologyPath() const;
    QString cellularTechnologyPath() const;
    QString bluetoothTechnologyPath() const;
    QString gpsTechnologyPath() const;
    QString ethernetTechnologyPath() const;

    Q_INVOKABLE QStringList servicesList(const QString &tech);
    Q_INVOKABLE QStringList savedServicesList(const QString &tech = QString());
    Q_INVOKABLE QStringList availableServices(const QString &tech = QString());
    Q_INVOKABLE QStringList technologiesList();
    Q_INVOKABLE QString technologyPathForService(const QString &path);
    Q_INVOKABLE QString technologyPathForType(const QString &type);

    Q_INVOKABLE void resetCountersForType(const QString &type);
    // Note: getTechnology() skipped, it was deprecated and use discouraged on qml

    void setOfflineModeProperty(bool offlineMode);
    void setSessionModeProperty(bool sessionMode);

public Q_SLOTS:
    // deprecated, should assign property
    void setOfflineMode(bool offlineMode);
    void setSessionMode(bool sessionMode);

    void registerAgent(const QString &path);
    void unregisterAgent(const QString &path);
    void registerCounter(const QString &path, quint32 accuracy, quint32 period);
    void unregisterCounter(const QString &path);

    // createSession skipped here because the QDBusObjectPath wouldn't be usable in QML.
    // destroySession skipped too for symmetry
    bool createService(
            const QVariantMap &settings,
            const QString &tech = QString(),
            const QString &service = QString(),
            const QString &device = QString());
    QString createServiceSync(
            const QVariantMap &settings,
            const QString &tech = QString(),
            const QString &service = QString(),
            const QString &device = QString());

Q_SIGNALS:
    void availabilityChanged();
    void stateChanged();
    void offlineModeChanged();
    void inputRequestTimeoutChanged();
    void defaultRouteChanged();
    void connectedWifiChanged();
    void connectedEthernetChanged();
    void sessionModeChanged();
    void servicesEnabledChanged();
    void technologiesEnabledChanged();
    void validChanged();
    void connectedChanged();
    void connectingChanged();
    void connectingWifiChanged();

    // following not property notifiers
    void technologiesChanged();
    void servicesChanged();
    void savedServicesChanged();
    void wifiServicesChanged();
    void cellularServicesChanged();
    void ethernetServicesChanged();
    void availableServicesChanged();
    void servicesListChanged();
    void serviceAdded(const QString &servicePath);
    void serviceRemoved(const QString &servicePath);
    void serviceCreated(const QString &servicePath);
    void serviceCreationFailed(const QString &error);

private:
    QSharedPointer<NetworkManager> m_sharedInstance;
};

#endif
