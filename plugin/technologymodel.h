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

/*
 * TechnologyModel is a list model specific to a certain technology (wifi by default).
 */
class TechnologyModel : public QAbstractListModel
{
    Q_OBJECT
    Q_DISABLE_COPY(TechnologyModel)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availabilityChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(bool powered READ isPowered WRITE setPowered NOTIFY poweredChanged)

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
    bool isConnected() const;
    bool isPowered() const;

    void setName(const QString &name);

    Q_INVOKABLE int indexOf(const QString &dbusObjectPath) const;

    Q_INVOKABLE NetworkService *get(int index) const;

public slots:
    void setPowered(const bool &powered);
    void requestScan() const;

signals:
    void nameChanged(const QString &name);
    void availabilityChanged(const bool &available);
    void connectedChanged(const bool &connected);
    void poweredChanged(const bool &powered);
    void technologiesChanged();

    void scanRequestFinished();

private:
    QString m_techname;
    NetworkManager* m_manager;
    NetworkTechnology* m_tech;
    QVector<NetworkService *> m_services;
    QHash<int, QByteArray> roleNames() const;

private slots:
    void updateTechnologies();
    void updateServiceList();
    void managerAvailabilityChanged(bool available);
    void changedPower(bool);
    void changedConnected(bool);
    void finishedScan();
};

#endif // TECHNOLOGYMODEL_H
