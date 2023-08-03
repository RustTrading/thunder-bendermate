#include "BMonMate.h"

namespace MonMate {

// Flags from the platform
// TODO: use BRobotStatus::Flags enum (now there is a problem with BRobotStatus `BInt16 Flags` field)

class ServerStatus {
public:
    ServerStatus(const BServerStatusPtr status): m_status(status) {}

    SRVKEY Name;
    bool ManagementOnline;
    bool RemoteCoreOnline;
    bool WasShutdown;
    BString RuntimeConfig;
    BString Revision;

    std::string name() const {
        return decodeServerID(m_status->Name).toAnsi();
    }

    bool management_online() const {
        return m_status->ManagementOnline;
    }

    bool remote_core_online() const {
        return m_status->RemoteCoreOnline;
    }

    bool was_shutdown() const {
        return m_status->WasShutdown;
    }

    std::string runtime_config() const {
        return m_status->RuntimeConfig.toAnsi();
    }

    std::string revision() const {
        return m_status->Revision.toAnsi();
    }

private:
    BServerStatusPtr m_status;
};

class AlarmEvent {
public:
    AlarmEvent(int level, const BString& message, BInt64 moment): m_level(level), m_message(message), m_moment(moment){}

    // TODO: Return enum instead
    int level() const {
        return m_level;
    }

    std::string message() const {
        return m_message.toAnsi();
    }

