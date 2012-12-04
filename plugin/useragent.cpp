/*
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "useragent.h"

UserAgent::UserAgent(TechnologyModel* parent)
  : QDBusAbstractAdaptor(parent),
    m_model(parent)
{
    // TODO
}

UserAgent::~UserAgent() {}

void UserAgent::Release()
{
    // here do clean up
}

void UserAgent::ReportError(const QDBusObjectPath &service_path, const QString &error)
{
    qDebug() << "From " << service_path.path() << " got this error:\n" << error;
    m_model->reportError(error);
}

void UserAgent::RequestBrowser(const QDBusObjectPath &service_path, const QString &url)
{
    qDebug() << "Service " << service_path.path() << " wants browser to open hotspot's url " << url;
}

void UserAgent::RequestInput(const QDBusObjectPath &service_path,
                                       const QVariantMap &fields,
                                       const QDBusMessage &message)
{
    qDebug() << "Service " << service_path.path() << " wants user input";

    QVariantMap json;
    foreach (const QString &key, fields.keys()){
        QVariantMap payload = qdbus_cast<QVariantMap>(fields[key]);
        json.insert(key, payload);
    }

    message.setDelayedReply(true);

    ServiceRequestData *reqdata = new ServiceRequestData;
    reqdata->objectPath = service_path.path();
    reqdata->fields = json;
    reqdata->reply = message.createReply();
    reqdata->msg = message;

    m_model->requestUserInput(reqdata);
}

void UserAgent::Cancel()
{
    qDebug() << "WARNING: request to agent got canceled";
}
