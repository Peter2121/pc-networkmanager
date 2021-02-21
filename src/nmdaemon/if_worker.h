#ifndef IF_WORKER_H
#define IF_WORKER_H

#include <ifaddrs.h>
#include <sys/sockio.h>
#define LOGURU_WITH_STREAMS 1
#include "loguru.hpp"
#include "sockpp/socket.h"
#include "sockpp/version.h"
#include "nmworker.h"
#include "interface.h"

class if_worker : public nmworker
{
protected:
    static constexpr nmcommand Cmds[] =
    {
        { nmscope::SYSTEM, nmcmd::IP_ADDR_SET },
        { nmscope::SYSTEM, nmcmd::IP4_DHCP_ENABLE },
        { nmscope::SYSTEM, nmcmd::IP6_DHCP_ENABLE },
        { nmscope::SYSTEM, nmcmd::IP4_ALIAS_ADD },
        { nmscope::SYSTEM, nmcmd::IP6_ALIAS_ADD },
        { nmscope::SYSTEM, nmcmd::IP4_ALIAS_REMOVE },
        { nmscope::SYSTEM, nmcmd::IP6_ALIAS_REMOVE },
        { nmscope::SYSTEM, nmcmd::MTU_GET },
        { nmscope::SYSTEM, nmcmd::MTU_SET },
        { nmscope::SYSTEM, nmcmd::MAC_ADDR_GET },
        { nmscope::SYSTEM, nmcmd::MAC_ADDR_SET }
//        { nmscope::SYSTEM, nmcmd::IF_ENABLE },
//        { nmscope::SYSTEM, nmcmd::IF_DISABLE },
    };
//    int getIfFlags(std::string);
//    bool setIfFlags(std::string, int);
    std::string ifName = "";
    address_base* getMainIfAddr(short family);
    bool removeIfAddr(address_base*);
    bool addIfAddr(addr*);
public:
    if_worker();
    ~if_worker();
    nmscope getScope();
    json execCmd(nmcommand_data*);
    bool isValidCmd(nmcommand_data*);
    json execCmdIpAddrSet(nmcommand_data*);
    json execCmdIpAliasAdd(nmcommand_data*);
    json execCmdIpAliasRemove(nmcommand_data*);
//    json execCmdIfEnable(nmcommand_data*);
//    json execCmdIfDisable(nmcommand_data*);
};

#endif // IF_WORKER_H
