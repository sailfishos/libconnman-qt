/*
 * Copyright 2011 Intel Corporation.
 * Copyright (c) 2012 - 2019, Jolla.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include <QtPlugin>

#include <QtQml>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>

#include <networkservice.h>
#include <clockmodel.h>
#include "declarativenetworkmanager.h"
#include "technologyservicemodel.h"
#include "savedservicemodel.h"
#include "useragent.h"
#include "networksession.h"
#include "counter.h"
#include "vpnmanager.h"
#include "vpnconnection.h"
#include "vpnmodel.h"

template<typename T>
static QObject *singleton_api_factory(QQmlEngine *, QJSEngine *)
{
    return new T;
}

class ConnmanApi: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString wifiTechnologyPath READ wifiTechnologyPath CONSTANT)
    Q_PROPERTY(QString cellularTechnologyPath READ cellularTechnologyPath CONSTANT)
    Q_PROPERTY(QString bluetoothTechnologyPath READ bluetoothTechnologyPath CONSTANT)
    Q_PROPERTY(QString gpsTechnologyPath READ gpsTechnologyPath CONSTANT)
    Q_PROPERTY(QString ethernetTechnologyPath READ ethernetTechnologyPath CONSTANT)

public:
    ConnmanApi(QObject *parent = nullptr) : QObject(parent) {}

    QString wifiTechnologyPath() { return NetworkManager::WifiTechnologyPath; }
    QString cellularTechnologyPath() { return NetworkManager::CellularTechnologyPath; }
    QString bluetoothTechnologyPath() { return NetworkManager::BluetoothTechnologyPath; }
    QString gpsTechnologyPath() { return NetworkManager::GpsTechnologyPath; }
    QString ethernetTechnologyPath() { return NetworkManager::EthernetTechnologyPath; }
};

class ConnmanPlugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "Connman")

public:
    void registerTypes(const char *uri);

    void initializeEngine(QQmlEngine *engine, const char *uri);
};

void ConnmanPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QLatin1String("MeeGo.Connman") || uri == QLatin1String("Connman"));
    if (QLatin1String(uri) == QLatin1String("MeeGo.Connman")) {
        qWarning() << "MeeGo.Connman QML module name is deprecated and subject for removal. Please adapt code to \"import Connman\".";
    }

    qmlRegisterSingletonType<ConnmanApi>(uri, 0, 2, "Connman", singleton_api_factory<ConnmanApi>);
    qmlRegisterType<NetworkService>(uri, 0, 2, "NetworkService");
    qmlRegisterType<TechnologyServiceModel>(uri, 0, 2, "TechnologyServiceModel");
    qmlRegisterType<TechnologyModel>(uri, 0, 2, "TechnologyModel");
    qmlRegisterType<SavedServiceModel>(uri, 0, 2, "SavedServiceModel");
    qmlRegisterType<UserAgent>(uri, 0, 2, "UserAgent");
    qmlRegisterType<ClockModel>(uri, 0, 2, "ClockModel");
    qmlRegisterType<NetworkSession>(uri, 0, 2, "NetworkSession");
    qmlRegisterType<DeclarativeNetworkManager>(uri, 0, 2, "NetworkManager");
    qmlRegisterType<DeclarativeNetworkManagerFactory>(uri, 0, 2, "NetworkManagerFactory");
    qmlRegisterType<NetworkTechnology>(uri, 0, 2, "NetworkTechnology");
    qmlRegisterType<Counter>(uri, 0, 2, "NetworkCounter");
    qmlRegisterSingletonType<VpnManager>(uri, 0, 2, "VpnManager", singleton_api_factory<VpnManager>);
    qmlRegisterType<VpnConnection>(uri, 0, 2, "VpnConnection");
    qmlRegisterSingletonType<VpnModel>(uri, 0, 2, "VpnModel", singleton_api_factory<VpnModel>);
}

void ConnmanPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(uri);
    Q_UNUSED(engine);
}

#include "plugin.moc"
