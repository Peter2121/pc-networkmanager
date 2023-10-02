#include "wpasuppnetwork.h"

WpaSuppNetwork::WpaSuppNetwork() : WpaSuppNetwork(0, "", "", 0, false, false, false)
{
}

WpaSuppNetwork::WpaSuppNetwork(int id, QString ssid, QString bssid, int priority, bool enabled, bool current, bool protectedd, QString key, QString sec) :
    Id(id), Ssid(ssid), Bssid(bssid), Priority(priority), Enabled(enabled), Current(current), Protected(protectedd), Key(key), Security(sec)
{
}

WpaSuppNetwork::WpaSuppNetwork(int id, QString ssid, QString bssid, int priority, bool enabled, bool current, bool protectedd) :
    Id(id), Ssid(ssid), Bssid(bssid), Priority(priority), Enabled(enabled), Current(current), Protected(protectedd), Key(QString()), Security(QString())
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

const QString &WpaSuppNetwork::GetBssid() const
{
    return Bssid;
}

void WpaSuppNetwork::SetSsid(const QString &ssid)
{
    Ssid = ssid;
}

void WpaSuppNetwork::SetBssid(const QString &bssid)
{
    Bssid = bssid;
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

int WpaSuppNetwork::GetId() const
{
    return Id;
}

const QString WpaSuppNetwork::GetKey() const
{
    return Key;
}

void WpaSuppNetwork::SetKey(const QString& new_key)
{
    Key = new_key;
}

QString WpaSuppNetwork::GetSecurity() const
{
    return Security;
}

void WpaSuppNetwork::SetSecurity(const QString &security)
{
    Security = security;
}

void WpaSuppNetwork::SetId(int id)
{
    Id = id;
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
