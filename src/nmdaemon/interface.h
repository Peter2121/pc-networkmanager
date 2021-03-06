#ifndef INTERFACE_H
#define INTERFACE_H

#include <string>
#include <exception>
#define LOGURU_WITH_STREAMS 1
#include "loguru.hpp"
#include "addr.h"

class interface
{
protected:
    std::string strName;
    std::vector<addr*> vectAddrs;
    bool hasIPv4;
    bool hasIPv6;
    bool isIfUp;
public:
    interface();
    interface(std::string);
    void setName(std::string);
    void addAddress(struct ifaddrs*);
    const std::vector<addr*>* getAddrs() const;
    const nlohmann::json getIfJson() const;
    const nlohmann::json getIfString() const;
    ~interface();
};

#endif // INTERFACE_H
