#include "BMonDaemonClient.h"
#include "Net/BMonitorDaemonConfig.h"

B_IMPLEMENT(BMonDaemonClient);

BMonDaemonClientPtr BMonDaemonClient::createObject(
    const BEndpointPtr& endpoint, BMonDaemonCallbacks* callbacks)
{
    // TODO: more attention to instantiation
    BMonDaemonClientPtr res = BClientSession::createObject(BDefs::kProtoBinary, endpoint, BMonDaemonClient::type(), {}, 5000);
    res->m_callbacks = callbacks;

    return res;
}

void BMonDaemonClient::onConnected()
{
//    std::cout << "In BMonDaemonClient::onConnected" << std::endl;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = true;
    m_condition.notify_one();
}

void BMonDaemonClient::onDisconnected(const boost::system::error_code& /*error*/)
{
   // std::cout << "In BMonDaemonClient::onDisconnected" << std::endl;
}


void BMonDaemonClient::onAlarm2(SRVKEY srv, int level, const BString& message, BInt64 moment)
{
//    std::cout << "In BMonDaemonClient::onAlarm2" << std::endl;
    m_callbacks->onAlarm2(srv, level, message, moment);
}

void BMonDaemonClient::onSensors(SRVKEY srv, const BSensorRepositoryPtr& data)
{
//    std::cout << "In BMonDaemonClient::onSensors" << std::endl;

    // TODO: probably parse sensors first
    m_callbacks->onSensors(srv, data->getData());
//    AcquireGIL lockGIL;
//    BArray<unsigned char, BArrayAllocatorCopyMemory<unsigned char> >
//    m_pyCallback.attr("onSensors")(decodeServerID(srv).toAnsi());
//    PyGILState_STATE state = PyGILState_Ensure();
//    m_pyCallback.attr("onSensors")();
//    PyGILState_Release(state);
//    m_pyCallback.attr("onSensors")();
}

void BMonDaemonClient::onDaemonConfig(const BString& config, const BString& rev, bool isAdmin)
{
//    std::cout << "In BMonDaemonClient::onDaemonConfig" << std::endl;
//    auto daemonConfigObject = BDaemonConfig::cast(BXml::deserialize(BDaemonConfig::type(), config));
}

void BMonDaemonClient::onDaemonConfig2(const BString& config, const BString& rev)
{
//    std::cout << "In BMonDaemonClient::onDaemonConfig2" << std::endl;
}

void BMonDaemonClient::onServerList(const std::vector<BServerStatusPtr>& data)
{
//    std::cout << "In BMonDaemonClient::onServerList" << std::endl;
}

void BMonDaemonClient::onServerListUpdate(const std::vector<BServerStatusPtr>& data)
{
//    std::cout << "In BMonDaemonClient::onServerListUpdate" << std::endl;
}

void BMonDaemonClient::onRobotList(SRVKEY srv, const std::vector<BRobotStatusPtr>& data)
{
//    std::cout << "In BMonDaemonClient::onRobotList" << std::endl;
}

void BMonDaemonClient::onRobotListUpdate(SRVKEY srv, const std::vector<BRobotStatusPtr>& data)
{
}

void BMonDaemonClient::onGatewayList(SRVKEY srv, const std::vector<BRobotStatusPtr>& data)
{
}

void BMonDaemonClient::onGatewayListUpdate(SRVKEY srv, const std::vector<BRobotStatusPtr>& data)
{
}

bool BMonDaemonClient::waitForConnect(BInt64 ms)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_connected)
        return true;
    m_condition.wait_for(lock, std::chrono::milliseconds(ms));
    return m_connected;
}
