/*
 * Copyright © 2010, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "timezonemodel.h"

#define CONNMAN_SERVICE "net.connman"
#define CONNMAN_CLOCK_INTERFACE CONNMAN_SERVICE ".Clock"

TimezoneModel::TimezoneModel() :
    mClockProxy(0)
{
    QTimer::singleShot(0,this,SLOT(connectToConnman()));
}

void TimezoneModel::connectToConnman()
{
    if (mClockProxy && mClockProxy->isValid())
        return;

    mClockProxy = new ClockProxy(CONNMAN_SERVICE, "/", QDBusConnection::systemBus(), this);

    if (!mClockProxy->isValid()) {
        qCritical("TimezoneModel: unable to connect to connman");
        delete mClockProxy;
        mClockProxy = NULL;
        return;
    }

    QDBusPendingReply<QVariantMap> reply = mClockProxy->GetProperties();
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher,
            SIGNAL(finished(QDBusPendingCallWatcher*)),
            this,
            SLOT(getPropertiesFinished(QDBusPendingCallWatcher*)));
    connect(mClockProxy,
            SIGNAL(PropertyChanged(const QString&, const QDBusVariant&)),
            this,
            SLOT(propertyChanged(const QString&, const QDBusVariant&)));
}

void TimezoneModel::getPropertiesFinished(QDBusPendingCallWatcher *call)
{
	QDBusPendingReply<QVariantMap> reply = *call;
	if (reply.isError()) {
		qCritical() << "TimezoneModel: getProperties: " << reply.error().name() << reply.error().message();
	} else {
        QVariantMap properties = reply.value();
        // sometimes the Timezone property doesn't exist
        //Q_ASSERT(properties.contains("Timezone"));
        //Q_ASSERT(properties.contains("TimezoneUpdates"));
        //Q_ASSERT(properties.value("Timezone").type() == QVariant::String);
        //Q_ASSERT(properties.value("TimezoneUpdates").type() == QVariant::String);
        mTimezone = properties.value("Timezone").toString();
        mTimezoneUpdates = properties.value("TimezoneUpdates").toString();
        emit timezoneChanged();
        emit timezoneUpdatesChanged();
    }
    call->deleteLater();
}

void TimezoneModel::setPropertyFinished(QDBusPendingCallWatcher *call)
{
	QDBusPendingReply<> reply = *call;
	if (reply.isError()) {
		qCritical() << "TimezoneModel: setProperty: " << reply.error().name() << reply.error().message();
    }
    call->deleteLater();
}

void TimezoneModel::propertyChanged(const QString &name, const QDBusVariant &value)
{
    if (name == "Timezone") {
        Q_ASSERT(value.variant().type() == QVariant::String);
        mTimezone = value.variant().toString();
        emit timezoneChanged();
    } else if (name == "TimezoneUpdates") {
        Q_ASSERT(value.variant().type() == QVariant::String);
        mTimezoneUpdates = value.variant().toString();
        emit timezoneUpdatesChanged();
    }
}

QString TimezoneModel::timezone() const
{
    return mTimezone;
}

void TimezoneModel::setTimezone(const QString &val)
{
    if (mClockProxy) {
        QDBusPendingReply<> reply = mClockProxy->SetProperty("Timezone", QDBusVariant(val));
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                this,
                SLOT(setPropertyFinished(QDBusPendingCallWatcher*)));
    } else {
        qCritical("TimezoneModel::setTimezone: not connected to connman");
    }
}

QString TimezoneModel::timezoneUpdates() const
{
    return mTimezoneUpdates;
}

void TimezoneModel::setTimezoneUpdates(const QString &val)
{
    if (mClockProxy) {
        QDBusPendingReply<> reply = mClockProxy->SetProperty("TimezoneUpdates", QDBusVariant(val));
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
        connect(watcher,
                SIGNAL(finished(QDBusPendingCallWatcher*)),
                this,
                SLOT(setPropertyFinished(QDBusPendingCallWatcher*)));
    } else {
        qCritical("TimezoneModel::setTimezoneUpdates: not connected to connman");
    }
}
