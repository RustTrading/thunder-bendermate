#include "BMonRemoteClient.h"
#include "Data/BInstrument.h"
#include "BError.h"

B_IMPLEMENT(BMonRemoteClient);

BMonRemoteClientPtr BMonRemoteClient::createObject(
    const BEndpointPtr& endpoint,
    const BEndpointPtr& receiverEndpoint)
{
    auto res = BMonRemoteClient::cast(BClientSession::createObject(BDefs::kProtoBinary, endpoint, BMonRemoteClient::type(), {}, 5000));

    res->setReconnectDelay(1000);
    res->m_receiverEndpoint = receiverEndpoint;

    return res;
}

void BMonRemoteClient::bindToVariable(BUInt64 name)
{
    {
        std::lock_guard<std::mutex> lock(m_bindingsMutex);
        if (!m_variableBindings.inc(name))
            return;
    }

    try
    {
        bind(name);
    }
    catch(...)
    {
    }
}

void BMonRemoteClient::unbindVariable(BUInt64 name)
{
    {
        std::lock_guard<std::mutex> lock(m_bindingsMutex);
        if (!m_variableBindings.dec(name))
            return;
    }

    try
    {
        unbind(name);
    }
    catch(...)
    {
    }
}

void BMonRemoteClient::bindToBookEx(const BString& name, const BString& className, const BString& exchangeName)
{
    auto isin = BInstrument::createObject(name, className, exchangeName);

    {
        std::lock_guard<std::mutex> lock(m_bindingsMutex);
        if (!m_bookBindings.inc(isin->id()))
            return;
    }

    try
    {
        bindToBook(isin->id(), name, className, exchangeName);
    }
    catch(...)
    {
    }
}

void BMonRemoteClient::unbindBookEx(const BString& name, const BString& className, const BString& exchangeName)
{
    auto isin = BInstrument::createObject(name, className, exchangeName);

    {
        std::lock_guard<std::mutex> lock(m_bindingsMutex);
        if (!m_bookBindings.dec(isin->id()))
            return;
    }

    try
    {
        unbindBook(name, className, exchangeName);
    }
    catch(...)
    {
    }
}


void BMonRemoteClient::bindToDealEx(const BString& name, const BString& className, const BString& exchangeName)
{
    auto isin = BInstrument::createObject(name, className, exchangeName);

    {
        std::lock_guard<std::mutex> lock(m_bindingsMutex);
        if (!m_dealBindings.inc(isin->id()))
            return;
    }

    try
    {
        bindToDeal(isin->id(), name, className, exchangeName);
    }
    catch(...)
    {
    }
}

void BMonRemoteClient::unbindDealEx(const BString& name, const BString& className, const BString& exchangeName)
{
    auto isin = BInstrument::createObject(name, className, exchangeName);

    {
        std::lock_guard<std::mutex> lock(m_bindingsMutex);
        if (!m_dealBindings.dec(isin->id()))
            return;
    }

    try
    {
        unbindDeal(name, className, exchangeName);
    }
    catch(...)
    {
    }
}

void BMonRemoteClient::onConnected()
{
    std::cout << "BMonRemoteClient::onConnected()" << std::endl;
    try
    {
        initialize(/*m_receiverEndpoint->Host*/L"", m_receiverEndpoint->Port, encodeServerID(L"RZVT"), true);
//        initialize(/*m_receiverEndpoint->Host*/L"", m_receiverEndpoint->Port, encodeServerID(L"U021"), true);
//        initialize(/*m_receiverEndpoint->Host*/L"", m_receiverEndpoint->Port, encodeServerID(L"no-server"), true);

        std::cout << "BMonRemoteClient::onConnected() --> initialize" << std::endl;

        BRobotBindingsMap<BUInt64> variableBindings;
        BRobotBindingsMap<BIsinID> bookBindings;

        {
            std::lock_guard<std::mutex> lock(m_bindingsMutex);
            variableBindings = m_variableBindings;
            bookBindings = m_bookBindings;
        }

        for (auto it = variableBindings.begin(); it != variableBindings.end(); ++it)
            bind(it->first);

        for (auto it = bookBindings.begin(); it != bookBindings.end(); ++it)
        {
            auto isin = BInstrument::createObject(it->first);
            bindToBook(it->first, isin->name(), isin->className(), isin->gatewayName());
        }
    }
    catch(...)
    {
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = true;
    m_condition.notify_one();
    std::cout << "BMonRemoteClient::onConnected() --> initialize done" << std::endl;

}


bool BMonRemoteClient::waitForConnect(BInt64 ms)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_connected)
        return true;
    m_condition.wait_for(lock, std::chrono::milliseconds(ms));
    return m_connected;
}