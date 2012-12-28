/*
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "session.h"

/*
 * Implementation of interface class Session
 */

Session::Session(const QString &path, QObject *parent)
    : QDBusAbstractInterface(staticConnmanService(), path, staticInterfaceName(), QDBusConnection::systemBus(), parent)
{
    qDebug() << Q_FUNC_INFO;
}

Session::~Session()
{
}

