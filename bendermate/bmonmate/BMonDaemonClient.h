#pragma once

#include "BObject.h"
#include "Net/BClientSessionPasswdAuth.h"
#include "Net/BMonitorService.h"
#include "Events/BEvent.h"

class BMonDaemonCallbacks
{
public:
    virtual void onSensors(SRVKEY srv, const BArray<BUInt8>& data) = 0;
    virtual void onAlarm2(SRVKEY srv, int level, const BString& message, BInt64 moment) = 0;

//    virtual void onVariable(BDateTime moment, BUInt64 name, double value, bool lastOne) = 0;
//    virtual void onBook(BIsinID instrumentId, const BBookPtr& book) = 0;
//    virtual void onDeal(const BDealPtr& deal) = 0;
//    virtual void onFormula(BRemoteFormulaId id, double value) = 0;
};

B_CLASS(BMonDaemonClient, BClientSessionPasswdAuth)
{
    B_DECLARE(BMonDaemonClient);

#include SERVICE_TAKE
    DECLARE_BENDER_DAEMON_SERVICE_METHODS

#include SERVICE_GIVE
    DECLARE_BENDER_DAEMON_SERVICE_CALLBACKS

public:
    enum
    {
        WAIT_INTERVAL_CONNECT_MS = 10000,
        WAIT_INTERVAL_FIGMENT_MS = 7200000
    };

    B_STATIC BMonDaemonClientPtr createObject(
        const BEndpointPtr& endpoint, BMonDaemonCallbacks* callbacks);

    bool waitForConnect(BInt64 ms);
private:
    void onConnected() override;
    void onDisconnected(const boost::system::error_code& error) override;

    void onDaemonConfig(const BString& config, const BString& rev, bool isAdmin);
    void onDaemonConfig2(const BString& config, const BString& rev);
    void onServerList(const std::vector<BServerStatusPtr>& data);
    void onServerListUpdate(const std::vector<BServerStatusPtr>& data);
    void onSensors(SRVKEY srv, const BSensorRepositoryPtr& data);
    void onRobotList(SRVKEY srv, const std::vector<BRobotStatusPtr>& data);
    void onRobotListUpdate(SRVKEY srv, const std::vector<BRobotStatusPtr>& data);
    void onGatewayList(SRVKEY srv, const std::vector<BRobotStatusPtr>& data);
    void onGatewayListUpdate(SRVKEY srv, const std::vector<BRobotStatusPtr>& data);
    void onAlarm2(SRVKEY srv, int level, const BString& message, BInt64 moment);


private:
    void onIndicator(BDateTime, BVariableKey, double) {}
    void onBook(SRVKEY, BIsinID, const BBookPtr&) {}

    mutable std::mutex m_mutex;
    mutable std::condition_variable m_condition;
    bool m_connected{false};

    BMonDaemonCallbacks* m_callbacks;
};
