#ifndef NMDAEMONSPY_H
#define NMDAEMONSPY_H

#include <QObject>
#include <QSignalSpy>


class NMDaemonSpy : public QSignalSpy
{
protected:
    int count;
public:
    NMDaemonSpy(const QObject *object, const QMetaMethod &signal) : QSignalSpy(object, signal) { count = QSignalSpy::count(); }
    NMDaemonSpy(const QObject *object, const char *signal)  : QSignalSpy(object, signal)  { count = QSignalSpy::count(); }
    template <typename Func>
    NMDaemonSpy(const typename QtPrivate::FunctionPointer<Func>::Object *obj, Func signal0) : QSignalSpy(obj, signal0)  { count = QSignalSpy::count(); }
    bool wait(int timeout = 5000);
    void reset();
};

#endif // NMDAEMONSPY_H
