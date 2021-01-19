#ifndef ADDRESS_IP4_H
#define ADDRESS_IP4_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "address.h"

class address_ip4 : public address
{
protected:
    struct in_addr ip_addr;
    const short family = AF_INET;
    struct sockaddr* sock_addr;
    std::string strAddr;
    void setIpAddr();
public:
    address_ip4(const struct sockaddr*);
    address_ip4(std::string);
    ~address_ip4();
    std::string getStrAddr() const;
    const struct sockaddr* getSockAddr() const;
    short getFamily() const;
    bool operator==(const address&);
    bool operator!=(const address&);
};

#endif // ADDRESS_IP4_H
