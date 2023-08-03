
#ifndef BMONMATE_H
#define BMONMATE_H

#include "BMonBook.h"
#include "BMonDeal.h"

#include <Data/BCurrency.h>
#include <IO/BFiler.h>

#include <boost/date_time.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <cstdint>

//#include <datetime.h>

#include "BMonDaemonClient.h"
#include "BMonDirectClient.h"
#include "BMonReceiver.h"
#include "BMonRemoteClient.h"

namespace MonMate {
    class DaemonConnector;  
}

extern "C" { 
    struct RobotStatusRust {
        char* name;
        char* config;
        RobotStatusRust* next_robot_status;
        RobotStatusRust* prev_robot_status;
    };

    struct SensorEventRust {
        char* name;
        char* category;
        double dvalue;
        char* svalue;
        uint64_t moment;
        SensorEventRust* next_sevent;
        SensorEventRust* prev_sevent;
    };

    struct RobotOrderRust {
        char* robot;
        char* account_name;
        char* instrument_name;
        char* side;
        double size_base;
        double size_quote;
        RobotOrderRust* next_rorder;
        RobotOrderRust* prev_rorder;
    };

    struct SensorVariableRust {
        char* name;
        double value;
        SensorVariableRust* next_svar;
        SensorVariableRust* prev_svar;
    };

    struct SensorPositionRust {
        char* account;
        char* name;
        char* iclass;
        char* gateway;
        double value;
        SensorPositionRust* next_spos;
        SensorPositionRust* prev_spos;
    };

    struct RustBook {
        BBookPtr bbook;
    };

    struct RustDeal {
        BDealPtr bdeal;
    };

    struct RustVariable {
        uint64_t moment; 
        uint64_t name;
        double value;
        bool lastOne;
    };

    struct RustFormula {
        BRemoteFormulaId bformula;
    };

    struct RustCallback {
        MonMate::DaemonConnector* dmon;
        uint64_t string_buffer_size;
    };

    struct RustDataCallback {
        BMonReceiverPtr dcon;
        void (*on_book)(RustBook*);
        void (*on_variable)(RustVariable*);
        void (*on_deal)(RustDeal*);
        void (*on_formula)(RustFormula*);
    };

    RustCallback *
    create_daemon_connector_impl();

    RustDataCallback*
    create_data_connector(
        int port,
        void (*on_book)(RustBook* rbook), 
        void (*on_deal)(RustDeal* rdeal), 
        void (*on_variable)(RustVariable* rvariable), 
        void (*on_formula)(RustFormula* rformula));

    int
    connect_to_daemon(RustCallback *rc, const char *host, std::uint16_t port);

    void
    delete_daemon_connector(struct RustCallback *rc);
 
    int
    login(struct RustCallback *rc, const char *user, const char *password);

    int
    login_by_hash(struct RustCallback *rc, const char *user, const char *hash);
    
    void
    gates_status(struct RustCallback *rc, const char *server, const char *gates, RobotStatusRust *slist);

    void
    robots_status(struct RustCallback *rc, const char *server, const char *robots, RobotStatusRust *slist);

    void
    add_robot(struct RustCallback *rc, const char *server, const char *robot, const char *config);
    
    void
    start_robot(struct RustCallback *rc, const char *server, const char *robot);

    void
    start_robots(struct RustCallback *rc, const char *server, const char *robots);

    void
    stop_robot(struct RustCallback *rc, const char *server, const char *robot);

    void
    stop_robots(struct RustCallback *rc, const char *server, const char *robots);

    void
    start_gateway(struct RustCallback *rc, const char* server, const char* gateway);

    void
    start_gateways(struct RustCallback *rc, const char* server, const char* gateways);

    void
    stop_gateway(struct RustCallback *rc, const char* server, const char* gateway);

    void
    stop_gateways(struct RustCallback *rc, const char* server, const char* gateways);

    void
    gateway_config_impl(struct RustCallback *rc, const char* server, const char* gateway, char* stringBuffer);

    void
    delete_robot(struct RustCallback *rc,  const char* server, const char* robot);
       
    void   
    get_all_orders(struct RustCallback *rc, const char* server, RobotOrderRust *rorder);
       
    void
    sensors(struct RustCallback *rc, const char *server, SensorEventRust *sevent);
    void 
    variables(struct RustCallback *rc, const char* server, SensorVariableRust *svar);
    void 
    positions(struct RustCallback *rc, const char* server, SensorPositionRust *spos);
}

