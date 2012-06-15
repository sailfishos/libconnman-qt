/*
 * Copyright 2011 Intel Corporation.
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "components.h"

#include <networkmanager.h>
#include "networkingmodel.h"
#include <clockmodel.h>


void Components::registerTypes(const char *uri)
{
	qmlRegisterType<NetworkManager>(uri,0,2,"NetworkManager");
	qmlRegisterType<NetworkingModel>(uri,0,2,"NetworkingModel");
	qmlRegisterType<ClockModel>(uri,0,2,"ClockModel");
}

void Components::initializeEngine(QDeclarativeEngine *engine, const char *uri)
{
    Q_UNUSED(uri);
	Q_UNUSED(engine);
}

Q_EXPORT_PLUGIN(Components);
