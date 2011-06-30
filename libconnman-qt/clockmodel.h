/*
 * Copyright © 2010, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#ifndef CLOCKMODEL_H
#define CLOCKMODEL_H

#include "clockproxy.h"

class ClockModel : public QObject {
    Q_OBJECT;

    Q_PROPERTY(QString timezone READ timezone WRITE setTimezone NOTIFY timezoneChanged);
    Q_PROPERTY(QString timezoneUpdates READ timezoneUpdates WRITE setTimezoneUpdates NOTIFY timezoneUpdatesChanged);
    Q_PROPERTY(QString timeUpdates READ timeUpdates WRITE setTimeUpdates NOTIFY timeUpdatesChanged);
    Q_PROPERTY(QStringList timeservers READ timeservers WRITE setTimeservers NOTIFY timeserversChanged);

public:
    ClockModel();

public slots:
    QString timezone() const;
    void setTimezone(const QString &val);
    QString timezoneUpdates() const;
    void setTimezoneUpdates(const QString &val);
    QString timeUpdates() const;
    void setTimeUpdates(const QString &val);
    QStringList timeservers() const;
    void setTimeservers(const QStringList &val);

    void setDate(QDate date);
    void setTime(QTime time);

    // helper function for Timepicker
    QTime time(QString h, QString m) { return QTime(h.toInt(), m.toInt()); }

signals:
    void timezoneChanged();
    void timezoneUpdatesChanged();
    void timeUpdatesChanged();
    void timeserversChanged();

private slots:
    void connectToConnman();
    void getPropertiesFinished(QDBusPendingCallWatcher*);
    void setPropertyFinished(QDBusPendingCallWatcher*);
    void propertyChanged(const QString&, const QDBusVariant&);

private:
    ClockProxy *mClockProxy;
    QString mTimezone;
    QString mTimezoneUpdates;
    QString mTimeUpdates;
    QStringList mTimeservers;

    Q_DISABLE_COPY(ClockModel);
};

#endif
