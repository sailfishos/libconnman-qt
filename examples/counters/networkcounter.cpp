/****************************************************************************
**
** Copyright (C) 2013 Jolla Ltd
** Contact: lorn.potter@gmail.com
**
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "networkcounter.h"
#include <connman-qt/counter.h>
#include <QtCore>

NetworkCounter::NetworkCounter(QObject *parent) :
    QObject(parent)
{
    counter = new Counter(this);

    connect(counter,SIGNAL(counterChanged(QString,QVariantMap,bool)),this,SLOT(counterChanged(QString,QVariantMap,bool)));
    connect(counter,SIGNAL(bytesReceivedChanged(quint32)),this,SLOT(bytesReceivedChanged(quint32)));
    connect(counter,SIGNAL(bytesTransmittedChanged(quint32)),this,SLOT(bytesTransmittedChanged(quint32)));
    connect(counter,SIGNAL(secondsOnlineChanged(quint32)),this,SLOT(secondsOnlineChanged(quint32)));
    connect(counter,SIGNAL(roamingChanged(bool)),SLOT(roamingChanged(bool)));

    counter->setRunning(true);
}

void NetworkCounter::counterChanged(const QString servicePath, const QVariantMap &counters,  bool roaming)
{
    QTextStream out(stdout);
    out  << endl << Q_FUNC_INFO << " " << servicePath << " " << (roaming ? "roaming" : "home") << endl;
}

void NetworkCounter::bytesReceivedChanged(quint32 bytesRx)
{
    QTextStream out(stdout);
    out  << Q_FUNC_INFO << " " << bytesRx << endl;
}

void NetworkCounter::bytesTransmittedChanged(quint32 bytesTx)
{
    QTextStream out(stdout);
    out  << Q_FUNC_INFO << " " << bytesTx << endl;
}

void NetworkCounter::secondsOnlineChanged(quint32 seconds)
{
    QTextStream out(stdout);
    out << Q_FUNC_INFO << " " << seconds << endl;
}

void NetworkCounter::roamingChanged(bool roaming)
{
    QTextStream out(stdout);
    out  << Q_FUNC_INFO << " " << roaming << endl;
}
