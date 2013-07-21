/*
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include <QtDBus/QDBusConnection>

#include "counter.h"
#include "networkmanager.h"

static const char COUNTER_PATH[] = "/ConnectivityCounter";

Counter::Counter(QObject *parent) :
    QObject(parent),
    m_manager(NetworkManagerFactory::createInstance()),
    bytesInHome(0),
    bytesOutHome(0),
    secondsOnlineHome(0),
    bytesInRoaming(0),
    bytesOutRoaming(0),
    secondsOnlineRoaming(0),
    roamingEnabled(0),
    currentInterval(1),
    currentAccuracy(1024),
    isRunning(0)
{
    new CounterAdaptor(this);
    QDBusConnection::systemBus().registerObject(COUNTER_PATH, this);
}

Counter::~Counter()
{
}

void Counter::serviceUsage(const QString &servicePath, const QVariantMap &counters,  bool roaming)
{
    Q_EMIT counterChanged(servicePath, counters, roaming);

    if (roaming != roamingEnabled) {
        Q_EMIT roamingChanged(roaming);
        roamingEnabled = roaming;
    }

    quint32 rxbytes = counters["RX.Bytes"].toUInt();
    quint32 txbytes = counters["TX.Bytes"].toUInt();
    quint32 time = counters["Time"].toUInt();

    if (rxbytes != 0)
        Q_EMIT bytesReceivedChanged(rxbytes);
    if ( txbytes!= 0)
        Q_EMIT bytesTransmittedChanged(txbytes);
    if (time!= 0)
        Q_EMIT secondsOnlineChanged(time);

    if (roaming) {
        if (rxbytes != 0) {
            bytesInRoaming = rxbytes;
        }
        if (txbytes != 0) {
            bytesOutRoaming = txbytes;
        }
        if (time != 0) {
            secondsOnlineRoaming = time;
        }
    } else {
        if (rxbytes != 0) {
            bytesInHome = rxbytes;
        }
        if (txbytes != 0) {
            bytesOutHome = txbytes;
        }
        if (time != 0) {
            secondsOnlineHome = time;
        }
    }
}

void Counter::release()
{
}

bool Counter::roaming() const
{
    return roamingEnabled;
}

quint32 Counter::bytesReceived() const
{
    if (roamingEnabled) {
        return bytesInRoaming;
    } else {
        return bytesInHome;
    }
    return 0;
}

quint32 Counter::bytesTransmitted() const
{
    if (roamingEnabled) {
        return bytesOutRoaming;
    } else {
        return bytesOutHome;
    }
    return 0;
}

quint32 Counter::secondsOnline() const
{
    if (roamingEnabled) {
        return secondsOnlineRoaming;
    } else {
        return secondsOnlineHome;
    }
    return 0;
}

/*
 *The accuracy value is in kilo-bytes. It defines
            the update threshold.

Changing the accuracy will reset the counters, as it will
need to be re registered from the manager.
*/
void Counter::setAccuracy(quint32 accuracy)
{
    currentAccuracy = accuracy;
    reRegister();
    Q_EMIT accuracyChanged(accuracy);
}

quint32 Counter::accuracy() const
{
    return currentAccuracy;
}

/*
 *The interval value is in seconds.
Changing the accuracy will reset the counters, as it will
need to be re registered from the manager.
*/
void Counter::setInterval(quint32 interval)
{
    currentInterval = interval;
    reRegister();
    Q_EMIT intervalChanged(interval);
}

quint32 Counter::interval() const
{
    return currentInterval;
}

void Counter::reRegister()
{
    if (m_manager->isAvailable()) {
        m_manager->unregisterCounter(QString(COUNTER_PATH));
        m_manager->registerCounter(QString(COUNTER_PATH),currentAccuracy,currentInterval);
    }
}

void Counter::setRunning(bool on)
{
    if (on) {
        if (m_manager->isAvailable()) {
            m_manager->registerCounter(QString(COUNTER_PATH),currentAccuracy,currentInterval);
            isRunning = true;
            Q_EMIT runningChanged(isRunning);
        }
    } else {
        if (m_manager->isAvailable()) {
            m_manager->unregisterCounter(QString(COUNTER_PATH));
            isRunning = false;
            Q_EMIT runningChanged(isRunning);
        }
    }
}

bool Counter::running() const
{
    return isRunning;
}


/*
 *This is the dbus adaptor to the connman interface
 **/
CounterAdaptor::CounterAdaptor(Counter* parent)
  : QDBusAbstractAdaptor(parent),
    m_counter(parent)
{
}

CounterAdaptor::~CounterAdaptor()
{
}

void CounterAdaptor::Release()
{
     m_counter->release();
}

void CounterAdaptor::Usage(const QDBusObjectPath &service_path,
                           const QVariantMap &home,
                           const QVariantMap &roaming)
{
    if (!home.isEmpty()) {
        // home
        m_counter->serviceUsage(service_path.path(), home, false);
    }
    if (!roaming.isEmpty()) {
        //roaming
        m_counter->serviceUsage(service_path.path(), roaming, true);
    }
}

