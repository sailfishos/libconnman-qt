/*
 * Copyright © 2010, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "clockmodel.h"
#include "connman_clock_interface.h"

#define CONNMAN_SERVICE "net.connman"
#define CONNMAN_CLOCK_INTERFACE CONNMAN_SERVICE ".Clock"

#define SET_CONNMAN_PROPERTY(key, val) \
        if (!d_ptr->mClockProxy) { \
            qCritical("ClockModel: SetProperty: not connected to connman"); \
        } else { \
            QDBusPendingReply<> reply = d_ptr->mClockProxy->SetProperty(key, QDBusVariant(val)); \
            QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this); \
            connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)), \
                    this, SLOT(setPropertyFinished(QDBusPendingCallWatcher*))); \
        }

class ClockModelPrivate
{
public:
    ClockModelPrivate();

    NetConnmanClockInterface *mClockProxy;
    QString mTimezone;
    QString mTimezoneUpdates;
    QString mTimeUpdates;
    QStringList mTimeservers;
};

ClockModelPrivate::ClockModelPrivate()
    : mClockProxy(nullptr)
{
}

ClockModel::ClockModel()
    : d_ptr(new ClockModelPrivate)
{
    QTimer::singleShot(0, this, SLOT(connectToConnman()));
}

ClockModel::~ClockModel()
{
    delete d_ptr;
    d_ptr = nullptr;
}

void ClockModel::connectToConnman()
{
    if (d_ptr->mClockProxy && d_ptr->mClockProxy->isValid())
        return;

    d_ptr->mClockProxy = new NetConnmanClockInterface(CONNMAN_SERVICE, "/", QDBusConnection::systemBus(), this);

    if (!d_ptr->mClockProxy->isValid()) {
        qCritical("ClockModel: unable to connect to connman");
        delete d_ptr->mClockProxy;
        d_ptr->mClockProxy = nullptr;
        return;
    }

    QDBusPendingReply<QVariantMap> reply = d_ptr->mClockProxy->GetProperties();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
            this, SLOT(getPropertiesFinished(QDBusPendingCallWatcher*)));

    connect(d_ptr->mClockProxy, SIGNAL(PropertyChanged(const QString&, const QDBusVariant&)),
            this, SLOT(propertyChanged(const QString&, const QDBusVariant&)));
}

void ClockModel::getPropertiesFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<QVariantMap> reply = *call;

    if (reply.isError()) {
        qCritical() << "ClockModel: getProperties: " << reply.error().name() << reply.error().message();
    } else {
        QVariantMap properties = reply.value();

        if (properties.contains(QLatin1String("Timezone"))) {
            d_ptr->mTimezone = properties.value("Timezone").toString();
            Q_EMIT timezoneChanged();
        }
        if (properties.contains(QLatin1String("TimezoneUpdates"))) {
            d_ptr->mTimezoneUpdates = properties.value("TimezoneUpdates").toString();
            Q_EMIT timezoneUpdatesChanged();
        }
        if (properties.contains(QLatin1String("TimeUpdates"))) {
            d_ptr->mTimeUpdates = properties.value("TimeUpdates").toString();
            Q_EMIT timeUpdatesChanged();
        }
        if (properties.contains(QLatin1String("Timeservers"))) {
            d_ptr->mTimeservers = properties.value("Timeservers").toStringList();
            Q_EMIT timeserversChanged();
        }
    }
    call->deleteLater();
}

void ClockModel::setPropertyFinished(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<> reply = *call;
    if (reply.isError()) {
        qCritical() << "ClockModel: setProperty: " << reply.error().name() << reply.error().message();
    }
    call->deleteLater();
}

void ClockModel::propertyChanged(const QString &name, const QDBusVariant &value)
{
    if (name == "Timezone") {
        d_ptr->mTimezone = value.variant().toString();
        Q_EMIT timezoneChanged();
    } else if (name == "TimezoneUpdates") {
        d_ptr->mTimezoneUpdates = value.variant().toString();
        Q_EMIT timezoneUpdatesChanged();
    } else if (name == "TimeUpdates") {
        d_ptr->mTimeUpdates = value.variant().toString();
        Q_EMIT timeUpdatesChanged();
    } else if (name == "Timeservers") {
        d_ptr->mTimeservers = value.variant().toStringList();
        Q_EMIT timeserversChanged();
    }
}

QString ClockModel::timezone() const
{
    return d_ptr->mTimezone;
}

void ClockModel::setTimezone(const QString &val)
{
    SET_CONNMAN_PROPERTY("Timezone", val);
}

QString ClockModel::timezoneUpdates() const
{
    return d_ptr->mTimezoneUpdates;
}

void ClockModel::setTimezoneUpdates(const QString &val)
{
    SET_CONNMAN_PROPERTY("TimezoneUpdates", val);
}

QString ClockModel::timeUpdates() const
{
    return d_ptr->mTimeUpdates;
}

void ClockModel::setTimeUpdates(const QString &val)
{
    SET_CONNMAN_PROPERTY("TimeUpdates", val);
}

QStringList ClockModel::timeservers() const
{
    return d_ptr->mTimeservers;
}

void ClockModel::setTimeservers(const QStringList &val)
{
    SET_CONNMAN_PROPERTY("Timeservers", val);
}

void ClockModel::setDate(QDate date)
{
    QDateTime toDate(date, QTime::currentTime());
    quint64 secsSinceEpoch = (quint64)toDate.toMSecsSinceEpoch() / 1000;
    SET_CONNMAN_PROPERTY("Time", secsSinceEpoch);
}

void ClockModel::setTime(QTime time)
{
    QDateTime toDate(QDate::currentDate(), time);
    quint64 secsSinceEpoch = (quint64)toDate.toMSecsSinceEpoch() / 1000;
    SET_CONNMAN_PROPERTY("Time", secsSinceEpoch);
}
