/*
 * Copyright © 2010 Intel Corporation.
 * Copyright © 2012-2017 Jolla Ltd.
 * Contact: Slava Monich <slava.monich@jolla.com>
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0. The full text of the Apache License
 * is at http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef TECHNOLOGYMODEL_H
#define TECHNOLOGYMODEL_H

#include <QAbstractListModel>
#include <networkmanager.h>
#include <networktechnology.h>
#include <networkservice.h>

/*
 * TechnologyServiceModel is a list model specific to a certain technology (wifi by default).
 */
class TechnologyServiceModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool available READ isAvailable NOTIFY availabilityChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(bool powered READ isPowered WRITE setPowered NOTIFY poweredChanged)
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(bool changesInhibited READ changesInhibited WRITE setChangesInhibited NOTIFY changesInhibitedChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(ServiceFilter filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_ENUMS(ServiceFilter)

public:
    enum ServiceFilter {
        AllServices,
        SavedServices,
        AvailableServices
    };

    enum ItemRoles {
        ServiceRole = Qt::UserRole + 1
    };

    TechnologyServiceModel(QObject *parent = 0);
    virtual ~TechnologyServiceModel();

    QVariant data(const QModelIndex &index, int role) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    int count() const;
    QString name() const;
    bool isAvailable() const;
    bool isConnected() const;
    void setPowered(bool powered);
    bool isPowered() const;
    bool isScanning() const;
    bool changesInhibited() const;
    ServiceFilter filter() const;

    void setName(const QString &name);
    void setChangesInhibited(bool b);
    void setFilter(ServiceFilter filter);

    Q_INVOKABLE int indexOf(const QString &dbusObjectPath) const;
    Q_INVOKABLE NetworkService *get(int index) const;
    Q_INVOKABLE void requestScan();

Q_SIGNALS:
    void nameChanged(const QString &name);
    void availabilityChanged(const bool &available);
    void connectedChanged(const bool &connected);
    void poweredChanged(const bool &powered);
    void scanningChanged(const bool &scanning);
    void changesInhibitedChanged(const bool &changesInhibited);
    void technologiesChanged();
    void countChanged();
    void filterChanged();
    void scanRequestFinished();

private:
    Q_DISABLE_COPY(TechnologyServiceModel)

    QString m_techname;
    QSharedPointer<NetworkManager> m_manager;
    NetworkTechnology* m_tech;
    QVector<NetworkService *> m_services;
    bool m_scanning;
    bool m_changesInhibited;
    bool m_uneffectedChanges;
    ServiceFilter m_filter;

    QHash<int, QByteArray> roleNames() const;
    void doUpdateTechnologies();

private Q_SLOTS:
    void updateTechnologies();
    void updateServiceList();
    void managerAvailabilityChanged(bool available);
    void changedPower(bool);
    void changedConnected(bool);
    void finishedScan();
    void networkServiceDestroyed(QObject *);
};

class TechnologyModel: public TechnologyServiceModel
{
public:
    TechnologyModel(QObject *parent = 0);
};

#endif // TECHNOLOGYMODEL_H