namespace MonMate {

enum RobotFlags {
    krfRestricted           = 1 << 0,
    krfPositionIsClosing    = 1 << 1,
    krfClearing             = 1 << 2,
    krfOutOfSchedule        = 1 << 3,
    krfCanTrade             = 1 << 4,
    krfRestrictedClosing    = 1 << 5,
};

class RobotStatus {
public:
    RobotStatus(const BRobotStatusPtr status): m_status(status)
    {}

    std::string name() const {
        return m_status->Name.toAnsi();
    }

    std::string config() const {
        return m_status->Config.toAnsi();
    }

    BRobotStatus::State status() const {
        return static_cast<BRobotStatus::State>(m_status->Status);
    }

     boost::shared_ptr<std::list<MonMate::RobotFlags>> flags() const {
        int f = m_status->Flags;
        int exponent = 0;
        auto plist = boost::make_shared<std::list<MonMate::RobotFlags>>();
        while (f > 0) {
            if (f % 2) {
                plist->push_back(static_cast<MonMate::RobotFlags>(1 << exponent));
            }
            f >>= 1;
            exponent += 1;
        }

        return plist;
    }

    std::string message() const {
        return m_status->Message.toAnsi();
    }

    std::string strategy() const {
        return m_status->Strategy.toAnsi();
    }

private:
    BRobotStatusPtr m_status;
};

class SensorEvent {
public:
    SensorEvent(BString category, BString rawName, double dVal, BString sVal, BInt32 count, double min, double max, double total, BDateTime moment)
        : m_category(category)
        , m_rawName(rawName)
        , m_dVal(dVal)
        , m_sVal(sVal)
        , m_count(count)
        , m_min(min)
        , m_max(max)
        , m_total(total)
        , m_moment(moment)
    {}

    std::string category() const {
        return m_category.toAnsi();
    }

    std::string name() const {
        return m_rawName.toAnsi();
    }

    BString rawName() const {
        return m_rawName;
    }

    double dVal() const {
        return m_dVal;
    }

    std::string sVal() const {
        return m_sVal.toAnsi();
    }

    BInt32 count() const {
        return m_count;
    }

    double min() const {
        return m_min;
    }

    double max() const {
        return m_max;
    }

    double total() const {
        return m_total;
    }

    BDateTime moment() const {
        return m_moment;
    }

    BArray<BString> splitName() {
        return m_rawName.split('|', false);
    }


private:
    BString m_category;
    BString m_rawName;
    double m_dVal;
    BString m_sVal;
    BInt32 m_count;
    double m_min;
    double m_max;
    double m_total;
    BDateTime m_moment;
};

// Experimental. API is subject to change 
class RobotOrder {
public:
    RobotOrder(BOrderSnapshotPtr order): m_order(order) {}

    std::string robot() const {
        return m_order->Robot.toAnsi();
    }

    std::string account_name() const {
        return m_order->Account->name().toAnsi();
    }

    std::string instrument_name() const {
        return m_order->Instrument->name().toAnsi();
    }

    // TODO: use python enum for order sides
    std::string side() const {
        return m_order->Side == BDefs::OrderSide::kBid ? "BID" : "ASK";
    }

    double size_base() const {
        return m_order->BaseVol;
    }

    double size_quote() const {
        return m_order->QuoteVol;
    }

//    BString           Robot;
//    BString           Strategy;
//
//    BAccountPtr       Account;
//    BInstrumentPtr    Instrument;
//
//    BUInt64           TagId       = 0;
//    BUInt64           Id          = 0;
//    BDefs::OrderSide  Side        = BDefs::OrderSide::kUnknown;
//    BInt64            InitialSize = 0;
//    BInt64            Size        = 0;
//    double            Price       = BDefs::NA;
//    BDefs::PriceType  PriceType   = BDefs::PriceType::kLimit;
//    BDefs::OrderState State       = BDefs::OrderState::kStateDeleteConfirmed;
//    double            BaseVol     = BDefs::NA;
//    double            QuoteVol    = BDefs::NA;
//
//    BDateTime         PlacedTimestamp;


private:
    BOrderSnapshotPtr m_order;
};

class SensorPosition
{
public:
    SensorPosition(BString account, BString name, BString iclass, BString gateway, double value)
        : m_account(account)
        , m_name(name)
        , m_iclass(iclass)
        , m_gateway(gateway)
        , m_value(value)
    {}

