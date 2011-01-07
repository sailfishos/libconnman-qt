/*   -*- Mode: C++ -*-
 * meegotouchcp-connman - connectivity plugin for duicontrolpanel
 * Copyright © 2010, Intel Corporation.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */
#include "networkitemmodel.h"
#include "debug.h"
#include "commondbustypes.h"

//FIXME: could reuse these static strings for getProperties and popertyChanged
const char* const NetworkItemModel::Name = "Name";
const char* const NetworkItemModel::Security = "Security";
const char* const NetworkItemModel::Strength = "Strength";
const char* const NetworkItemModel::State = "State";
const char* const NetworkItemModel::Type = "Type";
const char* const NetworkItemModel::PassphraseRequired = "PassphraseRequired";
const char* const NetworkItemModel::Passphrase = "Passphrase";
const char* const NetworkItemModel::IPv4 = "IPv4.Configuration";
const char* const NetworkItemModel::Nameservers = "Nameservers";
const char* const NetworkItemModel::DeviceAddress = "DeviceAddress";
const char* const NetworkItemModel::Mode = "Mode";

int NetworkItemModel::instances = 0;
int NetworkItemModel::idCounter = 0;

NetworkItemModel::NetworkItemModel(const QString &path, QObject *parent) :
  id(idCounter),
  m_service(NULL),
  m_getPropertiesWatcher(NULL),
  m_setPropertyWatcher(NULL),
  m_disconnectWatcher(NULL),
  m_connectWatcher(NULL),
  m_state(StateNone),
  m_strength(0),
  m_passphraseRequired(false)
{
  registerCommonDataTypes();

  setParent(parent);
  instances++;
  idCounter++;
  m_servicePath = path;
  if (!path.isEmpty()) {
    m_service = new Service("net.connman", path,
			    QDBusConnection::systemBus(),
			    this);
    if (!m_service->isValid()) {
	  qDebug("service %s is invalid!", STR(path));
      throw -1; //FIXME
    }
    QDBusPendingReply<QVariantMap> reply = m_service->GetProperties();
    m_getPropertiesWatcher = new QDBusPendingCallWatcher(reply, this);
    connect(m_getPropertiesWatcher,
	    SIGNAL(finished(QDBusPendingCallWatcher*)),
	    this,
	    SLOT(getPropertiesReply(QDBusPendingCallWatcher*)));

	connect(m_service,
		SIGNAL(PropertyChanged(const QString&,
				   const QDBusVariant &)),
		this,
		SLOT(propertyChanged(const QString&,
					 const QDBusVariant &)));

  }


  //this is an attempt to avoid having lots of networks show in the
  //list with no type.  The thought process is that the filter is
  //matching on empty strings.  It doesn't appear to work in the
  //nothing in the list except one thing problem.  But other problems
  //haven't been tried
  m_type = "????";
}

NetworkItemModel::~NetworkItemModel()
{
  instances--;
  qDebug("destructing instance %d.  There are %d models around", id, instances);
}


void NetworkItemModel::connectService()
{
#ifdef PRETEND
  qDebug("connectService pretending");
#else
  qDebug("connectService FOR REAL");
  m_service->Connect();
#endif
}

void NetworkItemModel::disconnectService()
{
#ifdef PRETEND
  qDebug("disconnectService pretending");
#else
  qDebug("disconnectService FOR REAL");
  m_service->Disconnect();
#endif
}

void NetworkItemModel::removeService()
{
#ifdef PRETEND
  qDebug("removeService pretending");
#else
  qDebug("removeService FOR REAL");
  m_service->Remove();
#endif
}

const QString& NetworkItemModel::name() const
{
  return m_name;
}

const QString& NetworkItemModel::security() const
{
  return m_security;
}

const NetworkItemModel::StateType& NetworkItemModel::state() const
{
  return m_state;
}

const int& NetworkItemModel::strength() const
{
  return m_strength;
}

const QString& NetworkItemModel::type() const
{
  return m_type;
}

const bool& NetworkItemModel::passphraseRequired() const
{
  return m_passphraseRequired;
}

const QString& NetworkItemModel::passphrase() const
{
  return m_passphrase;
}

const NetworkItemModel::IPv4Type& NetworkItemModel::ipv4() const
{
  return m_ipv4;
}

const QString& NetworkItemModel::mode() const
{
  return m_mode;
}

const QString NetworkItemModel::ipv4address() const
{
	return ipv4().Address;
}

const QString NetworkItemModel::ipv4netmask() const
{
	return ipv4().Netmask;
}

const QString NetworkItemModel::ipv4gateway() const
{
	return ipv4().Gateway;
}

const QString NetworkItemModel::ipv4method() const
{
	return ipv4().Method;
}

void NetworkItemModel::setPassphrase(const QString &passphrase)
{
  Q_ASSERT(m_service);
  QDBusPendingReply<void> reply =
    m_service->SetProperty("Passphrase", QDBusVariant(QVariant(passphrase)));
  reply.waitForFinished(); //FIXME: BAD
  if (reply.isError()) {
	qDebug("got error from setProperty");
    throw -1; //FIXME: don't throw
  }

}

