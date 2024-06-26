/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2017 Jolla Ltd.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0. The full text of the Apache License
 * is at http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef NETWORKTECHNOLOGY_H
#define NETWORKTECHNOLOGY_H

#include <QtDBus>
#include <QSharedPointer>

class NetworkTechnologyPrivate;

class NetworkTechnology : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString type READ type NOTIFY typeChanged)
    Q_PROPERTY(bool powered READ powered WRITE setPowered NOTIFY poweredChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(quint32 idleTimeout READ idleTimeout WRITE setIdleTimeout NOTIFY idleTimeoutChanged)

    Q_PROPERTY(bool tethering READ tethering WRITE setTethering NOTIFY tetheringChanged)
    Q_PROPERTY(QString tetheringId READ tetheringId WRITE setTetheringId NOTIFY tetheringIdChanged)
    Q_PROPERTY(QString tetheringPassphrase READ tetheringPassphrase WRITE setTetheringPassphrase NOTIFY tetheringPassphraseChanged)

public:
    NetworkTechnology(const QString &path, const QVariantMap &properties, QObject* parent);
    NetworkTechnology(QObject *parent = nullptr);

    virtual ~NetworkTechnology();

    bool available() const;
    QString name() const;
    QString type() const;
    bool powered() const;
    bool connected() const;
    QString objPath() const;

    QString path() const;

    quint32 idleTimeout() const;
    void setIdleTimeout(quint32);

    bool tethering() const;
    void setTethering(bool);

    QString tetheringId() const;
    void setTetheringId(const QString &id);

    QString tetheringPassphrase() const;
    void setTetheringPassphrase(const QString &pass);

public Q_SLOTS:
    void setPowered(bool powered);
    void scan();
    void setPath(const QString &path);

Q_SIGNALS:
    void availableChanged();
    void poweredChanged(const bool &powered);
    void connectedChanged(const bool &connected);
    void scanFinished();
    void idleTimeoutChanged(quint32 timeout);
    void tetheringChanged(bool tetheringEnabled);
    void tetheringIdChanged(const QString &tetheringId);
    void tetheringPassphraseChanged(const QString &passphrase);
    void pathChanged(const QString &path);
    void propertiesReady();
    void nameChanged(const QString &name);
    void typeChanged(const QString &type);

private:
    NetworkTechnologyPrivate *d_ptr;

private Q_SLOTS:
    void onInterfaceChanged(const QString &interface);
    void propertyChanged(const QString &name, const QDBusVariant &value);
    void emitPropertyChange(const QString &name, const QVariant &value);

    void scanReply(QDBusPendingCallWatcher *call);
    void getPropertiesFinished(QDBusPendingCallWatcher *call);

    void pendingSetProperty(const QString &key, const QVariant &value);

    void initialize();
    void createInterface();
    void destroyInterface();

private:
    Q_DISABLE_COPY(NetworkTechnology)
};

#endif //NETWORKTECHNOLOGY_H
