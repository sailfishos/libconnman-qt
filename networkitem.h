/*   -*- Mode: C++ -*-
 * meegotouchcp-connman - connectivity plugin for duicontrolpanel
 * Copyright © 2010, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
#ifndef NETWORKITEMMODEL_H
#define NETWORKITEMMODEL_H

#include "service.h"
#include "commondbustypes.h"

#include <QString>
#include <QObject>

//#define PRETEND 1



class NetworkItemModel : public QObject
{
  Q_OBJECT

 public:
  enum StateType {
	StateNone = 0,
	StateIdle,
	StateFailure,
	StateAssociation,
	StateConfiguration,
	StateReady,
	StateOnline,
  };

  struct IPv4Type
  {
	  QString Method;
	  QString Address;
	  QString Netmask;
	  QString Gateway;
  };

 public:
  NetworkItemModel(const QString &path = QString(), QObject *parent = 0);
  virtual ~NetworkItemModel();

 public:
  /* property names */
  static const char* const Name;
  static const char* const Security;
  static const char* const State;
  static const char* const Strength;
  static const char* const Type;
  static const char* const PassphraseRequired;
  static const char* const Passphrase;
  static const char* const IPv4;
  static const char* const IPv4Normal;
  static const char* const Nameservers;
  static const char* const DeviceAddress;
  static const char* const Mode;
  static const char* const SetupRequired;
  static const char* const APN;
  static const char* const LoginRequired;

  /* property definitions */
  Q_PROPERTY(QString name READ name NOTIFY nameChanged);
  Q_PROPERTY(QString security READ security NOTIFY securityChanged);
  Q_PROPERTY(StateType state READ state NOTIFY stateChanged);
  Q_PROPERTY(int strength READ strength);
  Q_PROPERTY(QString type READ type);
  Q_PROPERTY(QString mode READ mode);
  Q_PROPERTY(bool passphraseRequired READ passphraseRequired);
  Q_PROPERTY(QString passphrase READ passphrase WRITE setPassphrase);
  Q_PROPERTY(IPv4Type ipv4 READ ipv4 WRITE setIpv4);
  Q_PROPERTY(QStringList nameservers READ nameservers WRITE setNameservers);
  Q_PROPERTY(QString deviceAddress READ deviceAddress);
  Q_PROPERTY(QString ipaddress READ ipv4address WRITE setIpv4Address);
  Q_PROPERTY(QString netmask READ ipv4netmask WRITE setIpv4Netmask);
  Q_PROPERTY(QString method READ ipv4method WRITE setIpv4Gateway);
  Q_PROPERTY(QString gateway READ ipv4gateway WRITE setIpv4Method);
  Q_PROPERTY(bool setupRequired READ setupRequired NOTIFY setupRequiredChanged)
  Q_PROPERTY(QString apn READ apn WRITE setApn)
  Q_ENUMS(StateType)

  /* property getters */
  const QString& name() const;
  const QString& security() const;
  const StateType& state() const;
  const int& strength() const;
  const QString& type() const;
  const bool& passphraseRequired() const;
  const QString &passphrase() const;
  // const QVariantMap &ipv4() const;
  const IPv4Type &ipv4() const;
  const QStringList &nameservers() const;
  const QString &deviceAddress() const;
  const QString &mode() const;
  const QString ipv4address() const;
  const QString ipv4netmask() const;
  const QString ipv4gateway() const;
  const QString ipv4method() const;
  const bool setupRequired() const;
  const QString apn() const;

  /* property setters */
  //These actually set the property on the underlying service object.
  void setPassphrase(const QString &passphrase);
  void setIpv4(const IPv4Type &ipv4);
  void setNameservers(const QStringList &nameservers);

  void setIpv4Address(QString);
  void setIpv4Netmask(QString);
  void setIpv4Gateway(QString);
  void setIpv4Method(QString);
  void setApn(QString);

  /* debug */
  int id;
  static int instances;
  static int idCounter;
  void dump() const;
  QString dumpToString() const;

  const QString& servicePath() const;

 public slots:
  void connectService();
  void disconnectService();
  void removeService();

signals:
  void propertyChanged();
  void nameChanged(QString newname);
  void securityChanged(QString security);
  void setupRequiredChanged(bool setupRequired);
  void stateChanged(StateType newstate);

 protected:
  void timerEvent(QTimerEvent *event); //hack

 private:
  bool isListEqual(const QStringList &a, const QStringList &b) const;
  StateType state(const QString &);
  void _setIpv4(const QVariantMap &ipv4);

  QString m_servicePath;
  Service *m_service;
  QDBusPendingCallWatcher *m_getPropertiesWatcher;
  QDBusPendingCallWatcher *m_setPropertyWatcher;
  QDBusPendingCallWatcher *m_disconnectWatcher;
  QDBusPendingCallWatcher *m_connectWatcher;

  QString m_name;
  QString m_security;
  StateType m_state;
  int m_strength;
  QString m_type; /* should be enum ?? */
  bool m_passphraseRequired;
  QString m_passphrase;
  IPv4Type m_ipv4;
  QStringList m_nameservers;
  QString m_deviceAddress;
  QString m_mode;
  bool m_setupRequired;
  QString m_apn;

private slots:
  void getPropertiesReply(QDBusPendingCallWatcher *call);
  void propertyChanged(const QString &name,
		       const QDBusVariant &value);

  //These are all due to MBC#1070
  void setPropertyFinished(QDBusPendingCallWatcher *call);
  void disconnectFinished(QDBusPendingCallWatcher *call);
  void connectFinished(QDBusPendingCallWatcher *call);
};

Q_DECLARE_METATYPE(NetworkItemModel::IPv4Type);

#endif //NETWORKITEMMODEL_H

