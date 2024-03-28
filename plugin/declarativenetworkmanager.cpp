#include "declarativenetworkmanager.h"

#include <QDebug>

DeclarativeNetworkManagerFactory::DeclarativeNetworkManagerFactory(QObject *parent)
    : QObject(parent)
    , m_instance(new DeclarativeNetworkManager(this))
{
    qWarning() << "QML NetworkManagerFactory is deprecated. Just create NetworkManager instances.";
}

DeclarativeNetworkManagerFactory::~DeclarativeNetworkManagerFactory()
{
}

DeclarativeNetworkManager *DeclarativeNetworkManagerFactory::instance()
{
    return m_instance;
}

DeclarativeNetworkManager::DeclarativeNetworkManager(QObject *parent)
    : QObject(parent)
    , m_sharedInstance(NetworkManager::sharedInstance())
{
    connect(m_sharedInstance.data(), &NetworkManager::availabilityChanged,
            this, &DeclarativeNetworkManager::availabilityChanged);
    connect(m_sharedInstance.data(), &NetworkManager::stateChanged,
            this, &DeclarativeNetworkManager::stateChanged);
    connect(m_sharedInstance.data(), &NetworkManager::offlineModeChanged,
            this, &DeclarativeNetworkManager::offlineModeChanged);
    connect(m_sharedInstance.data(), &NetworkManager::inputRequestTimeoutChanged,
            this, &DeclarativeNetworkManager::inputRequestTimeoutChanged);
    connect(m_sharedInstance.data(), &NetworkManager::defaultRouteChanged,
            this, &DeclarativeNetworkManager::defaultRouteChanged);
    connect(m_sharedInstance.data(), &NetworkManager::connectedWifiChanged,
            this, &DeclarativeNetworkManager::connectedWifiChanged);
    connect(m_sharedInstance.data(), &NetworkManager::connectedEthernetChanged,
            this, &DeclarativeNetworkManager::connectedEthernetChanged);
    connect(m_sharedInstance.data(), &NetworkManager::sessionModeChanged,
            this, &DeclarativeNetworkManager::sessionModeChanged);
    connect(m_sharedInstance.data(), &NetworkManager::servicesEnabledChanged,
            this, &DeclarativeNetworkManager::servicesEnabledChanged);
    connect(m_sharedInstance.data(), &NetworkManager::technologiesEnabledChanged,
            this, &DeclarativeNetworkManager::technologiesEnabledChanged);
    connect(m_sharedInstance.data(), &NetworkManager::validChanged,
            this, &DeclarativeNetworkManager::validChanged);
    connect(m_sharedInstance.data(), &NetworkManager::connectedChanged,
            this, &DeclarativeNetworkManager::connectedChanged);
    connect(m_sharedInstance.data(), &NetworkManager::connectingChanged,
            this, &DeclarativeNetworkManager::connectingChanged);
    connect(m_sharedInstance.data(), &NetworkManager::connectingWifiChanged,
            this, &DeclarativeNetworkManager::connectingWifiChanged);
    connect(m_sharedInstance.data(), &NetworkManager::technologiesChanged,
            this, &DeclarativeNetworkManager::technologiesChanged);
    connect(m_sharedInstance.data(), &NetworkManager::servicesChanged,
            this, &DeclarativeNetworkManager::servicesChanged);
    connect(m_sharedInstance.data(), &NetworkManager::savedServicesChanged,
            this, &DeclarativeNetworkManager::savedServicesChanged);
    connect(m_sharedInstance.data(), &NetworkManager::wifiServicesChanged,
            this, &DeclarativeNetworkManager::wifiServicesChanged);
    connect(m_sharedInstance.data(), &NetworkManager::cellularServicesChanged,
            this, &DeclarativeNetworkManager::cellularServicesChanged);
    connect(m_sharedInstance.data(), &NetworkManager::ethernetServicesChanged,
            this, &DeclarativeNetworkManager::ethernetServicesChanged);
    connect(m_sharedInstance.data(), &NetworkManager::availableServicesChanged,
            this, &DeclarativeNetworkManager::availableServicesChanged);
    connect(m_sharedInstance.data(), &NetworkManager::servicesListChanged,
            this, &DeclarativeNetworkManager::servicesListChanged);
    connect(m_sharedInstance.data(), &NetworkManager::serviceAdded,
            this, &DeclarativeNetworkManager::serviceAdded);
    connect(m_sharedInstance.data(), &NetworkManager::serviceRemoved,
            this, &DeclarativeNetworkManager::serviceRemoved);
    connect(m_sharedInstance.data(), &NetworkManager::serviceCreated,
            this, &DeclarativeNetworkManager::serviceCreated);
    connect(m_sharedInstance.data(), &NetworkManager::serviceCreationFailed,
            this, &DeclarativeNetworkManager::serviceCreationFailed);
}

DeclarativeNetworkManager::~DeclarativeNetworkManager()
{
}

bool DeclarativeNetworkManager::isAvailable() const
{
    return m_sharedInstance->isAvailable();
}

QString DeclarativeNetworkManager::state() const
{
    return m_sharedInstance->state();
}

