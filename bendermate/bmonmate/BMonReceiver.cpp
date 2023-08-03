
#include <iostream>
#include "BMonReceiver.h"

B_IMPLEMENT(BMonReceiver);

BMonReceiverPtr BMonReceiver::createOnUnusedPort(BMonDataCallbacks* callbacks)
{
    auto port = 3099;

    BMonReceiverPtr res;
    for (int i = 0; i < 100; i++, port++)
    {
        try
        {
            res = BMonReceiver::createObject(port, callbacks);
            break;
        }
        catch (BException& e)
        {
            continue;
        }
        catch (...)
        {
            continue;
        }
    }

    return res;
}


BMonReceiverPtr BMonReceiver::createObject(int port, BMonDataCallbacks* callbacks)
{
    BMonReceiverPtr res = BMonReceiver::createObject();

    res->m_callbacks.reset(callbacks);
    res->m_socket.reset(new boost::asio::ip::udp::socket(res->m_service, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port)));
    res->m_thread.reset(new std::thread(std::bind(&BMonReceiver::run, res.get())));

    return res;
}

BMonReceiver::BMonReceiver()
    : m_udpParser(0, this, true)
{
}

BMonReceiver::~BMonReceiver()
{
    std::cout
            << "BMonReceiver::~BMonReceiver" << " "
            << std::endl;

    if (m_socket)
        m_socket->close();

    // TODO: Fix hanging here. At least it was hanging before the callbacks introduction
    if (m_thread && m_thread->joinable())
        m_thread->join();
}


BUInt32 BMonReceiver::port()
{
    return m_socket->local_endpoint().port();
}



void BMonReceiver::run()
{
    std::cout << "BMonReceiver::run" << std::endl;

    boost::system::error_code er;

    while (true)
    {
        size_t length = m_socket->receive(boost::asio::buffer(m_data, max_length), 0, er);

        if (er != 0)
            break;

        if (length > 0)
            m_udpParser.parse(m_data, length);
    }
}

void BMonReceiver::newRemoteVariable(BDateTime moment, const BVariableKey& key, double value, bool lastOne)
{
    std::cout << "BMonReceiver::newRemoteVariable()" << std::endl;
    m_callbacks->onVariable(moment, key, value, lastOne);
}

void BMonReceiver::newRemoteBook(SRVKEY /*serverID*/, BIsinID instrumentId, const BBookPtr& book)
{
//    std::cout << "BMonReceiver::newRemoteBook" << std::endl;
    m_callbacks->onBook(instrumentId, book);
}

void BMonReceiver::newRemoteDeal(SRVKEY /*serverID*/, const BDealPtr& deal)
{
//    std::cout << "BMonReceiver::newRemoteDeal" << std::endl;
    m_callbacks->onDeal(deal);
}

void BMonReceiver::newRemoteFormulaValue(BRemoteFormulaId /*id*/, double /*value*/)
{
    std::cout << "BMonReceiver::newRemoteFormulaValue" << std::endl;
}

void BMonReceiver::newRemoteFormulaError(BRemoteFormulaId /*id*/, const BString& /*message*/)
{
    std::cout << "BMonReceiver::newRemoteFormulaError" << std::endl;
}

void BMonReceiver::resetRemoteMarketData(SRVKEY /*serverID*/)
{
}

