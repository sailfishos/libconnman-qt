/*
 * Copyright © 2010, Intel Corporation.
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "commondbustypes.h"

#include "networktechnology.h"
#include "networkservice.h"
#include <QtDBus>

class Manager;
class NetworkManager;

class NetworkManagerFactory
{
public:
    NetworkManagerFactory() {}

    static NetworkManager* createInstance();
};

class NetworkManager : public QObject
{
    Q_OBJECT;

    Q_PROPERTY(bool available READ isAvailable NOTIFY availabilityChanged);
    Q_PROPERTY(QString state READ state NOTIFY stateChanged);
    Q_PROPERTY(bool offlineMode READ offlineMode WRITE setOfflineMode NOTIFY offlineModeChanged);
    Q_PROPERTY(NetworkService* defaultRoute READ defaultRoute NOTIFY defaultRouteChanged);

public:
    NetworkManager(QObject* parent=0);
    virtual ~NetworkManager();

    bool isAvailable() const;

    NetworkTechnology* getTechnology(const QString &type) const;
    const QVector<NetworkTechnology *> getTechnologies() const;
    const QVector<NetworkService*> getServices(const QString &tech = "") const;

    const QString state() const;
    bool offlineMode() const;
    NetworkService* defaultRoute() const;

public slots:
    void setOfflineMode(const bool &offlineMode);
    void registerAgent(const QString &path);
    void unregisterAgent(const QString &path);

signals:
    void availabilityChanged(bool available);

    void stateChanged(const QString &state);
    void offlineModeChanged(bool offlineMode);
    void technologiesChanged();
    void servicesChanged();
    void defaultRouteChanged(NetworkService* defaultRoute);

private:
    Manager *m_manager;

    QDBusPendingCallWatcher *m_getPropertiesWatcher;
    QDBusPendingCallWatcher *m_getTechnologiesWatcher;
    QDBusPendingCallWatcher *m_getServicesWatcher;

    /* Contains all property related to this net.connman.Manager object */
    QVariantMap m_propertiesCache;

    /* Not just for cache, but actual containers of Network* type objects */
    QHash<QString, NetworkTechnology *> m_technologiesCache;
    QHash<QString, NetworkService *> m_servicesCache;

    /* This is for sorting purpose only, never delete an object from here */
    QVector<NetworkService *> m_servicesOrder;

    /* This variable is used just to send signal if changed */
    NetworkService* m_defaultRoute;

    QDBusServiceWatcher *watcher;

    static const QString State;
    static const QString OfflineMode;

    bool m_available;

private slots:
    void connectToConnman(QString = "");
    void disconnectFromConnman(QString = "");
    void connmanUnregistered(QString = "");
    void getPropertiesReply(QDBusPendingCallWatcher *call);
    void getTechnologiesReply(QDBusPendingCallWatcher *call);
    void getServicesReply(QDBusPendingCallWatcher *call);
    void propertyChanged(const QString &name, const QDBusVariant &value);
    void updateServices(const ConnmanObjectList &changed, const QList<QDBusObjectPath> &removed);
    void updateDefaultRoute(NetworkService* defaultRoute);
    void technologyAdded(const QDBusObjectPath &technology, const QVariantMap &properties);
    void technologyRemoved(const QDBusObjectPath &technology);


private:
    Q_DISABLE_COPY(NetworkManager);
};

#endif //NETWORKMANAGER_H
