#include "BMonDirectClient.h"

B_IMPLEMENT(BMonDirectClient);

BMonDirectClientPtr BMonDirectClient::createObject(
    const BEndpointPtr& endpoint)
{
    BMonDirectClientPtr res = BClientSession::createObject(BDefs::kProtoBinary, endpoint, BMonDirectClient::type(), {}, 5000);

    return res;
}

void BMonDirectClient::onConnected()
{
//    std::cout << "In BMonDirectClient::onConnected" << std::endl;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = true;
    m_condition.notify_one();
}

void BMonDirectClient::onDisconnected(const boost::system::error_code& /*error*/)
{
//    std::cout << "In BMonDirectClient::onDisconnected" << std::endl;
}

void BMonDirectClient::onAlarm2(int level, const BString& message, BInt64 moment)
{
//    std::cout << "In BMonDirectClient::onAlarm2" << std::endl;
}

void BMonDirectClient::onSensors(const BSensorRepositoryPtr& data)
{
//    std::cout << "In BMonDirectClient::onSensors" << std::endl;
//    AcquireGIL lockGIL;
//    BArray<unsigned char, BArrayAllocatorCopyMemory<unsigned char> >
//    m_pyCallback.attr("onSensors")(decodeServerID(srv).toAnsi());
//    PyGILState_STATE state = PyGILState_Ensure();
//    m_pyCallback.attr("onSensors")();
//    PyGILState_Release(state);
//    m_pyCallback.attr("onSensors")();
}

void BMonDirectClient::onRobotList(const std::vector<BRobotStatusPtr>& data)
{
//    std::cout << "In BMonDirectClient::onRobotList" << std::endl;
}

void BMonDirectClient::onRobotListUpdate(const std::vector<BRobotStatusPtr>& data)
{
}

void BMonDirectClient::onGatewayList(const std::vector<BRobotStatusPtr>& data)
{
}

void BMonDirectClient::onGatewayListUpdate(const std::vector<BRobotStatusPtr>& data)
{
}

bool BMonDirectClient::waitForConnect(BInt64 ms)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_connected)
        return true;
    m_condition.wait_for(lock, std::chrono::milliseconds(ms));
    return m_connected;
}

void BMonDirectClient::onStatus(const BString& runtimeConfig, const BString& revision, bool wasShutdown)
{
//    BServerStatusPtr status = BServerStatus::createObject();
////    status->Name = m_directName;
//    status->Name = encodeServerID(L"*RAW");
//    status->ManagementOnline = true;
//    status->RemoteCoreOnline = true;
//    status->WasShutdown = wasShutdown;
//    status->RuntimeConfig = runtimeConfig;
//    status->Revision =  revision;
}

