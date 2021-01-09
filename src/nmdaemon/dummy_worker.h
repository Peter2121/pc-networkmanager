#ifndef DUMMY_WORKER_H
#define DUMMY_WORKER_H

#include "nmworker.h"

class dummy_worker : public nmworker
{
public:
    dummy_worker();
    ~dummy_worker();
    nmscope getScope();
    json execCmd(nmcommand_data*);
    bool isValidCmd(nmcommand_data*);
};

#endif // DUMMY_WORKER_H