    BDateTime moment() const {
        return m_moment;
    }


private:
    int m_level;
    BString m_message;
    BDateTime m_moment;
};

DaemonConnector::DaemonConnector()
    : DaemonConnector(boost::make_shared<RustCallback>())
{}

DaemonConnector::DaemonConnector(
    boost::shared_ptr<RustCallback> callbacks)
    : m_pyCallbacks(callbacks)
    , m_hasOnSensors(false)
    , m_hasOnAlarm(false)
{
}

void
DaemonConnector::connect(const std::wstring &host, const std::uint16_t port) {
    BEndpointPtr endpoint = BEndpoint::createObject(host.c_str(), port);
    m_session = BMonDaemonClient::createObject(endpoint, this);
    m_session->setReconnectDelay(1000);
    if (!m_session->waitForConnect(BMonDaemonClient::WAIT_INTERVAL_CONNECT_MS))
        throw BException(L"DaemonConnector: failed to connect to server");
}

int
DaemonConnector::login(const std::wstring &user, const std::wstring &password) {
    auto salt = m_session->storedSalt(user.c_str());
    auto password_hash = hashPassword(password.c_str(), salt);
    AuthServiceReply reply = m_session->login(user.c_str(), password_hash);
    //std::cout << reply.first << " --> " << reply.second.toAnsi() << std::endl;
    return reply.first;
}

int
DaemonConnector::loginByHash(const std::wstring &user, const std::wstring &hash) {
    AuthServiceReply reply = m_session->login(user.c_str(), hash.c_str());
    //std::cout << reply.first << " --> " << reply.second.toAnsi() << std::endl;
    return reply.first;
}

void
DaemonConnector::setRobotConfig(std::wstring server, std::wstring robot, std::wstring config) {
    m_session->setRobotConfig(encodeServerID(server.c_str()), robot.c_str(), config.c_str());
}

void
DaemonConnector::setGatewayConfig(std::wstring server, std::wstring gateway, std::wstring config) {
    m_session->setGatewayConfig(encodeServerID(server.c_str()), gateway.c_str(), config.c_str());
}

boost::shared_ptr<RobotStatus>
DaemonConnector::statusRobot(std::wstring server, std::wstring robot) {
    try {
        auto robotStatus = m_session->statusRobot(encodeServerID(server.c_str()), robot.c_str());
        return boost::make_shared<RobotStatus>(robotStatus);
    } catch (...) {
        return boost::shared_ptr<RobotStatus>();
    }
}

boost::shared_ptr<std::list<boost::shared_ptr<RobotStatus>>>
DaemonConnector::statusRobots(std::wstring server, std::wstring robots) {
    auto plist = boost::make_shared<std::list<boost::shared_ptr<RobotStatus>>>();

    std::vector<BRobotStatusPtr> robotStatuses = m_session->statusRobots(encodeServerID(server.c_str()), robots.c_str());

    for (auto robotStatus: robotStatuses) {
        plist->push_back(boost::make_shared<RobotStatus>(robotStatus));
    }
    return plist;
}

boost::shared_ptr<RobotStatus>
DaemonConnector::statusGateway(std::wstring server, std::wstring gateway) {
    try {
        auto gatewayStatus = m_session->statusGateway(encodeServerID(server.c_str()), gateway.c_str());

        return boost::make_shared<RobotStatus>(gatewayStatus);
    } catch (...) {
        return boost::shared_ptr<RobotStatus>();
    }
}

boost::shared_ptr<std::list<boost::shared_ptr<RobotStatus>>>
DaemonConnector::statusGateways(std::wstring server, std::wstring gateways) {
    auto plist = boost::make_shared<std::list<boost::shared_ptr<RobotStatus>>>();
    std::vector<BRobotStatusPtr> gatewayStatuses = m_session->statusGateways(encodeServerID(server.c_str()), gateways.c_str());
    //printf ("encode : %u",encodeServerID(server.c_str()));
    for (auto gatewayStatus: gatewayStatuses) {
        plist->push_back(boost::make_shared<RobotStatus>(gatewayStatus));
    }
    return plist;
}

std::string
DaemonConnector::gatewayConfig(std::wstring server, std::wstring gateway) {
    try {
        BString config = m_session->gatewayConfig(encodeServerID(server.c_str()), gateway.c_str());
        //printf ("encode : %u",encodeServerID(server.c_str()));
        return config.toAnsi();
    } catch (...) {
        return std::string();
    }
}

std::vector<SensorEvent> parseSensors(BArray<BUInt8> data) {
    auto filer = BFiler::createObject(BMemoryStream::createObject(&(data[0]), data.size(), BDefs::kZipCompression));
    auto version = filer->rdInt32();  // TODO: check that version >= 1
    B_UNUSED(version)
    int sz = filer->rdInt32();
    std::vector<SensorEvent> sensors;
    for (int i = 0; i < sz; i++)
    {
        BString category = filer->rdString();
        BString rawName = filer->rdString();
        double dVal = filer->rdDouble();
        BString sVal = filer->rdString();
        BInt32 count = filer->rdInt32();
        double min = filer->rdDouble();
        double max = filer->rdDouble();
        double total = filer->rdDouble();
        BDateTime moment = filer->rdDateTime();
        auto sensor = SensorEvent(
            category,
            rawName,
            dVal,
            sVal,
            count,
            min,
            max,
            total,
            moment
        );
//        if (sensor.category() != "Variable")
//            continue;
        sensors.push_back(sensor);
    }
    return sensors;
}


boost::shared_ptr<std::list<boost::shared_ptr<RobotOrder>>>
DaemonConnector::getAllOrders(std::wstring server) {
    auto data = m_session->getAllOrders(encodeServerID(server.c_str()));
    auto plist = boost::make_shared<std::list<boost::shared_ptr<RobotOrder>>>();
    for (auto order: data){
        plist->push_back(boost::make_shared<RobotOrder>(order));
    }
    return plist;
}

boost::shared_ptr<std::list<boost::shared_ptr<SensorEvent>>>
DaemonConnector::sensors(std::wstring server)
{
    BArray<BUInt8> data = m_session->sensors(encodeServerID(server.c_str()))->getData();
    auto plist = boost::make_shared<std::list<boost::shared_ptr<SensorEvent>>>();
    for (auto sensor : parseSensors(data)){
        plist->push_back(boost::make_shared<SensorEvent>(sensor));
    }
    return plist;
}

boost::shared_ptr<std::list<boost::shared_ptr<SensorVariable>>>
DaemonConnector::variables(std::wstring server)
{
    BArray<BUInt8> data = m_session->sensors(encodeServerID(server.c_str()))->getData();
    auto plist = boost::make_shared<std::list<boost::shared_ptr<SensorVariable>>>();
    for (auto sensor : parseSensors(data)){
        if (sensor.category() != "Variable") {
            continue;
        }
        plist->push_back(boost::make_shared<SensorVariable>(sensor.rawName(), sensor.dVal()));
    }
    return plist;
}

boost::shared_ptr<std::list<boost::shared_ptr<SensorPosition>>>
DaemonConnector::positions(std::wstring server)
{
    BArray<BUInt8> data = m_session->sensors(encodeServerID(server.c_str()))->getData();
    auto plist = boost::make_shared<std::list<boost::shared_ptr<SensorPosition>>>();
    for (auto sensor : parseSensors(data)){
        if (sensor.category() != "Position") {
            continue;
        }
        BArray<BString> parts = sensor.splitName();
        if (parts.size() == 4) {
            auto account = parts[0];
            auto name = parts[1];
            auto iclass = parts[2];
            auto gateway = parts[3];
            plist->push_back(boost::make_shared<SensorPosition>(account, name, iclass, gateway, sensor.dVal()));
        }
    }
    return plist;
}

void
DaemonConnector::addRobot(std::wstring server, std::wstring robot, std::wstring config)
{
    DaemonConnector::addRobot(server, robot, config, false);
}

void
DaemonConnector::addRobot(std::wstring server, std::wstring robot, std::wstring config, bool overwrite = false)
{
    this->m_session->addRobot(encodeServerID(server.c_str()), robot.c_str(), config.c_str(), overwrite);
}

void
DaemonConnector::startRobot(std::wstring server, std::wstring robot)
{
    this->m_session->startRobot(encodeServerID(server.c_str()), robot.c_str());
}

void
DaemonConnector::startRobots(std::wstring server, std::wstring robots)
{
    this->m_session->startRobots(encodeServerID(server.c_str()), robots.c_str());
}

void
DaemonConnector::stopRobot(std::wstring server, std::wstring robot)
{
    this->m_session->stopRobot(encodeServerID(server.c_str()), robot.c_str());
}

void
DaemonConnector::stopRobots(std::wstring server, std::wstring robots)
{
    this->m_session->stopRobots(encodeServerID(server.c_str()), robots.c_str());
}

void
DaemonConnector::startGateway(std::wstring server, std::wstring gateway)
{
    this->m_session->startGateway(encodeServerID(server.c_str()), gateway.c_str());
}

void
DaemonConnector::startGateways(std::wstring server, std::wstring gateways)
{
    this->m_session->startGateways(encodeServerID(server.c_str()), gateways.c_str());
}

void
DaemonConnector::stopGateway(std::wstring server, std::wstring gateway)
{
    this->m_session->stopGateway(encodeServerID(server.c_str()), gateway.c_str());
}

void
DaemonConnector::stopGateways(std::wstring server, std::wstring gateways)
{
    this->m_session->stopGateways(encodeServerID(server.c_str()), gateways.c_str());
}

void
DaemonConnector::deleteRobot(std::wstring server, std::wstring robot)
{
    this->m_session->delRobot(encodeServerID(server.c_str()), robot.c_str());
}

void
DaemonConnector::onSensors(SRVKEY srv, const BArray<BUInt8>& data)
{
//    std::cout << "DaemonConnector::onSensors" << std::endl;

    if (m_hasOnSensors){
    }
}

void
DaemonConnector::onAlarm2(SRVKEY srv, int level, const BString& message, BInt64 moment)
{
//    std::cout << "DaemonConnector::onAlarm2" << std::endl;

    if (m_hasOnAlarm){
    }
}

/////////////////////////////////////////////////////////////////////////////////
class DirectConnector
{
public:
    DirectConnector();
    // TODO: Add callbacks
//    DirectConnector(const py::object callbacks);

