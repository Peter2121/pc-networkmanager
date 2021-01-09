#ifndef SYS_WORKER_H
#define SYS_WORKER_H

#include "nmworker.h"

class system_worker : public nmworker
{
protected:
    static constexpr nmcommand Cmds[] =
    {
        { nmscope::SYSTEM, nmcmd::IF_ADD },
        { nmscope::SYSTEM, nmcmd::IF_REMOVE },
        { nmscope::SYSTEM, nmcmd::IF_LIST }
    };
public:
    system_worker();
    ~system_worker();
    nmscope getScope();
    json execCmd(nmcommand_data*);
    bool isValidCmd(nmcommand_data*);
    json execCmdIfList(nmcommand_data*);
};

#endif // SYS_WORKER_H
