/*   -*- Mode: C++ -*-
 * meegotouchcp-connman - connectivity plugin for duicontrolpanel
 * Copyright © 2010, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#ifndef COMMONDBUSTYPES_H
#define COMMONDBUSTYPES_H

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QMetaType>
#include <QtDBus/QtDBus>

typedef QMap<QString, QString> StringMap;
Q_DECLARE_METATYPE ( StringMap );

inline void registerCommonDataTypes() {
  qDBusRegisterMetaType<StringMap >();
}

#endif //COMMONDBUSTYPES_H