    void
    connect(const std::wstring &host, const std::uint16_t port);

    void
    setRobotConfig(std::wstring robot, std::wstring config);

    void
    setGatewayConfig(std::wstring gateway, std::wstring config);

    boost::shared_ptr<ServerStatus>
    statusServer();

    boost::shared_ptr<RobotStatus>
    statusRobot(std::wstring robot);

    boost::shared_ptr<std::list<boost::shared_ptr<RobotStatus>>>
    statusRobots(std::wstring robots);

    boost::shared_ptr<RobotStatus>
    statusGateway(std::wstring gateway);

    boost::shared_ptr<std::list<boost::shared_ptr<RobotStatus>>>
    statusGateways(std::wstring gateways);

    std::string
    gatewayConfig(std::wstring gateway);

    boost::shared_ptr<std::list<std::pair<std::string, double>>>
    sensors();

    void
    addRobot(std::wstring robot, std::wstring config);

    void
    addRobot(std::wstring robot, std::wstring config, bool overwrite);

    void
    startRobot(std::wstring robot);

    void
    startRobots(std::wstring robots);

    void
    stopRobot(std::wstring robot);

    void
    stopRobots(std::wstring robots);

    void
    startGateway(std::wstring gateway);

    void
    startGateways(std::wstring gateways);

    void
    stopGateway(std::wstring gateway);

    void
    stopGateways(std::wstring gateways);

