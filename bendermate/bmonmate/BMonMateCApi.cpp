#include "BMonMate.h"
#include <boost/shared_ptr.hpp>
#include <string>
#include <iostream>  
#include <stdio.h>

extern "C" {
    int run_script(int argc, char **argv);
    int run_file_script(int argc, char **argv) {
        run_script(argc, argv);
    }
}

char* malloc_cstr(const std::string& str) {
    char* ptr = (char*) malloc(str.size() + 1);
    ptr[str.size()] = '\0';
    str.copy(ptr, str.size());
    return ptr;
}

extern "C" {
void
setRobotStatusData(RobotStatusRust *slist, const MonMate::RobotStatus& rstatus) {
    slist->name = malloc_cstr(rstatus.name());
    slist->config = malloc_cstr(rstatus.config());
}
 
void
setSensorEventData(SensorEventRust *slist, const MonMate::SensorEvent& sevent) {
    slist->name = malloc_cstr(sevent.name());
    slist->category = malloc_cstr(sevent.category());
    slist->svalue = malloc_cstr(sevent.sVal());
    slist->dvalue = sevent.dVal();
    slist->moment = sevent.moment().toMicrosecondsSinceEpoch();
}

void
setRobotOrder(RobotOrderRust *rlist, const MonMate::RobotOrder& rorder) {
    rlist->robot = malloc_cstr(rorder.robot());
    rlist->account_name = malloc_cstr(rorder.account_name());
    rlist->instrument_name = malloc_cstr(rorder.instrument_name());
    rlist->side = malloc_cstr(rorder.side());
    rlist->size_base = rorder.size_base();
    rlist->size_quote = rorder.size_quote();
}

void
setSensorVariable(SensorVariableRust *svlist, const MonMate::SensorVariable& svar) {
    svlist->name = malloc_cstr(svar.name());
    svlist->value = svar.value();
}

void
setSensorPosition(SensorPositionRust *plist, const MonMate::SensorPosition& spos) {
    plist->account = malloc_cstr(spos.account());
    plist->name = malloc_cstr(spos.name());
    plist->iclass = malloc_cstr(spos.iclass());
    plist->gateway = malloc_cstr(spos.gateway());
    plist->value = spos.value();
}
}

std::wstring
string_to_wide_string(const char *string){
    wchar_t warr[strlen(string)];
    std::mbstowcs(&warr[0], string, strlen(string) + 1);
    return std::wstring(warr);
}

class BMonDataCallbacksRust : public BMonDataCallbacks
{
public:
   BMonDataCallbacksRust(
        void (*on_book)(RustBook* rbook), 
        void (*on_deal)(RustDeal* rdeal), 
        void (*on_variable)(RustVariable* rvariable),
        void (*on_formula)(RustFormula* rformula)) : BMonDataCallbacks(), 
        m_on_book(on_book),
        m_on_deal(on_deal),
        m_on_variable(on_variable),
        m_on_formula(on_formula) {          
        }
    virtual ~BMonDataCallbacksRust(){};
    
    void onVariable(BDateTime moment, BUInt64 name, double value, bool lastOne){
    
    };
    void onBook(BIsinID instrumentId, const BBookPtr& book){

    };
    void onDeal(const BDealPtr& deal){

    };
    void onFormula(BRemoteFormulaId id, double value){

    };
private:
    void (*m_on_book)(RustBook* rbook);
    void (*m_on_deal)(RustDeal* rdeal);
    void (*m_on_variable)(RustVariable* rvariable);
    void (*m_on_formula)(RustFormula* rformula);
};

