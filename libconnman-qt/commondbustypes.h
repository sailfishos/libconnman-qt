/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2017 Jolla Ltd.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0. The full text of the Apache License
 * is at http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef COMMONDBUSTYPES_H
#define COMMONDBUSTYPES_H

#include <QtCore/QMap>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QMetaType>
#include <QtDBus/QtDBus>
#include <QtDBus/QDBusObjectPath>

typedef QMap<QString, QString> StringMap;
Q_DECLARE_METATYPE ( StringMap )

typedef QPair<QString, QString> StringPair;
Q_DECLARE_METATYPE(StringPair)

typedef QVector<StringPair> StringPairArray;
Q_DECLARE_METATYPE(StringPairArray)

// TODO: re-implement with better interface i.e. "const QString path() const" instead of objpath
struct ConnmanObject {
    QDBusObjectPath objpath;
    QVariantMap properties;
};
Q_DECLARE_METATYPE ( ConnmanObject )
QDBusArgument &operator<<(QDBusArgument &argument, const ConnmanObject &obj);
const QDBusArgument &operator>>(const QDBusArgument &argument, ConnmanObject &obj);

typedef QList<ConnmanObject> ConnmanObjectList;
Q_DECLARE_METATYPE ( ConnmanObjectList )

void registerCommonDataTypes();

#define CONNMAN_SERVICE QLatin1String("net.connman")

#endif //COMMONDBUSTYPES_H