    void
    deleteRobot(std::wstring robot);

private:
    BMonDirectClientPtr m_session;
};

DirectConnector::DirectConnector()
{}

void
DirectConnector::connect(const std::wstring &host, const std::uint16_t port)
{
    BEndpointPtr endpoint = BEndpoint::createObject(host.c_str(), port);
    m_session = BMonDirectClient::createObject(endpoint);
    m_session->setReconnectDelay(1000);
    if (!m_session->waitForConnect(BMonDirectClient::WAIT_INTERVAL_CONNECT_MS))
        throw BException(L"DirectConnector: failed to connect to server");
}

void
DirectConnector::setRobotConfig(std::wstring robot, std::wstring config)
{
    m_session->setRobotConfig(robot.c_str(), config.c_str());
}

boost::shared_ptr<ServerStatus>
DirectConnector::statusServer()
{
    try
    {
        auto serverStatus = m_session->statusServer();
        return boost::make_shared<ServerStatus>(serverStatus);
    }
    catch (...)
    {
        return boost::shared_ptr<ServerStatus>();
    }
}

boost::shared_ptr<RobotStatus>
DirectConnector::statusRobot(std::wstring robot)
{
    try
    {
        auto robotStatus = m_session->statusRobot(robot.c_str());
        return boost::make_shared<RobotStatus>(robotStatus);
    }
    catch (...)
    {
        return boost::shared_ptr<RobotStatus>();
    }
}


boost::shared_ptr<std::list<boost::shared_ptr<RobotStatus>>>
DirectConnector::statusRobots(std::wstring robots)
{
    auto plist = boost::make_shared<std::list<boost::shared_ptr<RobotStatus>>>();

    std::vector<BRobotStatusPtr> robotStatuses = m_session->statusRobots(robots.c_str());

    for (auto robotStatus: robotStatuses) {
        plist->push_back(boost::make_shared<RobotStatus>(robotStatus));
    }
    return plist;
}

void
DirectConnector::setGatewayConfig(std::wstring gateway, std::wstring config)
{
    m_session->setGatewayConfig(gateway.c_str(), config.c_str());
}

boost::shared_ptr<RobotStatus>
DirectConnector::statusGateway(std::wstring gateway)
{
    try
    {
        auto gatewayStatus = m_session->statusGateway(gateway.c_str());

        return boost::make_shared<RobotStatus>(gatewayStatus);
    }
    catch (...)
    {
        return boost::shared_ptr<RobotStatus>();
    }
}

boost::shared_ptr<std::list<boost::shared_ptr<RobotStatus>>>
DirectConnector::statusGateways(std::wstring gateways)
{
    auto plist =  boost::make_shared<std::list<boost::shared_ptr<RobotStatus>>>();

    std::vector<BRobotStatusPtr> gatewayStatuses = m_session->statusGateways(gateways.c_str());

    for (auto gatewayStatus: gatewayStatuses) {
        plist->push_back(boost::make_shared<RobotStatus>(gatewayStatus));
    }
    return plist;
}


std::string
DirectConnector::gatewayConfig(std::wstring gateway)
{
    try
    {
        BString config = m_session->gatewayConfig(gateway.c_str());

        return config.toAnsi();
    }
    catch (...)
    {
        return std::string();
    }
}

boost::shared_ptr<std::list<std::pair<std::string, double>>>
DirectConnector::sensors()
{
    BArray<BUInt8> data = m_session->sensors(nullptr)->getData();
    auto filer = BFiler::createObject(BMemoryStream::createObject(&(data[0]), data.size(), BDefs::kZipCompression));
    auto version = filer->rdInt32();  // TODO: check that version >= 1
    int sz = filer->rdInt32();

    auto plist = boost::make_shared<std::list<std::pair<std::string, double>>>();
    for (int i = 0; i < sz; i++)
    {
        auto category = filer->rdString().toAnsi();
        auto rawName = filer->rdString().toAnsi();
//        const BStringList parsedName = rawName.split('|');
//        quint32 catHash = qHash(category);
//        quint32 rawNameHash = qHash(rawName);
//        SensorsModel::SensorItem* sensor = nullptr;

//        sensors.resize(sensors.size() + 1);
//        sensor = &sensors.back();
//        sensor->serverKey = serverKey;
//        sensor->catHash = catHash;
//        sensor->rawNameHash = rawNameHash;
//        sensor->server = server;

//        sensor->category = category;
//        sensor->rawName = rawName;

//        if (parsedName.size() == 4)
//        {
//            sensor->account = parsedName[0];
//            sensor->name = parsedName[1] + (parsedName[2].isEmpty() ? BString() : BStringLiteral("|%1").arg(parsedName[2]));
//            sensor->gateway = parsedName[3];
//        }

        auto dVal = filer->rdDouble();
        auto sVal = filer->rdString().toAnsi();
//        if (!sVal.isEmpty())
//            sensor->value = sVal;
//        else
//        {
//            if (dVal != BDefs::NA)
//                sensor->value = dVal;
//            else
//                sensor->value = BString("--- was reset ---");
//        }

        auto count = filer->rdInt32();
        auto min = filer->rdDouble();
        auto max = filer->rdDouble();
        auto total = filer->rdDouble();

//        if (sensor->min == BDefs::NA)
//            sensor->avg = BDefs::NA;
//        else if (sensor->count != 1)
//            sensor->avg /= sensor->count;

        auto moment = filer->rdDateTime();
//        if (moment.ticks())
//        {
//            moment = moment.toLocal();
//            sensor->moment = BStringLiteral("%1 %2").arg(Utils::toBString(moment.toDateString())).arg(Utils::toBString(moment.toTimeString()));
//        }

        // TODO: return other types of sensors (positions etc)
        if (category != "Variable") {
            continue;
        }

//        std::cout
//            << category << " "
//            << rawName << " "
//            << dVal << " "
//            << "'" << sVal << "'" << " "
//            << count << " "
//            << min << " "
//            << max << " "
//            << total << " "
//            << moment.toDateString().toAnsi() << " "
//            << std::endl;

        // TODO: return verbose info about sensors if requested
        B_UNUSED(version)
        B_UNUSED(category)
        B_UNUSED(sVal)
        B_UNUSED(count)
        B_UNUSED(min)
        B_UNUSED(max)
        B_UNUSED(total)
        B_UNUSED(moment)
        plist->push_back(std::make_pair(rawName, dVal));

    }
    return plist;

}

void
DirectConnector::addRobot(std::wstring robot, std::wstring config)
{
    DirectConnector::addRobot(robot, config, false);
}

void
DirectConnector::addRobot(std::wstring robot, std::wstring config, bool overwrite = false)
{
    m_session->addRobot(robot.c_str(), config.c_str(), overwrite);
}

void
DirectConnector::startRobot(std::wstring robot)
{
    m_session->startRobot(robot.c_str());
}

void
DirectConnector::startRobots(std::wstring robots)
{
    m_session->startRobots(robots.c_str());
}

void
DirectConnector::stopRobot(std::wstring robot)
{
    m_session->stopRobot(robot.c_str());
}

void
DirectConnector::stopRobots(std::wstring robots)
{
    m_session->stopRobots(robots.c_str());
}

void
DirectConnector::startGateway(std::wstring gateway)
{
    m_session->startGateway(gateway.c_str());
}

void
DirectConnector::startGateways(std::wstring gateways)
{
    m_session->startGateways(gateways.c_str());
}

void
DirectConnector::stopGateway(std::wstring gateway)
{
    m_session->stopGateway(gateway.c_str());
}

void
DirectConnector::stopGateways(std::wstring gateways)
{
    m_session->stopGateways(gateways.c_str());
}

void
DirectConnector::deleteRobot(std::wstring robot)
{
    m_session->delRobot(robot.c_str());
}

/////////////////////////////////////////////////////////////////////////////////
class DataConnector: public BMonDataCallbacks
{
public:
    DataConnector();
    DataConnector(boost::shared_ptr<RustCallback> callback);

