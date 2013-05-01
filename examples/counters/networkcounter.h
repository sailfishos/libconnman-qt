/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd
** Contact: lorn.potter@jollamobile.com
**
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef NETWORKCOUNTER_H
#define NETWORKCOUNTER_H

#include <QObject>
#include <connman-qt/counter.h>

class NetworkCounter : public QObject
{
    Q_OBJECT
public:
    explicit NetworkCounter(QObject *parent = 0);

signals:

public slots:
    void counterChanged(const QString servicePath, const QVariantMap &counters,  bool roaming);

    void bytesReceivedChanged(quint32 bytesRx);

    void bytesTransmittedChanged(quint32 bytesTx);

    void secondsOnlineChanged(quint32 seconds);

    void roamingChanged(bool roaming);
private:
    Counter *counter;

};

#endif // NETWORKCOUNTER_H
