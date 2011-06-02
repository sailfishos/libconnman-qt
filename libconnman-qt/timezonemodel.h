/*
 * Copyright © 2010, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#ifndef TIMEZONEMODEL_H
#define TIMEZONEMODEL_H

#include "clockproxy.h"

class TimezoneModel : public QObject {
    Q_OBJECT;

    Q_PROPERTY(QString timezone READ timezone WRITE setTimezone NOTIFY timezoneChanged);
    Q_PROPERTY(QString timezoneUpdates READ timezoneUpdates WRITE setTimezoneUpdates NOTIFY timezoneUpdatesChanged);

public:
    TimezoneModel();

public slots:
    QString timezone() const;
    void setTimezone(const QString &val);
    QString timezoneUpdates() const;
    void setTimezoneUpdates(const QString &val);

signals:
    void timezoneChanged();
    void timezoneUpdatesChanged();

private slots:
    void connectToConnman();
    void getPropertiesFinished(QDBusPendingCallWatcher*);
    void setPropertyFinished(QDBusPendingCallWatcher*);
    void propertyChanged(const QString&, const QDBusVariant&);

private:
    ClockProxy *mClockProxy;
    QString mTimezone;
    QString mTimezoneUpdates;

    Q_DISABLE_COPY(TimezoneModel);
};

#endif
