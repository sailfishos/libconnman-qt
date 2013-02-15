/*
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#ifndef COUNTER_H
#define COUNTER_H

#include <QObject>
#include <QVariantMap>
#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>

class NetworkManager;

class Counter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Counter)
public:
    explicit Counter(QObject *parent = 0);
    virtual ~Counter();

    void serviceUsage(const QString &servicePath, const QVariantMap &counters,  bool roaming);
    void secondsOnline(const QString &servicePath);
    QVariantMap latestStats(const QString &servicePath);
    QPair <quint32, quint32> latestBytes(const QString &servicePath);

signals:
   //  "RX.Bytes", "RX.Packets"
   //  "TX.Bytes", "TX.Packets"
   //  "Time"
    void counterChanged(const QString servicePath, const QVariantMap &counters,  bool roaming);
    
public slots:
    
private:
       NetworkManager* m_manager;
       QMap <QString,QVariantMap> latestCounts;

};


class CounterAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT;
    Q_CLASSINFO("D-Bus Interface", "net.connman.Counter");

public:
    explicit CounterAdaptor(Counter* parent);
    virtual ~CounterAdaptor();

public slots:
    void Release();
    void Usage(const QDBusObjectPath &service_path,
                                const QVariantMap &home,
                                const QVariantMap &roaming);

private:
    Counter* m_counter;
    friend class CounterAdaptor;
};
#endif // COUNTER_H
