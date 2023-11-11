#ifndef ASYNSTEP_HPP
#define ASYNSTEP_HPP


#include <functional>
#include <any>

#include <async++.h>

class AsynStepException : public std::exception
{
protected:
    std::string Info;

public:
    AsynStepException(std::string info) : Info(info) {}
    const char *what()
    {
        return Info.c_str();
    }
};

class AsynWfException : public std::exception
{
protected:
    std::string Info;

public:
    AsynWfException(std::string info) : Info(info) {}
    const char *what()
    {
        return Info.c_str();
    }
};

class AsynStepBase
{
protected:
    std::string Name;
public:
    std::string GetName() { return Name; }
    virtual bool Run() = 0;
    virtual bool Run(std::function<void()> waiter, long wait_period_ms) = 0;
    virtual ~AsynStepBase() {}
};

template <class T>
class AsynStep : public AsynStepBase
{
public:
    explicit AsynStep(std::function<T()> worker, std::function<bool(T)> validator, std::string name, std::function<void()> waiter=nullptr, long wait_period_ms=0L) :
        Worker(std::move(worker)), Validator(std::move(validator)), Waiter(std::move(waiter)), WaitPeriodMs(wait_period_ms)
    { Name = name; }
    ~AsynStep() override
    {
        Worker = nullptr;
        Validator = nullptr;
        Waiter = nullptr;
    }
    bool Run(std::function<void()> waiter, long wait_period_ms) override
    {
        auto task_worker = async::spawn(Worker);
        //auto task_validator = task_worker.then(Validator);
        /*
        auto task_validator = task_worker.then(
            [&](async::task<T> parent)
            {
                auto result = parent.get();
                Data = result;
                return Validator(result);
            });
        */
        while (!task_worker.ready())
        {
            if(waiter!=nullptr)
                waiter();
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_period_ms));
        }
        Data = task_worker.get();
        auto task_validator = async::spawn(
            [&]()
            {
                return Validator(Data);
            });
        while (!task_validator.ready())
        {
            if(waiter!=nullptr)
                waiter();
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_period_ms));
        }
        return task_validator.get();
    }
    bool Run() override
    {
        return Run(Waiter, WaitPeriodMs);
    }
    T GetData() { return Data; }
protected:
    std::function<T()> Worker;
    std::function<bool(T)> Validator;
    std::function<void()> Waiter;
    long WaitPeriodMs;
    T Data;
};

class AsynWf
{
public:
    AsynWf() {}
    ~AsynWf()
    {
        Steps.clear();
    }
    void AddStep(std::shared_ptr<AsynStepBase> step)
    {
        Steps.push_back(step);
    }
    void Run()
    {
        for (auto &step : Steps)
        {
            try
            {
                if(!step->Run())
                {
                    throw AsynWfException("Step " + step->GetName() + " validation failed");
                }
            }
            catch (AsynStepException se)
            {
                throw AsynWfException("Step " + step->GetName() + " exception: " + std::string(se.what()));
            }
        }
    }

protected:
    std::vector<std::shared_ptr<AsynStepBase>> Steps;
};

#endif // ASYNSTEP_HPP
