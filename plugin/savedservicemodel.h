/*
 * Copyright © 2010, Intel Corporation.
 * Copyright © 2013, Jolla.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * WARNING: this class is experimetal and is about to be refactored in order
 *          to deprecate NetworkingModel.
 */

#ifndef SAVEDSERVICEMODEL_H
#define SAVEDSERVICEMODEL_H

#include <QAbstractListModel>
#include <networkmanager.h>
#include <networkservice.h>

/*
 * SavedServiceModel is a list model containing saved wifi services.
 */
class SavedServiceModel : public QAbstractListModel
{
    Q_OBJECT
    Q_DISABLE_COPY(SavedServiceModel)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool sort READ sort WRITE setSort NOTIFY sortChanged)
    Q_PROPERTY(bool groupByCategory READ groupByCategory WRITE setGroupByCategory NOTIFY groupByCategoryChanged)

public:
    enum ItemRoles {
        ServiceRole = Qt::UserRole + 1,
        ManagedRole
    };

    SavedServiceModel(QAbstractListModel* parent = 0);
    virtual ~SavedServiceModel();

    QVariant data(const QModelIndex &index, int role) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QString name() const;
    void setName(const QString &name);

    bool sort() const;
    void setSort(bool sortList);

    bool groupByCategory() const;
    void setGroupByCategory(bool groupByCategory);

    Q_INVOKABLE int indexOf(const QString &dbusObjectPath) const;

    Q_INVOKABLE NetworkService *get(int index) const;

Q_SIGNALS:
    void nameChanged(const QString &name);
    void sortChanged();
    void groupByCategoryChanged();

private:
    QString m_techname;
    NetworkManager* m_manager;
    QVector<NetworkService *> m_services;
    bool m_sort;
    bool m_groupByCategory;

    QHash<int, QByteArray> roleNames() const;

private Q_SLOTS:
    void updateServiceList();
};

#endif // SAVEDSERVICEMODEL_H
