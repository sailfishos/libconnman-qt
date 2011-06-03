/*
 * Copyright 2011 Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at 
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "components.h"

#include <networklist.h>
#include <networkitem.h>
#include <clockmodel.h>


void Components::registerTypes(const char *uri)
{
	qmlRegisterType<NetworkListModel>(uri,0,1,"NetworkListModel");
	qmlRegisterType<NetworkItemModel>(uri,0,1,"NetworkItemModel");
	qmlRegisterType<ClockModel>(uri,0,1,"ClockModel");
}

void Components::initializeEngine(QDeclarativeEngine *engine, const char *uri)
{
    Q_UNUSED(uri);
	Q_UNUSED(engine);
}

Q_EXPORT_PLUGIN(Components);
