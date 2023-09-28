#ifndef WPASUPPNETWORK_H
#define WPASUPPNETWORK_H

#include <qstring.h>

class WpaSuppNetwork
{
public:
    WpaSuppNetwork();
    WpaSuppNetwork(int, QString, QString, int, bool, bool, bool);
    void Enable();
    void Disable();

    const QString &GetSsid() const;
    const QString &GetBssid() const;
    void SetSsid(const QString &newSsid);
    void SetBssid(const QString &newBssid);
    const QString GetKey() const;
    void SetKey(const QString &key);
    int GetPriority() const;
    void SetPriority(int newPriority);
    void IncreasePriority();
    void DecreasePriority();
    void SetCurrent();
    void UnsetCurrent();
    bool IsEnabled() const;
    bool IsCurrent() const;
    bool IsProtected() const;
    int GetId() const;


protected:
    int Id;
    QString Ssid;
    QString Bssid;
    int Priority;
    bool Enabled;
    bool Current;
    bool Protected;
    QString Key;
};

bool operator == (const WpaSuppNetwork&, const WpaSuppNetwork&);
bool operator < (const WpaSuppNetwork&, const WpaSuppNetwork&);
void operator ++ (WpaSuppNetwork&);
void operator -- (WpaSuppNetwork&);


#endif // WPASUPPNETWORK_H
