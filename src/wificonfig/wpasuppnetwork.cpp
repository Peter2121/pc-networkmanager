#include "wpasuppnetwork.h"

WpaSuppNetwork::WpaSuppNetwork() : WpaSuppNetwork("", 0, false, false, false)
{
}

WpaSuppNetwork::WpaSuppNetwork(QString ssid, int priority, bool enabled, bool current, bool protectedd) :
    Ssid(ssid), Priority(priority), Enabled(enabled), Current(current), Protected(protectedd)
{
}

void WpaSuppNetwork::IncreasePriority()
{
    Priority++;
}

void WpaSuppNetwork::DecreasePriority()
{
    Priority--;
}

void WpaSuppNetwork::Enable()
{
    Enabled = true;
}

void WpaSuppNetwork::Disable()
{
    Enabled = false;
}

const QString &WpaSuppNetwork::GetSsid() const
{
    return Ssid;
}

void WpaSuppNetwork::SetSsid(const QString &ssid)
{
    Ssid = ssid;
}

int WpaSuppNetwork::GetPriority() const
{
    return Priority;
}

void WpaSuppNetwork::SetPriority(int priority)
{
    Priority = priority;
}

void WpaSuppNetwork::SetCurrent()
{
    Current = true;
}

void WpaSuppNetwork::UnsetCurrent()
{
    Current = false;
}

bool WpaSuppNetwork::IsEnabled() const
{
    return Enabled;
}

bool WpaSuppNetwork::IsCurrent() const
{
    return Current;
}

bool WpaSuppNetwork::IsProtected() const
{
    return Protected;
}

bool operator == (const WpaSuppNetwork& net1, const WpaSuppNetwork& net2)
{
    return (net1.GetPriority() == net2.GetPriority());
}

bool operator < (const WpaSuppNetwork& net1, const WpaSuppNetwork& net2)
{
    return (net1.GetPriority() < net2.GetPriority());
}

void operator ++ (WpaSuppNetwork& net)
{
    net.IncreasePriority();
}

void operator -- (WpaSuppNetwork& net)
{
    net.DecreasePriority();
}
