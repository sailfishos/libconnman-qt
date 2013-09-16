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

public:
    enum ItemRoles {
        ServiceRole = Qt::UserRole + 1
    };

    SavedServiceModel(QAbstractListModel* parent = 0);
    virtual ~SavedServiceModel();

    QVariant data(const QModelIndex &index, int role) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    const QString name() const;
    void setName(const QString &name);

    Q_INVOKABLE int indexOf(const QString &dbusObjectPath) const;

    Q_INVOKABLE NetworkService *get(int index) const;

signals:
    void nameChanged(const QString &name);

private:
    QString m_techname;
    NetworkManager* m_manager;
    QVector<NetworkService *> m_services;
    QHash<int, QByteArray> roleNames() const;

private slots:
    void updateServiceList();
};

#endif // SAVEDSERVICEMODEL_H