extern "C" {
    RustCallback *
    create_daemon_connector_impl() {
        RustCallback* prc =  new RustCallback;
        prc->dmon = new MonMate::DaemonConnector(boost::shared_ptr<RustCallback>(prc));
        return prc;
    }

    RustDataCallback*
    create_data_connector(
        int port,
        void (*on_book)(RustBook* rbook), 
        void (*on_deal)(RustDeal* rdeal), 
        void (*on_variable)(RustVariable* rvariable), 
        void (*on_formula)(RustFormula* rformula)) {
            auto prc = new RustDataCallback;
            auto bmondatacallback = new BMonDataCallbacksRust(
                on_book, 
                on_deal, 
                on_variable,
                on_formula
                );
            prc->dcon = BMonReceiver::createObject(port, bmondatacallback);
        return prc;
    }

    int
    connect_to_daemon(RustCallback *rc, const char *host, std::uint16_t port) {
        const std::wstring &h = string_to_wide_string(host);
        try {
            rc->dmon->connect(h, port);
        } catch (...) {
            return -1;
        }
        return 0;
    }

    void
    delete_daemon_connector(struct RustCallback *rc) {
        if (rc)
            delete rc->dmon;
    }

    int
    login(struct RustCallback *rc, const char *user, const char *password) {
        const std::wstring &wuser = string_to_wide_string(user);
        const std::wstring &wpassword = string_to_wide_string(password);
        return rc->dmon->login(wuser, wpassword);
    }
    
    int
    login_by_hash(struct RustCallback *rc, const char *user, const char *hash) {
        const std::wstring &wuser = string_to_wide_string(user);
        const std::wstring &whash = string_to_wide_string(hash);
        return rc->dmon->loginByHash(wuser, whash);
    }
    
    void
    gates_status(struct RustCallback *rc, const char *server, const char *gates, RobotStatusRust *slist) {
        const std::wstring &wserver = string_to_wide_string(server);
        const std::wstring &wgways = string_to_wide_string(gates);
        boost::shared_ptr<std::list<boost::shared_ptr<MonMate::RobotStatus>>> st = rc->dmon->statusGateways(wserver, wgways);
        auto itst = st->begin();
        RobotStatusRust* head = slist;
        RobotStatusRust* prev_head = nullptr;
        while (itst !=st->end()) {
            setRobotStatusData(head, **itst);
            itst++;
            if (itst != st->end()) {
                prev_head =  head;
                head->next_robot_status = new RobotStatusRust{nullptr, nullptr, nullptr, nullptr};
                head = head->next_robot_status;
                head->prev_robot_status = prev_head;
            } else {
                head->next_robot_status = nullptr;
            }
        }
    }
  
    void
    robots_status(struct RustCallback *rc, const char *server, const char *robots, RobotStatusRust *slist) {
        const std::wstring& wserver = string_to_wide_string(server);
        const std::wstring& wrobots = string_to_wide_string(robots);
        boost::shared_ptr<std::list<boost::shared_ptr<MonMate::RobotStatus>>> st = rc->dmon->statusRobots(wserver, wrobots);
        auto itst = st->begin();
        RobotStatusRust *head = slist;
        RobotStatusRust *prev_head = nullptr;
        while (itst != st->end()) {
            setRobotStatusData(head, **itst);
            itst++;
            if (itst != st->end()) {
                prev_head =  head;
                head->next_robot_status = new RobotStatusRust{nullptr, nullptr, nullptr, nullptr};
                head = head->next_robot_status;
                head->prev_robot_status = prev_head;
            } else {
                head->next_robot_status = nullptr;
            }
        }
    }

    void
    add_robot(struct RustCallback *rc, const char *server, const char *robot, const char *config) {
        const std::wstring &wserver = string_to_wide_string(server);
        const std::wstring &wrobot = string_to_wide_string(robot);
        const std::wstring &wconfig = string_to_wide_string(config);
        rc->dmon->addRobot(wserver, wrobot, wconfig);
    }
    
    void
    start_robot(struct RustCallback *rc, const char *server, const char *robot) {
        const std::wstring &wserver = string_to_wide_string(server); 
        const std::wstring &wrobot  = string_to_wide_string(robot);
        rc->dmon->startRobot(wserver, wrobot);
    }

    void
    start_robots(struct RustCallback *rc, const char *server, const char *robots) {
        const std::wstring &wserver = string_to_wide_string(server); 
        const std::wstring &wrobots  = string_to_wide_string(robots);
        rc->dmon->startRobot(wserver, wrobots);
    }

    void
    stop_robot(struct RustCallback *rc, const char *server, const char *robot) {
        const std::wstring &wserver = string_to_wide_string(server); 
        const std::wstring &wrobot = string_to_wide_string(robot);
        rc->dmon->stopRobot(wserver, wrobot);
    }

    void
    stop_robots(struct RustCallback *rc, const char *server, const char *robots) {
        const std::wstring &wserver = string_to_wide_string(server); 
        const std::wstring &wrobots = string_to_wide_string(robots);
        rc->dmon->stopRobots(wserver, wrobots);
    }

    void
    start_gateway(struct RustCallback *rc, const char* server, const char* gateway) {
        const std::wstring &wserver = string_to_wide_string(server); 
        const std::wstring &wgateway = string_to_wide_string(gateway);
        rc->dmon->startGateway(wserver, wgateway);
    }

    void
    start_gateways(struct RustCallback *rc, const char* server, const char* gateways) {
        const std::wstring &wserver = string_to_wide_string(server); 
        const std::wstring &wgateways = string_to_wide_string(gateways);
        rc->dmon->startGateways(wserver, wgateways);
    }

    void
    stop_gateway(struct RustCallback *rc, const char* server, const char* gateway) {
        const std::wstring &wserver = string_to_wide_string(server); 
        const std::wstring &wgateway = string_to_wide_string(gateway);
        rc->dmon->stopGateway(wserver, wgateway);
    }

    void
    stop_gateways(struct RustCallback *rc, const char* server, const char* gateways) {
        const std::wstring &wserver = string_to_wide_string(server); 
        const std::wstring &wgateways  = string_to_wide_string(gateways);
        rc->dmon->stopGateways(wserver, wgateways);
    }
  
    void
    gateway_config_impl(struct RustCallback *rc, const char* server, const char* gateway, char* stringBuffer) {
        const std::wstring &wserver = string_to_wide_string(server); 
        const std::wstring &wgateway  = string_to_wide_string(gateway);
        std::string configString = rc->dmon->gatewayConfig(wserver, wgateway);
        if (rc->string_buffer_size > configString.length()) {
            configString.copy(stringBuffer, configString.length());
        }
    }

    void
    delete_robot(struct RustCallback *rc,  const char* server, const char* robot) {
        const std::wstring &wserver = string_to_wide_string(server); 
        const std::wstring &wrobot = string_to_wide_string(robot);
        rc->dmon->deleteRobot(wserver, wrobot);
    }
    
    void   
    get_all_orders(struct RustCallback *rc, const char* server, RobotOrderRust *rorder) {
        const std::wstring &wserver = string_to_wide_string(server);
        boost::shared_ptr<std::list<boost::shared_ptr<MonMate::RobotOrder>>> orders = rc->dmon->getAllOrders(wserver);
        auto itorder = orders->begin();
        RobotOrderRust *head = rorder;
        RobotOrderRust *prev_head = nullptr;
        while (itorder != orders->end()) {
          setRobotOrder(head, **itorder);
          itorder++;
          if (itorder != orders->end()) {
                prev_head =  head;
                head->next_rorder = new RobotOrderRust{nullptr, nullptr, nullptr, nullptr, -1, -1};
                head = head->next_rorder;
                head->prev_rorder = prev_head;
            } else {
                head->next_rorder = nullptr;
            }
        }
    }

    void
    sensors(struct RustCallback *rc, const char *server, SensorEventRust *sevent) {
        const std::wstring& wserver = string_to_wide_string(server);
        boost::shared_ptr<std::list<boost::shared_ptr<MonMate::SensorEvent>>>  sev = rc->dmon->sensors(wserver);
        auto itsev = sev->begin();
        SensorEventRust *head = sevent;
        SensorEventRust *prev_head = nullptr;
        while (itsev != sev->end()) {
            setSensorEventData(head, **itsev); 
            itsev++;
            if (itsev != sev->end()) {
                prev_head =  head;
                head->next_sevent = new SensorEventRust{nullptr, nullptr, -1, nullptr, 0, nullptr, nullptr};
                head = head->next_sevent;
                head->prev_sevent = prev_head;
            } else {
                head->next_sevent = nullptr;
            }
        }
    }

    void variables(struct RustCallback *rc, const char* server, SensorVariableRust *svar) {
        const std::wstring& wserver = string_to_wide_string(server);
        boost::shared_ptr<std::list<boost::shared_ptr<MonMate::SensorVariable>>>  sv = rc->dmon->variables(wserver);
        auto itsvar = sv->begin();
        SensorVariableRust *head = svar;
        SensorVariableRust *prev_head = nullptr;
        while (itsvar != sv->end()) {
            setSensorVariable(head, **itsvar);
            itsvar++;
            if (itsvar != sv->end()) {
                prev_head =  head;
                head->next_svar = new SensorVariableRust{nullptr, -1, nullptr, nullptr};
                head = head->next_svar;
                head->prev_svar = prev_head;
            } else {
                head->next_svar = nullptr;
            }
        }
    }

    void positions(struct RustCallback *rc, const char* server, SensorPositionRust *spos) {
        const std::wstring& wserver = string_to_wide_string(server);
        boost::shared_ptr<std::list<boost::shared_ptr<MonMate::SensorPosition>>>  sp = rc->dmon->positions(wserver);
        auto itspos = sp->begin();
        SensorPositionRust *head = spos;
        SensorPositionRust *prev_head = nullptr;
        while (itspos != sp->end()) {
            setSensorPosition(head, **itspos);
            itspos++;
            if (itspos != sp->end()) {
                prev_head =  head;
                head->next_spos = new SensorPositionRust{nullptr, nullptr, nullptr, nullptr, -1, nullptr, nullptr};
                head = head->next_spos;
                head->prev_spos = prev_head;
            } else {
                head->next_spos = nullptr;
            }
        }
    }
}
