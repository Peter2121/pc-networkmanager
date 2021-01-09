#include "dummy_worker.h"

dummy_worker::dummy_worker()
{ }

dummy_worker::~dummy_worker()
{ }

nmscope dummy_worker::getScope()
{
    return nmscope::DUMMY;
}

json dummy_worker::execCmd(nmcommand_data*)
{
    return { { JSON_PARAM_RESULT, JSON_PARAM_SUCC } };
}

bool dummy_worker::isValidCmd(nmcommand_data* pcmd)
{
    if(pcmd->getCommand().scope == nmscope::DUMMY)
        return true;
    else
        return false;
}