void NetworkItemModel::setIpv4(const IPv4Type &ipv4)
{
  Q_ASSERT(m_service);


  StringMap dict;
  Q_ASSERT(!ipv4.Method.isEmpty());
  dict.insert("Method", ipv4.Method);
  if (ipv4.Method != "dhcp") {
    //FIXME: what do to if address and such are empty!?!
    dict.insert("Address", ipv4.Address);
    dict.insert("Netmask", ipv4.Netmask);
    dict.insert("Gateway", ipv4.Gateway);
  }
  QVariant variant = qVariantFromValue(dict);
  QDBusPendingReply<void> reply =
    m_service->SetProperty(QString(IPv4), QDBusVariant(variant));
  if (m_setPropertyWatcher) {
    delete m_setPropertyWatcher;
  }
  m_setPropertyWatcher = new QDBusPendingCallWatcher(reply, this);
  connect(m_setPropertyWatcher,
	  SIGNAL(finished(QDBusPendingCallWatcher*)),
	  this,
	  SLOT(setPropertyFinished(QDBusPendingCallWatcher*)));
}


const QString& NetworkItemModel::servicePath() const
{
  return m_servicePath;
}

const QStringList& NetworkItemModel::nameservers() const
{
  return m_nameservers;
}

const QString& NetworkItemModel::deviceAddress() const
{
  return m_deviceAddress;
}

void NetworkItemModel::setNameservers(const QStringList &nameservers)
{
  Q_ASSERT(m_service);

  if (!isListEqual(m_nameservers, nameservers)) {
    QVariant variant = qVariantFromValue(nameservers);
    QDBusPendingReply<void> reply =
      m_service->SetProperty(Nameservers, QDBusVariant(variant));
      //I hate to wait here, but I'm not sure what else to do
      reply.waitForFinished();
      if (reply.isError()) {
	qDebug("got error from setProperty");
	throw -1; //FIXME: don't throw
      }
  }
}

void NetworkItemModel::setIpv4Address(QString v)
{
	m_ipv4.Address = v;
	setIpv4(m_ipv4);
}

void NetworkItemModel::setIpv4Netmask(QString v )
{
	m_ipv4.Netmask = v;
	setIpv4(m_ipv4);
}

void NetworkItemModel::setIpv4Gateway(QString v)
{
	m_ipv4.Gateway = v;
	setIpv4(m_ipv4);
}

void NetworkItemModel::setIpv4Method(QString v)
{
	m_ipv4.Method = v;
	setIpv4(m_ipv4);
}

NetworkItemModel::StateType NetworkItemModel::state(const QString &state)
{
  NetworkItemModel::StateType _state;
  if (state == "idle") {
	_state = StateIdle;
  } else if (state == "failure") {
	_state = StateFailure;
  } else if (state == "association") {
	_state = StateAssociation;
  } else if (state == "configuration") {
	_state = StateConfiguration;
  } else if (state == "ready") {
	_state = StateReady;
  } else if (state == "online") {
	_state = StateOnline;
  } else {
	_state = StateNone;
	qDebug("setting to to STATE_NONE because of unknown state returned: \"%s\"", STR(state));
  }

  return _state;
}

//This sets the m_ipv4 with data.  It does not set the data on the
//Service object
void NetworkItemModel::_setIpv4(const QVariantMap &ipv4)
{
  bool modified = false;
  QString string;

  string = ipv4["Method"].toString();
 // qDebug("Method: %s", STR(string));
  if (m_ipv4.Method != string) {
    m_ipv4.Method = string;
    modified = true;
  }
  string = ipv4["Address"].toString();
  //qDebug("Address: %s", STR(string));
  if (m_ipv4.Address != string) {
    m_ipv4.Address = string;
    modified = true;
  }
  string =  ipv4["Netmask"].toString();
  //qDebug("Netmask: %s", STR(string));
  if (m_ipv4.Netmask != string) {
    m_ipv4.Netmask = string;
    modified = true;
  }
  string = ipv4["Gateway"].toString();
 // qDebug("Gateway: %s", STR(string));
  if (m_ipv4.Gateway != string) {
    m_ipv4.Gateway = string;
    modified = true;
  }
}

bool NetworkItemModel::isListEqual(const QStringList &a, const QStringList &b) const
{
  if (a.count() != b.count()) {
    return false;
  }
  for (int i=0; i < a.count(); i++) {
    if (a.at(i) != b.at(i)) {
      return false;
    }
  }
  return true;
}



