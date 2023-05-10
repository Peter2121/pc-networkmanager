#ifndef WPASUPPNETWORK_H
#define WPASUPPNETWORK_H

#include <qstring.h>

class WpaSuppNetwork
{
public:
    WpaSuppNetwork();
    WpaSuppNetwork(QString, int, bool, bool, bool);
    void operator ++ ();
    void operator -- ();
    bool operator == (const WpaSuppNetwork&);
    bool operator < (const WpaSuppNetwork&);
    void Enable();
    void Disable();

    const QString &GetSsid() const;
    void SetSsid(const QString &newSsid);
    int GetPriority() const;
    void SetPriority(int newPriority);
    void SetCurrent();
    void UnsetCurrent();
    bool IsEnabled() const;
    bool IsCurrent() const;
    bool IsProtected() const;

protected:
    QString Ssid;
    int Priority;
    bool Enabled;
    bool Current;
    bool Protected;
};

#endif // WPASUPPNETWORK_H
