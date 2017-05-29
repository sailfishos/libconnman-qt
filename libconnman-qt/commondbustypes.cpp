/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2017 Jolla Ltd.
 * Contact: Slava Monich <slava.monich@jolla.com>
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 */

#include "libconnman_p.h"

#define CONNMAN_ERROR "net.connman.Error"

const QString ConnmanError::Failed(CONNMAN_ERROR ".Failed");
const QString ConnmanError::InvalidArguments(CONNMAN_ERROR ".InvalidArguments");
const QString ConnmanError::PermissionDenied(CONNMAN_ERROR ".PermissionDenied");
const QString ConnmanError::PassphraseRequired(CONNMAN_ERROR ".PassphraseRequired");
const QString ConnmanError::NotRegistered(CONNMAN_ERROR ".NotRegistered");
const QString ConnmanError::NotUnique(CONNMAN_ERROR ".NotUnique");
const QString ConnmanError::NotSupported(CONNMAN_ERROR ".NotSupported");
const QString ConnmanError::NotImplemented(CONNMAN_ERROR ".NotImplemented");
const QString ConnmanError::NotFound(CONNMAN_ERROR ".NotFound");
const QString ConnmanError::NoCarrier(CONNMAN_ERROR ".NoCarrier");
const QString ConnmanError::InProgress(CONNMAN_ERROR ".InProgress");
const QString ConnmanError::AlreadyExists(CONNMAN_ERROR ".AlreadyExists");
const QString ConnmanError::AlreadyEnabled(CONNMAN_ERROR ".AlreadyEnabled");
const QString ConnmanError::AlreadyDisabled(CONNMAN_ERROR ".AlreadyDisabled");
const QString ConnmanError::AlreadyConnected(CONNMAN_ERROR ".AlreadyConnected");
const QString ConnmanError::NotConnected(CONNMAN_ERROR ".NotConnected");
const QString ConnmanError::OperationAborted(CONNMAN_ERROR ".OperationAborted");
const QString ConnmanError::OperationTimeout(CONNMAN_ERROR ".OperationTimeout");
const QString ConnmanError::InvalidService(CONNMAN_ERROR ".InvalidService");
const QString ConnmanError::InvalidProperty(CONNMAN_ERROR ".InvalidProperty");

const QString ConnmanState::Idle("idle");
const QString ConnmanState::Association("association");
const QString ConnmanState::Configuration("configuration");
const QString ConnmanState::Ready("ready");
const QString ConnmanState::Online("online");
const QString ConnmanState::Disconnect("disconnect");
const QString ConnmanState::Failure("failure");

// Marshall the ConnmanObject data into a D-Bus argument
QDBusArgument &operator<<(QDBusArgument &argument, const ConnmanObject &obj)
{
    argument.beginStructure();
    argument << obj.objpath << obj.properties;
    argument.endStructure();
    return argument;
}

// Retrieve the ConnmanObject data from the D-Bus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, ConnmanObject &obj)
{
    argument.beginStructure();
    argument >> obj.objpath >> obj.properties;
    argument.endStructure();
    return argument;
}