    void
    connect(const std::wstring &host, const std::uint16_t port, const std::uint16_t receiverPort);

    void
    bindToBook(std::wstring name, std::wstring className, const std::wstring exchangeName);

    void
    unbindBook(std::wstring name, std::wstring className, const std::wstring exchangeName);

    void
    bindToDeal(std::wstring name, std::wstring className, const std::wstring exchangeName);

    void
    unbindDeal(std::wstring name, std::wstring className, const std::wstring exchangeName);

    void
    bindToVariable(std::wstring name);

    void
    unbindVariable(std::wstring name);

    void onVariable(BDateTime moment, BUInt64 name, double value, bool lastOne) override;
    void onBook(BIsinID instrumentId, const BBookPtr& book) override;
    void onDeal(const BDealPtr& deal) override;
    void onFormula(BRemoteFormulaId id, double value) override;

private:
    BMonReceiverPtr m_receiver;
    BMonRemoteClientPtr m_session;

    boost::shared_ptr<RustCallback> m_pyCallback;
    bool m_hasOnVariable;
    bool m_hasOnBook;
    bool m_hasOnDeal;
    bool m_hasOnFormula;
};

DataConnector::DataConnector()
    : DataConnector(boost::make_shared<RustCallback>())
{}

DataConnector::DataConnector(
    boost::shared_ptr<RustCallback> callback)
    : m_pyCallback(callback)
    , m_hasOnVariable(false)
    , m_hasOnBook(false)
    , m_hasOnDeal(false)
    , m_hasOnFormula(false)
{
    //if (py::hasattr(m_pyCallback, "on_variable"))
       // m_hasOnVariable = true;

    //if (py::hasattr(m_pyCallback, "on_book"))
        //m_hasOnBook = true;

    //if (py::hasattr(m_pyCallback, "on_deal"))
        //m_hasOnDeal = true;

    //if (py::hasattr(m_pyCallback, "on_formula"))
        //m_hasOnFormula = true;
}

void
DataConnector::connect(
        const std::wstring &host,
        const std::uint16_t port,
        const std::uint16_t receiverPort = 3099)
{
    // TODO: handle port-in-use exception
    m_receiver = BMonReceiver::createObject(receiverPort, this);
    // std::cout << "open UDP connection on port " << m_receiver->port() << std::endl;
    m_session = BMonRemoteClient::createObject(
            BEndpoint::createObject(host.c_str(), port),
            BEndpoint::createObject(BString::kEmpty, m_receiver->port()));
    m_session->setReconnectDelay(1000);
    if (!m_session->waitForConnect(BMonRemoteClient::WAIT_INTERVAL_CONNECT_MS))
        throw BException(L"DirectConnector: failed to connect to server");
}

void
DataConnector::bindToVariable(std::wstring name) {
    // std::cout << "DataConnector::bindToVariable" << std::endl;
    m_session->bindToVariable(encodeIndicatorID(name.c_str()));
}

void
DataConnector::unbindVariable(std::wstring name) {
    // std::cout << "DataConnector::unbindVariable" << std::endl;
    m_session->unbindVariable(encodeIndicatorID(name.c_str()));
}

void
DataConnector::bindToBook(std::wstring name, std::wstring className, const std::wstring exchangeName) {
    // std::cout << "DataConnector::bindToBook" << std::endl;
    m_session->bindToBookEx(name.c_str(), className.c_str(), exchangeName.c_str());
}

void
DataConnector::unbindBook(std::wstring name, std::wstring className, const std::wstring exchangeName) {
    // std::cout << "DataConnector::unbindBook" << std::endl;
    m_session->unbindBookEx(name.c_str(), className.c_str(), exchangeName.c_str());
}

void
DataConnector::bindToDeal(std::wstring name, std::wstring className, const std::wstring exchangeName) {
    // std::cout << "DataConnector::bindToDeal" << std::endl;
    m_session->bindToDealEx(name.c_str(), className.c_str(), exchangeName.c_str());
}

void
DataConnector::unbindDeal(std::wstring name, std::wstring className, const std::wstring exchangeName) {
    // std::cout << "DataConnector::unbindDeal" << std::endl;
    m_session->unbindDealEx(name.c_str(), className.c_str(), exchangeName.c_str());
}

void
DataConnector::onVariable(BDateTime moment, BUInt64 name, double value, bool lastOne) {
    // std::cout << "DataConnector::onVariable" << std::endl;
    // TODO: Actual variable handling
    // if (m_hasOnVariable) {
        //m_pyCallback.attr("on_variable")(value);
    // }
//    B_UNUSED(lastOne);
//
//    if (moment.isNull())
//        moment = BDateTime::now();
//
//    if (BDefs::NA != value)
//        m_context->setIndicator(moment, key.name, value);
//    else
//        m_context->resetIndicator(moment, key.name);
}

void
DataConnector::onBook(BIsinID instrumentId, const BBookPtr& book) {
    if (m_hasOnBook) {
        std::cout << "Calling onBook TODO"<< std::endl;
    }
}

void
DataConnector::onDeal(const BDealPtr& deal) {
    if (m_hasOnDeal) {
         std::cout << "Calling onDeal TODO"<< std::endl;
    }

}

void
DataConnector::onFormula(BRemoteFormulaId id, double value) {}

}
