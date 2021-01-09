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
    return { { JSON_PARAM_RESULT, JSON_PARAM_SUCC }, { JSON_PARAM_DATA, {} } };
}

