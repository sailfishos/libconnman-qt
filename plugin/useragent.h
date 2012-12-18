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

#include <networkmanager.h>

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
    Q_DISABLE_COPY(UserAgent)

public:
    explicit UserAgent(QObject* parent = 0);
    virtual ~UserAgent();

    Q_INVOKABLE void sendUserReply(const QVariantMap &input);

signals:
    void userInputRequested(const QString &servicePath, const QVariantList &fields);
    void userInputCanceled();
    void errorReported(const QString &error);

private slots:
    void updateMgrAvailability(bool &available);

private:
    void requestUserInput(ServiceRequestData* data);
    void cancelUserInput();
    void reportError(const QString &error);

    ServiceRequestData* m_req_data;
    NetworkManager* m_manager;

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
    Q_NOREPLY void RequestInput(const QDBusObjectPath &service_path,
                                const QVariantMap &fields,
                                const QDBusMessage &message);
    void Cancel();

private:
    UserAgent* m_userAgent;
};

#endif // USERAGENT_H
