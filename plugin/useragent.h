/*
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#ifndef USERAGENT_H
#define USERAGENT_H

#include <QDBusAbstractAdaptor>
#include "technologymodel.h"

class UserAgent : public QDBusAbstractAdaptor
{
    Q_OBJECT;
    Q_CLASSINFO("D-Bus Interface", "net.connman.Agent");

public:
    UserAgent(TechnologyModel* parent);
    virtual ~UserAgent();

public slots:
    void Release();
    void ReportError(const QDBusObjectPath &service_path, const QString &error);
    void RequestBrowser(const QDBusObjectPath &service_path, const QString &url);
    Q_NOREPLY void RequestInput(const QDBusObjectPath &service_path,
                                const QVariantMap &fields,
                                const QDBusMessage &message);
    void Cancel();

private:
    TechnologyModel* m_model;
};

#endif // USERAGENT_H
