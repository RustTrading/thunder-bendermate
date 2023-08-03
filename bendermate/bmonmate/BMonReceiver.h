
#pragma once

#include "BObject.h"
#include "Net/BUdpMarketDataParser.h"
#include <boost/asio.hpp>
#include "BString.h"
#include "BDateTime.h"
#include "Data/BBook.h"

#include <thread>

class BMonDataCallbacks
{
public:
    virtual void onVariable(BDateTime moment, BUInt64 name, double value, bool lastOne) = 0;
    virtual void onBook(BIsinID instrumentId, const BBookPtr& book) = 0;
    virtual void onDeal(const BDealPtr& deal) = 0;
    virtual void onFormula(BRemoteFormulaId id, double value) = 0;
    virtual ~BMonDataCallbacks(){}
};

B_CLASS(BMonReceiver, BObject)
    , public IUdpMarketDataCallbacks
{
    B_DECLARE(BMonReceiver)

public:
    B_STATIC BMonReceiverPtr createObject(int port, BMonDataCallbacks* callbacks);
    B_STATIC BMonReceiverPtr createOnUnusedPort(BMonDataCallbacks* callbacks);

    BMonReceiver();
    virtual ~BMonReceiver();

    // IUdpMarketDataCallbacks
    void newRemoteVariable(BDateTime moment, const BVariableKey& key, double value, bool lastOne) override;
    void newRemoteFormulaValue(BRemoteFormulaId id, double value)                                 override;
    void newRemoteFormulaError(BRemoteFormulaId id, const BString& message)                       override;
    void newRemoteBook(SRVKEY serverID, BIsinID instrumentId, const BBookPtr& book)               override;
    void newRemoteDeal(SRVKEY serverID, const BDealPtr& deal)                                     override;
    void resetRemoteMarketData(SRVKEY serverID)                                                   override;

    BUInt32 port();

private:
    void run();
    void parse(size_t length, unsigned char* data);

    BUdpMarketDataParser   m_udpParser;

    boost::asio::io_service m_service;
    std::shared_ptr<boost::asio::ip::udp::socket> m_socket;
    std::shared_ptr<std::thread> m_thread;

    enum { max_length = 2048 };
    unsigned char m_data[max_length];

    std::shared_ptr<BMonDataCallbacks> m_callbacks;
};
