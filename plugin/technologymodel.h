/*
 * Copyright © 2010, Intel Corporation.
 * Copyright © 2012, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * WARNING: this class is experimetal and is about to be refactored in order
 *          to deprecate NetworkingModel.
 */

#ifndef TECHNOLOGYMODEL_H
#define TECHNOLOGYMODEL_H

#include <QAbstractListModel>
#include <networkmanager.h>
#include <networktechnology.h>
#include <networkservice.h>

struct ServiceRequestData
{
    QString objectPath;
    QVariantMap fields;
    QDBusMessage reply;
    QDBusMessage msg;
};

/*
 * TechnologyModel is a list model specific to a certain technology (wifi by default).
 * TODO: 1. consider refactoring this class to NetworkServiceModel with
 *          "technologyName" as a filtering property;
 *       2. drop properties NetworkServiceModel.available, NetworkServiceModel.powered;
 *       2. expose NetworkTechnology as QtQuick component.
 */
class TechnologyModel : public QAbstractListModel
{
    Q_OBJECT;
    Q_DISABLE_COPY(TechnologyModel);

    // TODO: consider "name" as writeable property of TechnologyModel QtQuck component
    Q_PROPERTY(QString name READ name NOTIFY nameChanged);
    Q_PROPERTY(bool available READ isAvailable NOTIFY availabilityChanged);
    Q_PROPERTY(bool powered READ isPowered WRITE setPowered NOTIFY poweredChanged);

public:
    enum ItemRoles {
        ServiceRole = Qt::UserRole + 1
    };

    TechnologyModel(QAbstractListModel* parent = 0);
    virtual ~TechnologyModel();

    QVariant data(const QModelIndex &index, int role) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    const QString name() const;
    bool isAvailable() const;
    bool isPowered() const;

    void requestUserInput(ServiceRequestData* data);
    void reportError(const QString &error);

    Q_INVOKABLE void sendUserReply(const QVariantMap &input);
    Q_INVOKABLE int indexOf(const QString &dbusObjectPath) const;

public slots:
    void setPowered(const bool &powered);
    void requestScan() const;

signals:
    void nameChanged(const QString &name);
    void availabilityChanged(const bool &available);
    void poweredChanged(const bool &powered);
    void technologiesChanged();

    void userInputRequested(const QString &servicePath, const QVariantMap &fields);
    void errorReported(const QString &error);
    void scanRequestFinished();

private:
    QString m_techname;
    NetworkManager* m_manager;
    NetworkTechnology* m_tech;
    ServiceRequestData* m_req_data;
    QVector<NetworkService *> m_services;

private slots:
    void updateTechnologies();
    void updateServiceList();
    void managerAvailabilityChanged(bool available);
};

#endif // TECHNOLOGYMODEL_H