    std::string account() const {
        return m_account.toAnsi();
    }

    std::string name() const {
        return m_name.toAnsi();
    }

    std::string iclass() const {
        return m_iclass.toAnsi();
    }

    std::string gateway() const {
        return m_gateway.toAnsi();
    }

    double value() const {
        return m_value;
    }

private:
    BString m_account;
    BString m_name;
    BString m_iclass;
    BString m_gateway;
    double m_value;
};

class SensorVariable
{
public:
    SensorVariable(BString name, double value): m_name(name), m_value(value) {}

    std::string name() const {
        return m_name.toAnsi();
    }

    double value() const {
        return m_value;
    }

private:
    BString m_name;
    double m_value;

};

class DaemonConnector: public BMonDaemonCallbacks {
public:
    DaemonConnector();
    DaemonConnector(boost::shared_ptr<RustCallback> callbacks);
    virtual ~DaemonConnector() {};

    void
    connect(const std::wstring &host, const std::uint16_t port);

    // TODO: Do not use plain text passwords - use hashes in password file
    int
    login(const std::wstring &user, const std::wstring &password);

    int
    loginByHash(const std::wstring &user, const std::wstring &hash);

    void
    setRobotConfig(std::wstring server, std::wstring robot, std::wstring config);

    void
    setGatewayConfig(std::wstring server, std::wstring gateway, std::wstring config);

    boost::shared_ptr<RobotStatus>
    statusRobot(std::wstring server, std::wstring robot);
    
    boost::shared_ptr<std::list<boost::shared_ptr<RobotStatus>>>
    statusRobots(std::wstring server, std::wstring robots);

    boost::shared_ptr<RobotStatus>
    statusGateway(std::wstring server, std::wstring gateway);

    boost::shared_ptr<std::list<boost::shared_ptr<RobotStatus>>>
    statusGateways(std::wstring server, std::wstring gateways);

    std::string
    gatewayConfig(std::wstring server, std::wstring gateway);

    boost::shared_ptr<std::list<boost::shared_ptr<RobotOrder>>>
    getAllOrders(std::wstring server);

    boost::shared_ptr<std::list<boost::shared_ptr<SensorEvent>>>
    sensors(std::wstring server);

    boost::shared_ptr<std::list<boost::shared_ptr<SensorVariable>>>
    variables(std::wstring server);

    boost::shared_ptr<std::list<boost::shared_ptr<SensorPosition>>>
    positions(std::wstring server);

    void
    addRobot(std::wstring server, std::wstring robot, std::wstring config);

    void
    addRobot(std::wstring server, std::wstring robot, std::wstring config, bool overwrite);

    void
    startRobot(std::wstring server, std::wstring robot);

    void
    startRobots(std::wstring server, std::wstring robots);

    void
    stopRobot(std::wstring server, std::wstring robot);

    void
    stopRobots(std::wstring server, std::wstring robots);

    void
    startGateway(std::wstring server, std::wstring gateway);

    void
    startGateways(std::wstring server, std::wstring gateways);

    void
    stopGateway(std::wstring server, std::wstring gateway);

    void
    stopGateways(std::wstring server, std::wstring gateways);

    void
    deleteRobot(std::wstring server, std::wstring robot);

    void onSensors(SRVKEY srv, const BArray<BUInt8>& data);
    void onAlarm2(SRVKEY srv, int level, const BString& message, BInt64 moment);
private:
    BString m_user;
    BMonDaemonClientPtr m_session;
    boost::shared_ptr<RustCallback> m_pyCallbacks;
    bool m_hasOnSensors;
    bool m_hasOnAlarm;
};
}
#endif
