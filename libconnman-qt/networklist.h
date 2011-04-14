/*   -*- Mode: C++ -*-
 * meegotouchcp-connman - connectivity plugin for duicontrolpanel
 * Copyright © 2010, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
#ifndef NETWORKLISTMODEL_H
#define NETWORKLISTMODEL_H

#include "networkitem.h"
#include "manager.h"

#include <QAbstractTableModel>
#include <QtDBus>

class QDBusServiceWatcher;

class NetworkListModel : public QAbstractListModel
{
  Q_OBJECT;

  Q_PROPERTY(bool offlineMode READ offlineMode WRITE setOfflineMode NOTIFY offlineModeChanged);
  Q_PROPERTY(QString defaultTechnology READ defaultTechnology NOTIFY defaultTechnologyChanged);
  Q_PROPERTY(QString state READ state NOTIFY stateChanged);
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged);
  Q_PROPERTY(QStringList availableTechnologies READ availableTechnologies NOTIFY availableTechnologiesChanged);
  Q_PROPERTY(QStringList enabledTechnologies READ enabledTechnologies NOTIFY enabledTechnologiesChanged);
  Q_PROPERTY(QStringList connectedTechnologies READ connectedTechnologies NOTIFY connectedTechnologiesChanged);
  Q_PROPERTY(NetworkItemModel* defaultRoute READ defaultRoute WRITE setDefaultRoute NOTIFY defaultRouteChanged);

public:  
  NetworkListModel(QObject* parent=0);
  virtual ~NetworkListModel();
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  QVariant data(const QModelIndex &index, int role) const;

  const QStringList availableTechnologies() const;
  const QStringList enabledTechnologies() const;
  const QStringList connectedTechnologies() const;

  NetworkItemModel* defaultRoute() { return m_defaultRoute; }

  void setDefaultRoute(NetworkItemModel* item);



public slots:
  NetworkItemModel* service(QString name);

  void connectService(const QString &name, const QString &security,
			  const QString &passphrase);
  void connectService(const QString &name);
  void setProperty(const int &index, QString property, const QVariant &value);
  void requestScan();
  bool offlineMode() const;
  void setOfflineMode(const bool &offlineMode);
  void enableTechnology(const QString &technology);
  void disableTechnology(const QString &technology);
  QString defaultTechnology() const;
  QString state() const;

  QList<NetworkItemModel*> networks() { return m_networks.toList(); }

signals:
  void technologiesChanged(const QStringList &availableTechnologies,
			   const QStringList &enabledTechnologies,
			   const QStringList &connectedTechnologies);
  void availableTechnologiesChanged(const QStringList);
  void enabledTechnologiesChanged(const QStringList);
  void connectedTechnologiesChanged(const QStringList);

  void offlineModeChanged(bool offlineMode);
  void defaultTechnologyChanged(const QString &defaultTechnology);
  void stateChanged(const QString &state);
  void countChanged(int newCount);
  void defaultRouteChanged(NetworkItemModel* item);
  void connectedNetworkItemsChanged();

protected:
  void timerEvent(QTimerEvent *event); //hack


private:
  int findNetworkItemModel(const QDBusObjectPath &path) const;
  void emitTechnologiesChanged();

  Manager *m_manager;
  QDBusPendingCallWatcher *m_getPropertiesWatcher;
  QDBusPendingCallWatcher *m_connectServiceWatcher;
  QVariantMap m_propertiesCache;
  QVector<NetworkItemModel*> m_networks;
  QStringList m_headerData;
  static const QString availTechs;
  static const QString enablTechs;
  static const QString connTechs;
  static const QString OfflineMode;
  static const QString DefaultTechnology;
  static const QString State;
  NetworkItemModel* m_defaultRoute;
  QDBusServiceWatcher *watcher;

private slots:
  void connectToConnman(QString = "");
  void disconnectFromConnman(QString = "");
  void getPropertiesReply(QDBusPendingCallWatcher *call);
  void connectServiceReply(QDBusPendingCallWatcher *call);
  void propertyChanged(const QString &name,
		       const QDBusVariant &value);
  void networkItemModified(const QList<const char *> &members);
  void itemPropertyChanged();
  void countChangedSlot(int newCount);
  void itemStateChanged(NetworkItemModel::StateType);
private:
  Q_DISABLE_COPY(NetworkListModel);
};

#endif //NETWORKLISTMODEL_H
