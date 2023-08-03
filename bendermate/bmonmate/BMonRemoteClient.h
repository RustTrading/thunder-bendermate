
#pragma once

#include "Robots/BRobotBindings.h"
#include "Net/BClientSession.h"
#include "Net/BRemoteCoreService.h"
#include <vector>
#include <thread>
#include <mutex>

#include <boost/thread/shared_mutex.hpp>

B_CLASS(BMonRemoteClient, BClientSession)
{
    B_DECLARE(BMonRemoteClient)

#include SERVICE_TAKE
    DECLARE_REMOTE_CORE_SERVICE_METHODS

public:
    enum
    {
        WAIT_INTERVAL_CONNECT_MS = 10000,
        WAIT_INTERVAL_FIGMENT_MS = 7200000
    };

    B_STATIC BMonRemoteClientPtr createObject(
        const BEndpointPtr& endpoint,
        const BEndpointPtr& receiverEndpoint);

    void bindToVariable(BUInt64 name);
    void unbindVariable(BUInt64 name);
    void bindToBookEx(const BString& name, const BString& className, const BString& exchangeName);
    void unbindBookEx(const BString& name, const BString& className, const BString& exchangeName);
    void bindToDealEx(const BString& name, const BString& className, const BString& exchangeName);
    void unbindDealEx(const BString& name, const BString& className, const BString& exchangeName);

    bool waitForConnect(BInt64 ms);

protected:
    virtual void onConnected() override;
    virtual void onDisconnected(const boost::system::error_code&) override {}

private:
    virtual void callback(const BString& /*name*/, const BServiceParams& /*params*/, BServiceResult& /*result*/) {}

private:
    BEndpointPtr m_receiverEndpoint;
    std::mutex   m_bindingsMutex;
    BRobotBindingsMap<BUInt64> m_variableBindings;
    BRobotBindingsMap<BIsinID> m_bookBindings;
    BRobotBindingsMap<BIsinID> m_dealBindings;

    mutable std::mutex m_mutex;
    mutable std::condition_variable m_condition;
    bool m_connected{false};
};

