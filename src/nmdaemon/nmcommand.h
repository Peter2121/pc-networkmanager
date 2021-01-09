#ifndef NMCOMMAND_H
#define NMCOMMAND_H

enum class nmscope
{
    NONE, DUMMY, SYSTEM, INTERFACE, WPA
};

enum class nmcmd
{
    NONE, TEST,
    IF_ADD, IF_REMOVE, IF_LIST,
    IF_ENABLE, IF_DISABLE,
    ADDR_ADD, ADDR_REMOVE, ADDR_CHANGE,
    WPA_SCAN, WPA_SETKEY, WPA_GETKEY, WPA_CONNECT, WPA_DISCONNECT
};

struct nmcommand
{
    nmscope scope;
    nmcmd cmd;
};

static nmcommand cmds[] =
{
    { nmscope::DUMMY, nmcmd::TEST },
    { nmscope::SYSTEM, nmcmd::IF_ADD },
    { nmscope::SYSTEM, nmcmd::IF_REMOVE },
    { nmscope::SYSTEM, nmcmd::IF_LIST },
    { nmscope::INTERFACE, nmcmd::IF_ENABLE },
    { nmscope::INTERFACE, nmcmd::IF_DISABLE },
    { nmscope::INTERFACE, nmcmd::ADDR_ADD },
    { nmscope::INTERFACE, nmcmd::ADDR_REMOVE },
    { nmscope::INTERFACE, nmcmd::ADDR_CHANGE },
    { nmscope::WPA, nmcmd::WPA_SCAN },
    { nmscope::WPA, nmcmd::WPA_SETKEY },
    { nmscope::WPA, nmcmd::WPA_GETKEY },
    { nmscope::WPA, nmcmd::WPA_CONNECT },
    { nmscope::WPA, nmcmd::WPA_DISCONNECT }
};

#endif // NMCOMMAND_H
