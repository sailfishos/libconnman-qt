/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2020 Jolla Ltd.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0. The full text of the Apache License
 * is at http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "networktechnology.h"
#include "networkservice.h"
#include <QtDBus>
#include <QSharedPointer>

class NetworkManager;

// This class is deprecated
class NetworkManagerFactory : public QObject
{
    Q_OBJECT
    Q_PROPERTY(NetworkManager* instance READ instance CONSTANT)

public:
    Q_DECL_DEPRECATED_X("Use NetworkManager::sharedInstance()") static NetworkManager* createInstance();
    // deprecated
    NetworkManager* instance();
};

class NetworkManager : public QObject
{
    Q_OBJECT
    Q_ENUMS(State)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availabilityChanged)
    // deprecated
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool offlineMode READ offlineMode WRITE setOfflineMode NOTIFY offlineModeChanged)
    Q_PROPERTY(NetworkService* defaultRoute READ defaultRoute NOTIFY defaultRouteChanged)
    Q_PROPERTY(NetworkService* connectedWifi READ connectedWifi NOTIFY connectedWifiChanged)
    Q_PROPERTY(NetworkService* connectedEthernet READ connectedEthernet NOTIFY connectedEthernetChanged)
    Q_PROPERTY(bool connectingWifi READ connectingWifi NOTIFY connectingWifiChanged)

    Q_PROPERTY(bool sessionMode READ sessionMode WRITE setSessionMode NOTIFY sessionModeChanged)
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
    enum State {
        UnknownState,
        OfflineState,
        IdleState,
        ReadyState,
        OnlineState
    };

    static const QString WifiTechnologyPath;
    static const QString CellularTechnologyPath;
    static const QString BluetoothTechnologyPath;
    static const QString GpsTechnologyPath;
    static const QString EthernetTechnologyPath;

    Q_DECL_DEPRECATED_X("Use NetworkManager::sharedInstance()") static NetworkManager* instance();

    static QSharedPointer<NetworkManager> sharedInstance();

    NetworkManager(QObject *parent = 0);
    virtual ~NetworkManager();

    bool isAvailable() const;

    // Note: the resulting pointer on getTechnology() and getTechnologies() can get invalid after the next
    // technologiesUpdated signal, so need to be sure to fetch values again and stop using pointers that
    // are no longer available.
    Q_INVOKABLE NetworkTechnology* getTechnology(const QString &type) const;
    QVector<NetworkTechnology *> getTechnologies() const;
    QVector<NetworkService*> getServices(const QString &tech = QString()) const;
    QVector<NetworkService*> getSavedServices(const QString &tech = QString()) const;
    QVector<NetworkService*> getAvailableServices(const QString &tech = QString()) const;
    void removeSavedService(const QString &identifier) const;

    Q_INVOKABLE QStringList servicesList(const QString &tech);
    Q_INVOKABLE QStringList savedServicesList(const QString &tech = QString());
    Q_INVOKABLE QStringList availableServices(const QString &tech = QString());
    Q_INVOKABLE QStringList technologiesList();
    Q_INVOKABLE QString technologyPathForService(const QString &path);
    Q_INVOKABLE QString technologyPathForType(const QString &type);

    // deprecated
    QString state() const;
    bool offlineMode() const;
    NetworkService* defaultRoute() const;
    NetworkService* connectedWifi() const;
    bool connectingWifi() const;

    NetworkService *connectedEthernet() const;

    // deprecated
    bool sessionMode() const;
    uint inputRequestTimeout() const;

    // deprecated
    bool servicesEnabled() const;
    // deprecated
    void setServicesEnabled(bool enabled);

    // deprecated
    bool technologiesEnabled() const;
    // deprecated
    void setTechnologiesEnabled(bool enabled);

    bool isValid() const;

    State globalState() const;
    bool connected() const;
    bool connecting() const;

    Q_INVOKABLE void resetCountersForType(const QString &type);

    QString wifiTechnologyPath() const;
    QString cellularTechnologyPath() const;
    QString bluetoothTechnologyPath() const;
    QString gpsTechnologyPath() const;
    QString ethernetTechnologyPath() const;

public Q_SLOTS:
    void setOfflineMode(bool offlineMode);
    void registerAgent(const QString &path);
    void unregisterAgent(const QString &path);
    void registerCounter(const QString &path, quint32 accuracy, quint32 period);
    void unregisterCounter(const QString &path);
    QDBusObjectPath createSession(const QVariantMap &settings, const QString &sessionNotifierPath);
    void destroySession(const QString &sessionAgentPath);
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
    // deprecated
    void setSessionMode(bool sessionMode);

Q_SIGNALS:
    void availabilityChanged(bool available);
    void stateChanged(const QString &state);
    void globalStateChanged(State state);
    void offlineModeChanged(bool offlineMode);
    void inputRequestTimeoutChanged();
    void technologiesChanged();
    void servicesChanged();
    void savedServicesChanged();
    void wifiServicesChanged();
    void cellularServicesChanged();
    void ethernetServicesChanged();
    void availableServicesChanged();
    void defaultRouteChanged(NetworkService* defaultRoute);
    void connectedWifiChanged();
    void connectedEthernetChanged();
    void sessionModeChanged(bool);
    void servicesListChanged(const QStringList &list);
    void serviceAdded(const QString &servicePath);
    void serviceRemoved(const QString &servicePath);

    void serviceCreated(const QString &servicePath);
    void serviceCreationFailed(const QString &error);

    void servicesEnabledChanged();
    void technologiesEnabledChanged();
    void validChanged();

    void connectedChanged();
    void connectingChanged();
    void connectingWifiChanged();

private:
    typedef bool (*ServiceSelector)(NetworkService*);
    void propertyChanged(const QString &name, const QVariant &value);
    void setConnmanAvailable(bool available);
    bool connectToConnman();
    void disconnectFromConnman();
    QVector<NetworkService*> selectServices(const QStringList &list, const QString &tech) const;
    QVector<NetworkService*> selectServices(const QStringList &list, ServiceSelector selector) const;
    NetworkService* selectDefaultRoute(const QString &path);
    QStringList selectServiceList(const QStringList &list, const QString &tech) const;
    QStringList selectServiceList(const QStringList &list, ServiceSelector selector) const;
    void updateDefaultRoute();

private:
    class Private;
    friend class Private;
    Private *m_priv;

private Q_SLOTS:
    void onConnmanRegistered();
    void onConnmanUnregistered();
    void disconnectTechnologies();
    void setupTechnologies();
    void disconnectServices();
    void setupServices();
    void propertyChanged(const QString &name, const QDBusVariant &value);
    void technologyAdded(const QDBusObjectPath &technology, const QVariantMap &properties);
    void technologyRemoved(const QDBusObjectPath &technology);
    void getPropertiesFinished(QDBusPendingCallWatcher *watcher);
    void getTechnologiesFinished(QDBusPendingCallWatcher *watcher);
    void getServicesFinished(QDBusPendingCallWatcher *watcher);

private:
    Q_DISABLE_COPY(NetworkManager)
};

#endif //NETWORKMANAGER_H
