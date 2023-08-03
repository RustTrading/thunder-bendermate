#pragma once

#include "BObject.h"
#include "Net/BClientSession.h"
#include "Net/BMonitorService.h"
#include "Events/BEvent.h"

B_CLASS(BMonDirectClient, BClientSession)
{
    B_DECLARE(BMonDirectClient);

#include SERVICE_TAKE
    DECLARE_BENDER_MONITOR_SERVICE_METHODS

#include SERVICE_GIVE
    DECLARE_BENDER_MONITOR_SERVICE_CALLBACKS

public:
    enum
    {
        WAIT_INTERVAL_CONNECT_MS = 10000,
        WAIT_INTERVAL_FIGMENT_MS = 7200000
    };

    B_STATIC BMonDirectClientPtr createObject(
        const BEndpointPtr& endpoint);

    bool waitForConnect(BInt64 ms);

private:
    void onConnected() override;
    void onDisconnected(const boost::system::error_code& error) override;

    void onSensors(const BSensorRepositoryPtr& data);
    void onRobotList(const std::vector<BRobotStatusPtr>& data);
    void onRobotListUpdate(const std::vector<BRobotStatusPtr>& data);
    void onGatewayList(const std::vector<BRobotStatusPtr>& data);
    void onGatewayListUpdate(const std::vector<BRobotStatusPtr>& data);
    void onAlarm2(int level, const BString& message, BInt64 moment);
    void onStatus(const BString& runtimeConfig, const BString& version, bool wasShutdown);
    void onStatus2(const BString& runtimeConfig, const BString& version)
    { onStatus(runtimeConfig, version, false); }


private:
//    void onIndicator(BDateTime, BVariableKey, double) {}
//    void onBook(SRVKEY, BIsinID, const BBookPtr&) {}

    mutable std::mutex m_mutex;
    mutable std::condition_variable m_condition;
    bool m_connected{false};
};
