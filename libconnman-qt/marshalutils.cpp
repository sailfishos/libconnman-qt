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
#include <QDBusArgument> 
#include "vpnconnection.h"

#include "marshalutils.h"

#if defined(__clang__) &&  __clang_major__ >= 11 || __GNUC__ >= 12
Q_DBUS_EXPORT const QDBusArgument &operator>>(const QDBusArgument &a, RouteStructure &v);
Q_DBUS_EXPORT QDBusArgument &operator<<(QDBusArgument &a, const RouteStructure &v);
#endif

// Empty namespace for local static functions
namespace {

// Marshall the RouteStructure data into a D-Bus argument
QDBusArgument &operator<<(QDBusArgument &argument, const RouteStructure &routestruct)
{
    QVariantMap dict;
    dict.insert("ProtocolFamily", routestruct.protocolFamily);
    dict.insert("Network", routestruct.network);
    dict.insert("Netmask", routestruct.netmask);
    dict.insert("Gateway", routestruct.gateway);

    argument.beginStructure();
    argument << dict;
    argument.endStructure();
    return argument;
}

// Retrieve the RouteStructure data from the D-Bus argument
const QDBusArgument &operator>>(const QDBusArgument &argument, RouteStructure &routestruct)
{
    QVariantMap dict;
    argument.beginStructure();
    argument >> dict;
    argument.endStructure();

    routestruct.protocolFamily = dict.value("ProtocolFamily", 0).toInt();
    routestruct.network = dict.value("Network").toString();
    routestruct.netmask = dict.value("Netmask").toString();
    routestruct.gateway = dict.value("Gateway").toString();

    return argument;
}

QVariant convertState (const QString &key, const QVariant &value, bool toDBus)
{
    QList<QPair<QVariant, QVariant> > states;
    states.push_back(qMakePair(QVariant::fromValue(QStringLiteral("idle")), QVariant::fromValue(static_cast<int>(VpnConnection::Idle))));
    states.push_back(qMakePair(QVariant::fromValue(QStringLiteral("failure")), QVariant::fromValue(static_cast<int>(VpnConnection::Failure))));
    states.push_back(qMakePair(QVariant::fromValue(QStringLiteral("configuration")), QVariant::fromValue(static_cast<int>(VpnConnection::Configuration))));
    states.push_back(qMakePair(QVariant::fromValue(QStringLiteral("ready")), QVariant::fromValue(static_cast<int>(VpnConnection::Ready))));
    states.push_back(qMakePair(QVariant::fromValue(QStringLiteral("disconnect")), QVariant::fromValue(static_cast<int>(VpnConnection::Disconnect))));

    auto lit = std::find_if(states.cbegin(), states.cend(), [value, toDBus](const QPair<QVariant, QVariant> &pair) { return value == (toDBus ? pair.second : pair.first); });
    if (lit != states.end()) {
        return toDBus ? (*lit).first : (*lit).second;
    }
    qDebug() << "No conversion found for" << (toDBus ? "QML" : "DBus") << "value:" << value << key;
    return value;
}

QVariant convertRoutes (const QString &, const QVariant &value, bool toDBus) {
    // We use qDBusRegisterMetaType in VpnConnections to convert automatically
    // between QList<RouteStruture> and QDBusArgument, but we still need to
    // convert to/from suitable Javascript structures
    QVariant variant;
    if (toDBus) {
        QVariantList in = value.toList();
        QList<RouteStructure> out;
        for (QVariant item : in) {
            QVariantMap jsRoute = item.toMap();
            RouteStructure route;
            route.protocolFamily = jsRoute.value("ProtocolFamily", 0).toInt();
            route.network = jsRoute.value("Network").toString();
            route.netmask = jsRoute.value("Netmask").toString();
            route.gateway = jsRoute.value("Gateway").toString();
            out << route;
        }
        variant.setValue(out);
    }
    else {
        QList<RouteStructure> in = qdbus_cast<QList<RouteStructure>>(value.value<QDBusArgument>());
        QVariantList out;
        for (RouteStructure route : in) {
            QVariantMap jsRoute;
            jsRoute.insert("ProtocolFamily", route.protocolFamily);
            jsRoute.insert("Network", route.network);
            jsRoute.insert("Netmask", route.netmask);
            jsRoute.insert("Gateway", route.gateway);
            out << jsRoute;
        }
        variant.setValue(out);
    }
    return variant;
}

} // Empty namespace

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
        }

        rv.insert(key, convertToQml(key, value));
    }

    if (!providerProperties.isEmpty()) {
        rv.insert(QStringLiteral("providerProperties"), QVariant::fromValue(providerProperties));
    }

    return rv;
}

#include <QDBusMetaType>
// Conversion to/from DBus/QML
QHash<QString, MarshalUtils::conversionFunction> MarshalUtils::propertyConversions()
{
    qDBusRegisterMetaType<RouteStructure>();
    qDBusRegisterMetaType<QList<RouteStructure>>();

    QHash<QString, conversionFunction> rv;

    rv.insert(QStringLiteral("state"), convertState);
    rv.insert(QStringLiteral("userroutes"), convertRoutes);
    rv.insert(QStringLiteral("serverroutes"), convertRoutes);

    return rv;
}

QVariant MarshalUtils::convertValue(const QString &key, const QVariant &value, bool toDBus)
{
    static const QHash<QString, conversionFunction> conversions(propertyConversions());

    auto it = conversions.find(key.toLower());
    if (it != conversions.end()) {
        return it.value()(key, value, toDBus);
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

