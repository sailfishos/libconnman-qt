/*
 * Copyright © 2012-2017 Jolla Ltd.
 * Contact: Slava Monich <slava.monich@jolla.com>
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0. The full text of the Apache License
 * is at http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef LIBCONNMAN_PRIVATE_H
#define LIBCONNMAN_PRIVATE_H

#include "commondbustypes.h"
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(lcConnman)

#define CONNMAN_SERVICE QLatin1String("net.connman")

class ConnmanError {
public:
    static const QString Failed;
    static const QString InvalidArguments;
    static const QString PermissionDenied;
    static const QString PassphraseRequired;
    static const QString NotRegistered;
    static const QString NotUnique;
    static const QString NotSupported;
    static const QString NotImplemented;
    static const QString NotFound;
    static const QString NoCarrier;
    static const QString InProgress;
    static const QString AlreadyExists;
    static const QString AlreadyEnabled;
    static const QString AlreadyDisabled;
    static const QString AlreadyConnected;
    static const QString NotConnected;
    static const QString OperationAborted;
    static const QString OperationTimeout;
    static const QString InvalidService;
    static const QString InvalidProperty;
};

class ConnmanState {
public:
    static const QString Idle;
    static const QString Association;
    static const QString Configuration;
    static const QString Ready;
    static const QString Online;
    static const QString Disconnect;
    static const QString Failure;

    static inline bool connecting(QString state)
        { return (state == Association || state == Configuration); }
    static inline bool connected(QString state)
        { return (state == Online || state == Ready); }
};

#endif // LIBCONNMAN_PRIVATE_H
