/*   -*- Mode: C++ -*-
 * meegotouchcp-connman - connectivity plugin for duicontrolpanel
 * Copyright © 2010, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
#ifndef DEBUG_H
#define DEBUG_H

#include <QString>

#define STR(qstring) qstring.toLatin1().constData()

  #include <QtDebug>
  #define MDEBUG(...) qDebug(__VA_ARGS__)
  #define MWARNING(...) qWarning(__VA_ARGS__)
  #define MCRITICAL(...) qCritical(__VA__ARGS__)

#endif //DEBUG_H
