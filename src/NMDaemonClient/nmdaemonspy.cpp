#include "nmdaemonspy.h"

bool NMDaemonSpy::wait(int timeout)
{
    if(QSignalSpy::count() > count)
        return true;
    return QSignalSpy::wait(timeout);
}

void NMDaemonSpy::reset()
{
    count = QSignalSpy::count();
}
