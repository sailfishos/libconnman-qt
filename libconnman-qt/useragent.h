/*
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#ifndef USERAGENT_H
#define USERAGENT_H

#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusAbstractAdaptor>
#include <QTimer>
#include <QElapsedTimer>

#include "networkmanager.h"

struct ServiceRequestData
{
    QString objectPath;
    QVariantMap fields;
    QDBusMessage reply;
    QDBusMessage msg;
};

class UserAgentPrivate;

class UserAgent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString connectionRequestType READ connectionRequestType WRITE setConnectionRequestType)
    Q_PROPERTY(QString path READ path WRITE setAgentPath)
    Q_DISABLE_COPY(UserAgent)

public:
    explicit UserAgent(QObject* parent = 0);
    virtual ~UserAgent();

    QString connectionRequestType() const;
    QString path() const;

public Q_SLOTS:
    void sendUserReply(const QVariantMap &input);

    void sendConnectReply(const QString &replyMessage, int timeout = 120);
    void setConnectionRequestType(const QString &type);
    void setAgentPath(const QString &path);

Q_SIGNALS:
    void userInputRequested(const QString &servicePath, const QVariantMap &fields);
    void userInputCanceled();
    void errorReported(const QString &servicePath, const QString &error);
    void browserRequested(const QString &servicePath, const QString &url);

    void userConnectRequested(const QDBusMessage &message);
    void connectionRequest();

private Q_SLOTS:
    void updateMgrAvailability(bool);
    void requestTimeout();

private:
    void requestUserInput(ServiceRequestData* data);
    void cancelUserInput();
    void reportError(const QString &servicePath, const QString &error);
    void requestConnect(const QDBusMessage &msg);
    void requestBrowser(const QString &servicePath, const QString &url,
                        const QDBusMessage &message);

    UserAgentPrivate *d_ptr;

    friend class AgentAdaptor;
};

#endif // USERAGENT_H
