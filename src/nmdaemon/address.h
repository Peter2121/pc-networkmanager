#ifndef ADDRESS_H
#define ADDRESS_H

#include <string>

class address
{
public:
    virtual ~address() {}
    virtual std::string getStrAddr() const = 0;
    virtual const struct sockaddr* getSockAddr() const = 0;
    virtual short getFamily() const = 0;
    virtual bool operator==(const address&) = 0;
    virtual bool operator!=(const address&) = 0;
};

#endif // ADDRESS_H