void NetworkItemModel::getPropertiesReply(QDBusPendingCallWatcher *call)
{
  QDBusPendingReply<QVariantMap> reply = *call;
  if (reply.isError()) {
	qDebug("getPropertiesReply is error!");
    QDBusError error = reply.error();
	qDebug("service: %s", STR(servicePath()));
	qDebug(QString("type: %1 name: %2 message: %3").
		 arg(QDBusError::errorString(error.type()))
		 .arg(error.name())
		 .arg(error.message()).toAscii());
	return;
  }
  qDebug()<<"getPropertiesReply";
  QVariantMap properties = reply.value();
  //although it seems dangerous as some of these properties are not
  //present, grabbing them is not, because QMap will insert the
  //default value into the map if it isn't present.  That's "" for
  //strings and 0 for ints/bools

  m_name = qdbus_cast<QString>(properties[Name]);
  m_type = qdbus_cast<QString>(properties[Type]);
  m_mode= qdbus_cast<QString>(properties[Mode]);
  m_security = qdbus_cast<QString>(properties[Security]);
  m_passphraseRequired = qdbus_cast<bool>(properties[PassphraseRequired]);
  m_passphrase = qdbus_cast<QString>(properties[Passphrase]);
  m_strength = qdbus_cast<int>(properties[Strength]);
  m_state = state(qdbus_cast<QString>(properties[State]));
  _setIpv4(qdbus_cast<QVariantMap>(properties["IPv4"]));
  m_nameservers = qdbus_cast<QStringList>(properties[Nameservers]);
  m_deviceAddress = qdbus_cast<QVariantMap>(properties["Ethernet"])["Address"].toString();
  emit propertyChanged();
}

void NetworkItemModel::propertyChanged(const QString &name,
				       const QDBusVariant &value)
{
  qDebug()<<"NetworkItemModel: property "<<STR(name)<<" changed: "<<value.variant();
    if (name == State) {
	  m_state = state(value.variant().toString());
	  stateChanged(m_state);
    } else if (name == Name) {
		m_name = (value.variant().toString());
		nameChanged(m_name);
    } else if (name == Type) {
	  m_type = (value.variant().toString());
    } else if (name == Mode) {
	  m_mode = (value.variant().toString());
    } else if (name == Security) {
		m_security = (value.variant().toString());
		securityChanged(m_security);
    } else if (name == PassphraseRequired) {
	  m_passphraseRequired = (value.variant().toBool());
    } else if (name == Passphrase) {
	  m_passphrase = (value.variant().toString());
    } else if (name == Strength) {
	  m_strength = (value.variant().toInt());
    } else if (name == IPv4) {
	  _setIpv4(qdbus_cast<QVariantMap>(value.variant()));
    } else if (name == Nameservers) {
	  m_nameservers = (value.variant().toStringList());
    } else if (name == "Ethernet") {
	  m_deviceAddress = (qdbus_cast<QVariantMap>(value.variant())["Address"].toString());
    } else {
	  qDebug("We don't do anything with property: %s", STR(name));
    }

	emit propertyChanged();
}


void NetworkItemModel::timerEvent(QTimerEvent *event)
{
  Q_UNUSED(event);
  //qDebug("hello!");
  //setStrength(rand()*100.0/RAND_MAX);
}

void NetworkItemModel::dump() const
{
  qDebug("%s", STR(dumpToString()));
}

QString NetworkItemModel::dumpToString() const
{
  return QString("id: %1 name: %2 state: %3 type: %4: path: %5").arg(id).arg(m_name).arg(m_state).arg(type()).arg(m_servicePath);
}

void NetworkItemModel::setPropertyFinished(QDBusPendingCallWatcher *call)
{
  QDBusPendingReply<void> reply = *call;
  if (reply.isError()) {
	qDebug()<<"not continuing because of error in setProperty!"<<reply.error().message();

  } else {
    QDBusPendingReply<void> nextReply = m_service->Disconnect();
    if (m_setPropertyWatcher) {
      delete m_setPropertyWatcher;
    }
    m_setPropertyWatcher = new QDBusPendingCallWatcher(nextReply, this);
    connect(m_setPropertyWatcher,
	    SIGNAL(finished(QDBusPendingCallWatcher*)),
	    this,
	    SLOT(disconnectFinished(QDBusPendingCallWatcher*)));
  }
}

void NetworkItemModel::disconnectFinished(QDBusPendingCallWatcher *call)
{
  QDBusPendingReply<void> reply = *call;
  if (reply.isError()) {
	qDebug("not continuing because of error in disconnect!");
  } else {
    QDBusPendingReply<void> nextReply = m_service->Connect();
    if (m_setPropertyWatcher) {
      delete m_setPropertyWatcher;
    }
    m_disconnectWatcher = new QDBusPendingCallWatcher(nextReply, this);
    connect(m_disconnectWatcher,
	    SIGNAL(finished(QDBusPendingCallWatcher*)),
	    this,
	    SLOT(connectFinished(QDBusPendingCallWatcher*)));
  }
}

void NetworkItemModel::connectFinished(QDBusPendingCallWatcher *call)
{
  QDBusPendingReply<void> reply = *call;
  if (reply.isError()) {
	qDebug("error calling connect!");
  } else {
	qDebug("connect finished without error");
  }
}