bool DeclarativeNetworkManager::offlineMode() const
{
    return m_sharedInstance->offlineMode();
}

NetworkService* DeclarativeNetworkManager::defaultRoute() const
{
    return m_sharedInstance->defaultRoute();
}

NetworkService* DeclarativeNetworkManager::connectedWifi() const
{
    return m_sharedInstance->connectedWifi();
}

NetworkService *DeclarativeNetworkManager::connectedEthernet() const
{
    return m_sharedInstance->connectedEthernet();
}

bool DeclarativeNetworkManager::connectingWifi() const
{
    return m_sharedInstance->connectingWifi();
}

bool DeclarativeNetworkManager::sessionMode() const
{
    return m_sharedInstance->sessionMode();
}

uint DeclarativeNetworkManager::inputRequestTimeout() const
{
    return m_sharedInstance->inputRequestTimeout();
}

bool DeclarativeNetworkManager::servicesEnabled() const
{
    return m_sharedInstance->servicesEnabled();
}

void DeclarativeNetworkManager::setServicesEnabled(bool enabled)
{
    m_sharedInstance->setServicesEnabled(enabled);
}

bool DeclarativeNetworkManager::technologiesEnabled() const
{
    return m_sharedInstance->technologiesEnabled();
}

void DeclarativeNetworkManager::setTechnologiesEnabled(bool enabled)
{
    m_sharedInstance->setTechnologiesEnabled(enabled);
}

bool DeclarativeNetworkManager::isValid() const
{
    return m_sharedInstance->isValid();
}

bool DeclarativeNetworkManager::connected() const
{
    return m_sharedInstance->connected();
}

bool DeclarativeNetworkManager::connecting() const
{
    return m_sharedInstance->connecting();
}

QString DeclarativeNetworkManager::wifiTechnologyPath() const
{
    return m_sharedInstance->wifiTechnologyPath();
}

QString DeclarativeNetworkManager::cellularTechnologyPath() const
{
    return m_sharedInstance->cellularTechnologyPath();
}

QString DeclarativeNetworkManager::bluetoothTechnologyPath() const
{
    return m_sharedInstance->bluetoothTechnologyPath();
}

QString DeclarativeNetworkManager::gpsTechnologyPath() const
{
    return m_sharedInstance->gpsTechnologyPath();
}

QString DeclarativeNetworkManager::ethernetTechnologyPath() const
{
    return m_sharedInstance->ethernetTechnologyPath();
}

QStringList DeclarativeNetworkManager::servicesList(const QString &tech)
{
    return m_sharedInstance->servicesList(tech);
}

QStringList DeclarativeNetworkManager::savedServicesList(const QString &tech)
{
    return m_sharedInstance->savedServicesList(tech);
}

QStringList DeclarativeNetworkManager::availableServices(const QString &tech)
{
    return m_sharedInstance->availableServices(tech);
}

QStringList DeclarativeNetworkManager::technologiesList()
{
    return m_sharedInstance->technologiesList();
}

QString DeclarativeNetworkManager::technologyPathForService(const QString &path)
{
    return m_sharedInstance->technologyPathForService(path);
}

QString DeclarativeNetworkManager::technologyPathForType(const QString &type)
{
    return m_sharedInstance->technologyPathForType(type);
}

void DeclarativeNetworkManager::resetCountersForType(const QString &type)
{
    m_sharedInstance->resetCountersForType(type);
}

void DeclarativeNetworkManager::setOfflineModeProperty(bool offlineMode)
{
    m_sharedInstance->setOfflineMode(offlineMode);
}

void DeclarativeNetworkManager::setSessionModeProperty(bool sessionMode)
{
    m_sharedInstance->setSessionMode(sessionMode);
}

void DeclarativeNetworkManager::setOfflineMode(bool offlineMode)
{
    qWarning() << "NetworkManager:setOfflineMode() is deprecated. Assign to property";
    setOfflineModeProperty(offlineMode);
}

void DeclarativeNetworkManager::setSessionMode(bool sessionMode)
{
    qWarning() << "NetworkManager:setSessionMode() is deprecated. Assign to property";
    setSessionModeProperty(sessionMode);
}

void DeclarativeNetworkManager::registerAgent(const QString &path)
{
    m_sharedInstance->registerAgent(path);
}

void DeclarativeNetworkManager::unregisterAgent(const QString &path)
{
    m_sharedInstance->unregisterAgent(path);
}

void DeclarativeNetworkManager::registerCounter(const QString &path, quint32 accuracy, quint32 period)
{
    m_sharedInstance->registerCounter(path, accuracy, period);
}

void DeclarativeNetworkManager::unregisterCounter(const QString &path)
{
    m_sharedInstance->unregisterCounter(path);
}

bool DeclarativeNetworkManager::createService(
        const QVariantMap &settings,
        const QString &tech,
        const QString &service,
        const QString &device)
{
    return m_sharedInstance->createService(settings, tech, service, device);
}

QString DeclarativeNetworkManager::createServiceSync(
        const QVariantMap &settings,
        const QString &tech,
        const QString &service,
        const QString &device)
{
    return m_sharedInstance->createServiceSync(settings, tech, service, device);
}
