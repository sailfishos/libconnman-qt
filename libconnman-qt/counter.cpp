/*
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include <QtDBus/QDBusConnection>
#if (QT_VERSION >= QT_VERSION_CHECK(5,10,0))
#include <QRandomGenerator>
#endif

#include "counter.h"
#include "networkmanager.h"

class CounterPrivate
{
public:
    CounterPrivate();

    QSharedPointer<NetworkManager> m_manager;

    quint64 bytesInHome;
    quint64 bytesOutHome;
    quint32 secondsOnlineHome;

    quint64 bytesInRoaming;
    quint64 bytesOutRoaming;
    quint32 secondsOnlineRoaming;

    bool roamingEnabled;
    quint32 currentInterval;
    quint32 currentAccuracy;

    QString counterPath;
    bool shouldBeRunning;

    bool registered;
};

CounterPrivate::CounterPrivate()
    : m_manager(NetworkManager::sharedInstance())
    , bytesInHome(0)
    , bytesOutHome(0)
    , secondsOnlineHome(0)
    , bytesInRoaming(0)
    , bytesOutRoaming(0)
    , secondsOnlineRoaming(0)
    , roamingEnabled(false)
    , currentInterval(1)
    , currentAccuracy(1024)
    , shouldBeRunning(false)
    , registered(false)
{
}

Counter::Counter(QObject *parent)
    : QObject(parent)
    , d_ptr(new CounterPrivate)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5,10,0))
    quint32 randomValue = QRandomGenerator::global()->generate();
#else
    QTime time = QTime::currentTime();
    qsrand((uint)time.msec());
    int randomValue = qrand();
#endif

    //this needs to be unique so we can use more than one at a time with different processes
    d_ptr->counterPath = "/ConnectivityCounter" + QString::number(randomValue);

    new CounterAdaptor(this);
    if (!QDBusConnection::systemBus().registerObject(d_ptr->counterPath, this))
        qWarning("Could not register DBus object on %s", qPrintable(d_ptr->counterPath));

    connect(d_ptr->m_manager.data(), &NetworkManager::availabilityChanged,
            this, &Counter::updateCounterAgent);
}

Counter::~Counter()
{
    if (d_ptr->registered)
        d_ptr->m_manager->unregisterCounter(d_ptr->counterPath);

    delete d_ptr;
    d_ptr = nullptr;
}

void Counter::serviceUsage(const QString &servicePath, const QVariantMap &counters, bool roaming)
{
    Q_EMIT counterChanged(servicePath, counters, roaming);

    if (roaming != d_ptr->roamingEnabled) {
        d_ptr->roamingEnabled = roaming;
        Q_EMIT roamingChanged(roaming);
    }

    quint64 rxbytes = counters["RX.Bytes"].toULongLong();
    quint64 txbytes = counters["TX.Bytes"].toULongLong();
    quint32 time = counters["Time"].toUInt();

    if (roaming) {
        if (rxbytes != 0) {
            d_ptr->bytesInRoaming = rxbytes;
        }
        if (txbytes != 0) {
            d_ptr->bytesOutRoaming = txbytes;
        }
        if (time != 0) {
            d_ptr->secondsOnlineRoaming = time;
        }
    } else {
        if (rxbytes != 0) {
            d_ptr->bytesInHome = rxbytes;
        }
        if (txbytes != 0) {
            d_ptr->bytesOutHome = txbytes;
        }
        if (time != 0) {
            d_ptr->secondsOnlineHome = time;
        }
    }

    if (rxbytes != 0)
        Q_EMIT bytesReceivedChanged(rxbytes);
    if (txbytes != 0)
        Q_EMIT bytesTransmittedChanged(txbytes);
    if (time != 0)
        Q_EMIT secondsOnlineChanged(time);
}

void Counter::release()
{
    d_ptr->registered = false;
    Q_EMIT runningChanged(d_ptr->registered);
}

bool Counter::roaming() const
{
    return d_ptr->roamingEnabled;
}

quint64 Counter::bytesReceived() const
{
    if (d_ptr->roamingEnabled) {
        return d_ptr->bytesInRoaming;
    } else {
        return d_ptr->bytesInHome;
    }
}

quint64 Counter::bytesTransmitted() const
{
    if (d_ptr->roamingEnabled) {
        return d_ptr->bytesOutRoaming;
    } else {
        return d_ptr->bytesOutHome;
    }
}

quint32 Counter::secondsOnline() const
{
    if (d_ptr->roamingEnabled) {
        return d_ptr->secondsOnlineRoaming;
    } else {
        return d_ptr->secondsOnlineHome;
    }
}

/*
 *The accuracy value is in kilo-bytes. It defines
            the update threshold.

Changing the accuracy will reset the counters, as it will
need to be re registered from the manager.
*/
void Counter::setAccuracy(quint32 accuracy)
{
    if (d_ptr->currentAccuracy == accuracy)
        return;

    d_ptr->currentAccuracy = accuracy;
    Q_EMIT accuracyChanged(accuracy);
    updateCounterAgent();
}

quint32 Counter::accuracy() const
{
    return d_ptr->currentAccuracy;
}

/*
 *The interval value is in seconds.
Changing the accuracy will reset the counters, as it will
need to be re registered from the manager.
*/
void Counter::setInterval(quint32 interval)
{
    if (d_ptr->currentInterval == interval)
        return;

    d_ptr->currentInterval = interval;
    Q_EMIT intervalChanged(interval);
    updateCounterAgent();
}

quint32 Counter::interval() const
{
    return d_ptr->currentInterval;
}

void Counter::setRunning(bool on)
{
    if (d_ptr->shouldBeRunning == on)
        return;

    d_ptr->shouldBeRunning = on;
    updateCounterAgent();
}

void Counter::updateCounterAgent()
{
    if (!d_ptr->m_manager->isAvailable()) {
        if (d_ptr->registered) {
            d_ptr->registered = false;
            Q_EMIT runningChanged(d_ptr->registered);
        }
        return;
    }

    if (d_ptr->registered) {
        d_ptr->m_manager->unregisterCounter(d_ptr->counterPath);
        if (!d_ptr->shouldBeRunning) {
            d_ptr->registered = false;
            Q_EMIT runningChanged(d_ptr->registered);
            return;
        }
    }

    if (d_ptr->shouldBeRunning) {
        d_ptr->m_manager->registerCounter(d_ptr->counterPath, d_ptr->currentAccuracy, d_ptr->currentInterval);
        if (!d_ptr->registered) {
            d_ptr->registered = true;
            Q_EMIT runningChanged(d_ptr->registered);
        }
    }
}

bool Counter::running() const
{
    return d_ptr->registered;
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

