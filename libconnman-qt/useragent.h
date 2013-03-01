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

class NetworkManager;

struct ServiceRequestData
{
    QString objectPath;
    QVariantMap fields;
    QDBusMessage reply;
    QDBusMessage msg;
};

class UserAgent : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString connectionRequestType READ connectionRequestType WRITE setConnectionRequestType)
    Q_PROPERTY(void connectionRequest NOTIFY connectionRequest)
    Q_PROPERTY(QString path READ path WRITE setAgentPath)
    Q_DISABLE_COPY(UserAgent)

public:
    explicit UserAgent(QObject* parent = 0);
    virtual ~UserAgent();
enum ConnectionRequestType {
    TYPE_DEFAULT =0,
    TYPE_SUPPRESS,
    TYPE_CLEAR
};

public slots:
    void sendUserReply(const QVariantMap &input);

    void sendConnectReply(const QString &replyMessage, int timeout = 120);
    void setConnectionRequestType(const QString &type);
    QString connectionRequestType() const;

    QString path() const;
    void setAgentPath(QString &path);

signals:
    void userInputRequested(const QString &servicePath, const QVariantList &fields);
    void userInputCanceled();
    void errorReported(const QString &error);

    void userConnectRequested(const QDBusMessage &message);
    void connectionRequest();

private slots:
    void updateMgrAvailability(bool);
    void requestTimeout();

private:
    void requestUserInput(ServiceRequestData* data);
    void cancelUserInput();
    void reportError(const QString &error);
    void requestConnect(const QDBusMessage &msg);

    ServiceRequestData* m_req_data;
    NetworkManager* m_manager;
    QDBusMessage currentDbusMessage;
    ConnectionRequestType requestType;
    QString agentPath;

    friend class AgentAdaptor;
};

class AgentAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT;
    Q_CLASSINFO("D-Bus Interface", "net.connman.Agent");

public:
    explicit AgentAdaptor(UserAgent* parent);
    virtual ~AgentAdaptor();

public slots:
    void Release();
    void ReportError(const QDBusObjectPath &service_path, const QString &error);
    void RequestBrowser(const QDBusObjectPath &service_path, const QString &url);
    void RequestConnect(const QDBusMessage &message);

    Q_NOREPLY void RequestInput(const QDBusObjectPath &service_path,
                                const QVariantMap &fields,
                                const QDBusMessage &message);
    void Cancel();

private:
    UserAgent* m_userAgent;
};

#endif // USERAGENT_H
