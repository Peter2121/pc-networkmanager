#include "nmcommand_data.h"

nmcommand_data::nmcommand_data(std::string str_data)
{
    json_data = json::parse(str_data, nullptr, false);
    if(!json_data.is_discarded())
    {
        if (json_data.contains(JSON_PARAM_SCOPE))
        {
            std::string strScope = json_data[JSON_PARAM_SCOPE];
            auto enumscope = magic_enum::enum_cast<nmscope>(strScope);
            if (enumscope.has_value())
                command.scope = enumscope.value();
            else
                command.scope = nmscope::NONE;
        }
        else
            command.scope = nmscope::NONE;
        if (json_data.contains(JSON_PARAM_CMD))
        {
            std::string strCmd = json_data[JSON_PARAM_CMD];
            auto enumcmd = magic_enum::enum_cast<nmcmd>(strCmd);
            if (enumcmd.has_value())
                command.cmd = enumcmd.value();
            else
                command.cmd = nmcmd::NONE;
        }
        else
            command.cmd = nmcmd::NONE;
    }
    else
    {
        command.scope = nmscope::NONE;
        command.cmd = nmcmd::NONE;
    }
}

bool nmcommand_data::isValid()
{
    if(json_data.is_discarded())
        return false;
    if(command.scope == nmscope::NONE)
        return false;
    if(command.cmd == nmcmd::NONE)
        return false;

    for(auto nmcmd : cmds)
    {
        if ( (nmcmd.scope == command.scope) && (nmcmd.cmd == command.cmd) )
            return true;
    }

    return true;
}

json nmcommand_data::getJsonData()
{
    return json_data;
}

nmcommand nmcommand_data::getCommand()
{
    return command;
}
