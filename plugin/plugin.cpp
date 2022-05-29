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
#include "technologymodel.h"
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

class ConnmanPlugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    #ifdef USE_MODULE_PREFIX
        Q_PLUGIN_METADATA(IID "MeeGo.Connman")
    #else
        Q_PLUGIN_METADATA(IID "Connman")
    #endif

public:
    void registerTypes(const char *uri);

    void initializeEngine(QQmlEngine *engine, const char *uri);
};

void ConnmanPlugin::registerTypes(const char *uri)
{
    // @uri MeeGo.Connman

    qmlRegisterType<NetworkService>(uri,0,2,"NetworkService");
    qmlRegisterType<TechnologyModel>(uri,0,2,"TechnologyModel");
    qmlRegisterType<SavedServiceModel>(uri,0,2,"SavedServiceModel");
    qmlRegisterType<UserAgent>(uri,0,2,"UserAgent");
    qmlRegisterType<ClockModel>(uri,0,2,"ClockModel");
    qmlRegisterType<NetworkSession>(uri,0,2,"NetworkSession");
    qmlRegisterType<NetworkManager>(uri,0,2,"NetworkManager");
    qmlRegisterType<NetworkManagerFactory>(uri,0,2,"NetworkManagerFactory");
    qmlRegisterType<NetworkTechnology>(uri,0,2,"NetworkTechnology");
    qmlRegisterType<Counter>(uri,0,2,"NetworkCounter");
    qmlRegisterSingletonType<VpnManager>(uri,0,2,"VpnManager", singleton_api_factory<VpnManager>);
    qmlRegisterType<VpnConnection>(uri,0,2,"VpnConnection");
    qmlRegisterSingletonType<VpnModel>(uri, 0, 2, "VpnModel", singleton_api_factory<VpnModel>);
}

void ConnmanPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(uri);
    Q_UNUSED(engine);
}

#include "plugin.moc"
