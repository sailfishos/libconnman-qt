/*
 * Copyright (c) 2016 - 2019 Jolla Ltd.
 * Copyright (c) 2019 Open Mobile Platform LLC.
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * "Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Nemo Mobile nor the names of its contributors
 *     may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 */

#include <QDebug>
#include "vpnconnection.h"

#include "marshalutils.h"

template<typename T>
inline QVariant extract(const QDBusArgument &arg)
{
    T rv;
    arg >> rv;
    return QVariant::fromValue(rv);
}

template<typename T>
inline QVariant extractArray(const QDBusArgument &arg)
{
    QVariantList rv;

    arg.beginArray();
    while (!arg.atEnd()) {
        rv.append(extract<T>(arg));
    }
    arg.endArray();

    return QVariant::fromValue(rv);
}

QVariantMap MarshalUtils::propertiesToQml(const QVariantMap &fromDBus)
{
    QVariantMap rv;

    QVariantMap providerProperties;

    for (QVariantMap::const_iterator it = fromDBus.cbegin(), end = fromDBus.cend(); it != end; ++it) {
        QString key(it.key());
        QVariant value(it.value());

        if (key.indexOf(QChar('.')) != -1) {
            providerProperties.insert(key, value);
            continue;
        }

        // QML properties must be lowercased
        QChar &initial(*key.begin());
        initial = initial.toLower();

        // Some properties must be extracted manually
        if (key == QStringLiteral("iPv4") ||
            key == QStringLiteral("iPv6")) {
            // iPv4 becomes ipv4 and iPv6 becomes ipv6
            key = key.toLower();
            value = extract<QVariantMap>(value.value<QDBusArgument>());
        } else if (key == QStringLiteral("serverRoutes") ||
                   key == QStringLiteral("userRoutes")) {
            value = extractArray<QVariantMap>(value.value<QDBusArgument>());
        }

        rv.insert(key, convertToQml(key, value));
    }

    if (!providerProperties.isEmpty()) {
        rv.insert(QStringLiteral("providerProperties"), QVariant::fromValue(providerProperties));
    }

    return rv;
}

// Conversion to/from DBus/QML
QHash<QString, QList<QPair<QVariant, QVariant> > > MarshalUtils::propertyConversions()
{
    QHash<QString, QList<QPair<QVariant, QVariant> > > rv;

    QList<QPair<QVariant, QVariant> > states;
    states.push_back(qMakePair(QVariant::fromValue(QString("idle")), QVariant::fromValue(static_cast<int>(VpnConnection::Idle))));
    states.push_back(qMakePair(QVariant::fromValue(QString("failure")), QVariant::fromValue(static_cast<int>(VpnConnection::Failure))));
    states.push_back(qMakePair(QVariant::fromValue(QString("configuration")), QVariant::fromValue(static_cast<int>(VpnConnection::Configuration))));
    states.push_back(qMakePair(QVariant::fromValue(QString("ready")), QVariant::fromValue(static_cast<int>(VpnConnection::Ready))));
    states.push_back(qMakePair(QVariant::fromValue(QString("disconnect")), QVariant::fromValue(static_cast<int>(VpnConnection::Disconnect))));
    rv.insert(QString("state"), states);

    return rv;
}

QVariant MarshalUtils::convertValue(const QString &key, const QVariant &value, bool toDBus)
{
    static const QHash<QString, QList<QPair<QVariant, QVariant> > > conversions(propertyConversions());

    auto it = conversions.find(key.toLower());
    if (it != conversions.end()) {
        const QList<QPair<QVariant, QVariant> > &list(it.value());
        auto lit = std::find_if(list.cbegin(), list.cend(), [value, toDBus](const QPair<QVariant, QVariant> &pair) { return value == (toDBus ? pair.second : pair.first); });
        if (lit != list.end()) {
            return toDBus ? (*lit).first : (*lit).second;
        } else {
            qDebug() << "No conversion found for" << (toDBus ? "QML" : "DBus") << "value:" << value << key;
        }
    }

    return value;
}

QVariant MarshalUtils::convertToQml(const QString &key, const QVariant &value)
{
    return convertValue(key, value, false);
}

QVariant MarshalUtils::convertToDBus(const QString &key, const QVariant &value)
{
    return convertValue(key, value, true);
}

QVariantMap MarshalUtils::propertiesToDBus(const QVariantMap &fromQml)
{
    QVariantMap rv;

    for (QVariantMap::const_iterator it = fromQml.cbegin(), end = fromQml.cend(); it != end; ++it) {
        QString key(it.key());
        QVariant value(it.value());

        if (key == QStringLiteral("providerProperties")) {
            const QVariantMap providerProperties(value.value<QVariantMap>());
            for (QVariantMap::const_iterator pit = providerProperties.cbegin(), pend = providerProperties.cend(); pit != pend; ++pit) {
                rv.insert(pit.key(), pit.value());
            }
            continue;
        }

        // The DBus properties are capitalized
        QChar &initial(*key.begin());
        initial = initial.toUpper();

        if (key == QStringLiteral("Ipv4") ||
            key == QStringLiteral("Ipv6")) {
            // Ipv4 becomes IPv4 and Ipv6 becomes IPv6
            key[1] = 'P';
        }

        rv.insert(key, convertToDBus(key, value));
    }

    return rv;
}

