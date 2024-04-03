/*
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "useragent.h"
#include "networkmanager.h"

static const char AGENT_PATH[] = "/ConnectivityUserAgent";

class AgentAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "net.connman.Agent")

public:
    explicit AgentAdaptor(UserAgent* parent);
    virtual ~AgentAdaptor();

public Q_SLOTS:
    void Release();
    void ReportError(const QDBusObjectPath &service_path, const QString &error);
    Q_NOREPLY void RequestBrowser(const QDBusObjectPath &service_path, const QString &url,
                        const QDBusMessage &message);
    void RequestConnect(const QDBusMessage &message);

    Q_NOREPLY void RequestInput(const QDBusObjectPath &service_path,
                                const QVariantMap &fields,
                                const QDBusMessage &message);
    void Cancel();

private:
    UserAgent* m_userAgent;
    QElapsedTimer browserRequestTimer;
};

class UserAgentPrivate
{
public:
    enum ConnectionRequestType {
        TYPE_DEFAULT =0,
        TYPE_SUPPRESS,
        TYPE_CLEAR
    };

    UserAgentPrivate();

    ServiceRequestData *m_req_data;
    QSharedPointer<NetworkManager> m_manager;
    QDBusMessage currentDbusMessage;
    ConnectionRequestType requestType;
    QString agentPath;
    QTimer requestTimer;
    QDBusMessage requestMessage;
};

UserAgentPrivate::UserAgentPrivate()
    : m_req_data(nullptr)
    , m_manager(NetworkManager::sharedInstance())
    , requestType(TYPE_DEFAULT)
{
}

UserAgent::UserAgent(QObject* parent)
    : QObject(parent)
    , d_ptr(new UserAgentPrivate)
{
    QString agentpath = QLatin1String("/ConnectivityUserAgent");
    setAgentPath(agentpath);
    connect(d_ptr->m_manager.data(), &NetworkManager::availabilityChanged,
            this, &UserAgent::updateMgrAvailability);

    d_ptr->requestTimer.setSingleShot(true);
    connect(&d_ptr->requestTimer, &QTimer::timeout,
            this, &UserAgent::requestTimeout);
}

UserAgent::~UserAgent()
{
    d_ptr->m_manager->unregisterAgent(QString(d_ptr->agentPath));

    delete d_ptr;
    d_ptr = nullptr;
}

void UserAgent::requestUserInput(ServiceRequestData* data)
{
    d_ptr->m_req_data = data;
    Q_EMIT userInputRequested(data->objectPath, data->fields);
}

void UserAgent::cancelUserInput()
{
    delete d_ptr->m_req_data;
    d_ptr->m_req_data = nullptr;
    Q_EMIT userInputCanceled();
}

void UserAgent::reportError(const QString &servicePath, const QString &error)
{
    Q_EMIT errorReported(servicePath, error);
}

void UserAgent::sendUserReply(const QVariantMap &input)
{
    if (d_ptr->m_req_data == nullptr) {
        qWarning() << "Got reply for non-existing request";
        return;
    }

    if (!input.isEmpty()) {
        QDBusMessage &reply = d_ptr->m_req_data->reply;
        reply << input;
        QDBusConnection::systemBus().send(reply);
    } else {
        QDBusMessage error = d_ptr->m_req_data->msg.createErrorReply(
            QString("net.connman.Agent.Error.Canceled"),
            QString("canceled by user"));
        QDBusConnection::systemBus().send(error);
    }
    delete d_ptr->m_req_data;
    d_ptr->m_req_data = nullptr;
}

void UserAgent::requestTimeout()
{
    qDebug() << Q_FUNC_INFO << d_ptr->requestMessage.arguments();
    setConnectionRequestType("Clear");
}

void UserAgent::sendConnectReply(const QString &replyMessage, int timeout)
{
    setConnectionRequestType(replyMessage);

    if (!d_ptr->requestTimer.isActive())
        d_ptr->requestTimer.start(timeout * 1000);
}

void UserAgent::updateMgrAvailability(bool available)
{
    if (available) {
        d_ptr->m_manager->registerAgent(d_ptr->agentPath);
    } else {
        if (d_ptr->requestTimer.isActive())
            d_ptr->requestTimer.stop();
    }
}

void UserAgent::setConnectionRequestType(const QString &type)
{
    if (type == "Suppress") {
        d_ptr->requestType = UserAgentPrivate::TYPE_SUPPRESS;
    } else if (type == "Clear") {
        d_ptr->requestType = UserAgentPrivate::TYPE_CLEAR;
    } else {
        d_ptr->requestType = UserAgentPrivate::TYPE_DEFAULT;
    }
}

QString UserAgent::connectionRequestType() const
{
    switch (d_ptr->requestType) {
    case UserAgentPrivate::TYPE_SUPPRESS:
        return "Suppress";
    case UserAgentPrivate::TYPE_CLEAR:
        return "Clear";
    default:
        break;
    }
    return QString();
}

void UserAgent::requestConnect(const QDBusMessage &msg)
{
    QList<QVariant> arguments2;
    arguments2 << QVariant("Clear");
    d_ptr->requestMessage = msg.createReply(arguments2);

    QList<QVariant> arguments;
    arguments << QVariant(connectionRequestType());
    QDBusMessage error = msg.createReply(arguments);

    if (!QDBusConnection::systemBus().send(error)) {
        qWarning() << "Could not queue message";
    }

    if (connectionRequestType() == "Suppress") {
        return;
    }

    Q_EMIT connectionRequest();
    Q_EMIT userConnectRequested(msg);
    setConnectionRequestType("Suppress");
}

QString UserAgent::path() const
{
    return d_ptr->agentPath;
}

void UserAgent::setAgentPath(const QString &path)
{
    if (path.isEmpty())
        return;

    new AgentAdaptor(this); // this object will be freed when UserAgent is freed
    d_ptr->agentPath = path;
    QDBusConnection::systemBus().registerObject(d_ptr->agentPath, this);

    if (d_ptr->m_manager->isAvailable()) {
        d_ptr->m_manager->registerAgent(d_ptr->agentPath);
    }
}

void UserAgent::requestBrowser(const QString &servicePath, const QString &url,
                               const QDBusMessage &message)
{
    qDebug() << message.arguments();
    Q_EMIT browserRequested(servicePath, url);
}

////////////////////

AgentAdaptor::AgentAdaptor(UserAgent* parent)
  : QDBusAbstractAdaptor(parent),
    m_userAgent(parent)
{
    browserRequestTimer.invalidate();
}

AgentAdaptor::~AgentAdaptor()
{
}

void AgentAdaptor::Release()
{
}

void AgentAdaptor::ReportError(const QDBusObjectPath &service_path, const QString &error)
{
    m_userAgent->reportError(service_path.path(), error);
}

void AgentAdaptor::RequestBrowser(const QDBusObjectPath &service_path, const QString &url,
                                  const QDBusMessage &message)
{
    message.setDelayedReply(true);
    m_userAgent->requestBrowser(service_path.path(), url, message);
}

void AgentAdaptor::RequestInput(const QDBusObjectPath &service_path,
                                       const QVariantMap &fields,
                                       const QDBusMessage &message)
{
    QVariantMap json;
    for (const QString &key : fields.keys()){
        QVariantMap payload = qdbus_cast<QVariantMap>(fields[key]);
        json.insert(key, payload);
    }

    message.setDelayedReply(true);

    ServiceRequestData *reqdata = new ServiceRequestData;
    reqdata->objectPath = service_path.path();
    reqdata->fields = json;
    reqdata->reply = message.createReply();
    reqdata->msg = message;

    m_userAgent->requestUserInput(reqdata);
}

void AgentAdaptor::Cancel()
{
    m_userAgent->cancelUserInput();
}

void AgentAdaptor::RequestConnect(const QDBusMessage &message)
{
    message.setDelayedReply(true);
    m_userAgent->requestConnect(message);
}

#include "useragent.moc"
