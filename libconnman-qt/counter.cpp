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
    m_manager(NetworkManagerFactory::createInstance())
{
    new CounterAdaptor(this);
    QDBusConnection::systemBus().registerObject(COUNTER_PATH, this);

    if (m_manager->isAvailable()) {
        m_manager->registerCounter(QString(COUNTER_PATH),1024,5);
    }
}

Counter::~Counter()
{
}

void Counter::serviceUsage(const QString &servicePath, const QVariantMap &counters,  bool roaming)
{
    latestCounts.insert(servicePath, counters);
    Q_EMIT counterChanged(servicePath, counters, roaming);
}

QVariantMap Counter::latestStats(const QString &servicePath)
{
    return latestCounts[servicePath];
}


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
}

void CounterAdaptor::Usage(const QDBusObjectPath &service_path,
                           const QVariantMap &home,
                           const QVariantMap &roaming)
{
    if (roaming.isEmpty()) {
        // home
        m_counter->serviceUsage(service_path.path(), home, false);
    } else if (home.isEmpty()) {
        //roaming
        m_counter->serviceUsage(service_path.path(), roaming, true);
    }
}

