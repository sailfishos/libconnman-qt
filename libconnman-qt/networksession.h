/*
 * Copyright © 2012, Jolla Ltd
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */


#ifndef SESSIONSERVICE_H
#define SESSIONSERVICE_H

#include <QObject>
#include <QtDBus>

class SessionAgent;

class NetworkSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString state READ state)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString bearer READ bearer)
    Q_PROPERTY(QString sessionInterface READ sessionInterface)
    Q_PROPERTY(QVariantMap ipv4 READ ipv4)
    Q_PROPERTY(QVariantMap ipv6 READ ipv6)

    Q_PROPERTY(QString path READ path WRITE setPath)

    Q_PROPERTY(QStringList allowedBearers READ allowedBearers WRITE setAllowedBearers NOTIFY allowedBearersChanged)
    Q_PROPERTY(QString connectionType READ connectionType WRITE setConnectionType NOTIFY connectionTypeChanged)

public:
    NetworkSession(QObject *parent = 0);
    virtual ~NetworkSession();

    //Settings
     QString state() const;
     QString name() const;
     QString bearer() const;
     QString sessionInterface() const;
     QVariantMap ipv4() const;
     QVariantMap ipv6() const;
     QStringList allowedBearers() const;
     QString connectionType() const;

     QString path() const;

    void setAllowedBearers(const QStringList &bearers);
    void setConnectionType(const QString &type);

signals:

    void allowedBearersChanged(const QStringList &bearers);
    void connectionTypeChanged(const QString &type);
    void settingsChanged(const QVariantMap &settings);

public slots:
    void requestDestroy();
    void requestConnect();
    void requestDisconnect();
    void sessionSettingsUpdated(const QVariantMap &settings);
    void setPath(const QString &path);
    void registerSession();

private:
    SessionAgent *m_sessionAgent;
    QVariantMap settingsMap;
    QString m_path;
};

#endif // SESSIONSERVICE_H
