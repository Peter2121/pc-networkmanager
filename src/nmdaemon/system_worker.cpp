#include "system_worker.h"

system_worker::system_worker()
{ }

system_worker::~system_worker()
{ }

nmscope system_worker::getScope()
{
    return nmscope::SYSTEM;
}

json system_worker::execCmd(nmcommand_data* pcmd)
{
    switch (pcmd->getCommand().cmd)
    {
        case nmcmd::IF_LIST :
            return execCmdIfList(pcmd);
        case nmcmd::IF_ADD :
        case nmcmd::IF_REMOVE :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_NOT_IMPLEMENTED} };
        default :
            return { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, {JSON_PARAM_ERR, JSON_DATA_ERR_INVALID_COMMAND} };
    }
}

bool system_worker::isValidCmd(nmcommand_data* pcmd)
{
    if( pcmd->getCommand().scope != getScope() )
        return false;
    int cmd_size = sizeof(Cmds) / sizeof(Cmds[0]);
    for(int i=0; i<cmd_size; i++)
    {
        if(pcmd->getCommand().cmd == Cmds[i].cmd)
            return true;
    }
    return false;
}

json system_worker::execCmdIfList(nmcommand_data*)
{
    struct ifaddrs * ifaddrs_ptr;
    nlohmann::json retIfListJson;
    std::vector<nlohmann::json> vectIfsJson;
    int status;
    std::map<std::string, interface> ifMap;

    status = getifaddrs (& ifaddrs_ptr);
    if (status == -1) {
        LOG_S(ERROR) << "Error in getifaddrs: " << errno << " (" << strerror (errno) << ")";
    }

    while (ifaddrs_ptr) {
        ifMap[ifaddrs_ptr->ifa_name].setName(std::string(ifaddrs_ptr->ifa_name));
        ifMap[ifaddrs_ptr->ifa_name].addAddress(ifaddrs_ptr);
        ifaddrs_ptr = ifaddrs_ptr->ifa_next;
    }

    freeifaddrs (ifaddrs_ptr);

    for (auto iface=ifMap.begin(); iface!=ifMap.end(); ++iface)
    {
      vectIfsJson.push_back(iface->second.getIfJson());
    }

    nlohmann::json addrJson;

    retIfListJson[JSON_PARAM_INTERFACES] = vectIfsJson;

    return retIfListJson;
}

