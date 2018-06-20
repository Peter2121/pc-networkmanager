//===========================================
//  PC-BSD source code
//  Copyright (c) 2015, PC-BSD Software/iXsystems
//  Available under the 3-clause BSD license
//  See the LICENSE file for full details
//===========================================
// Note: This was almost entirely written by Tim McCormick in 2009 for
//   the first PC-BSD library, and copied here by Ken Moore in 2015
//===========================================
#include "backend-network.h"
#include "backend-general.h"

//#include "sysadm-global.h"

using namespace backend;

//====================
//   STATIC LISTING FUNCTION 
//====================
QStringList NetDevice::listNetDevices(){
  QStringList result;
  struct ifaddrs *ifap;
  getifaddrs(&ifap);
  char ifName[IFNAMSIZ];
   //Probe the system for each device (one at a time)
  while (ifap != NULL){
     strncpy(ifName, ifap->ifa_name, IFNAMSIZ);
     if (result.contains(ifName) == 0) result += ifName;
     ifap = ifap->ifa_next;
  }
  //Close the 
  freeifaddrs(ifap);
  return result;
}

//=====================
//          CLASS FUNCTIONS
//=====================
//Get the name of the device (taking off any numbers)
QString NetDevice::devName(){
   uint pos = name.indexOf(QRegExp("[0-9]+$"));
   QString result = name;
   result.truncate(pos);
   return result;
}

//Return just the number of the device (removing the name)
uint NetDevice::devNum(){
   uint pos = name.indexOf(QRegExp("[0-9]+$"));
   return name.mid(pos).toInt();
}

//Fetch the IP and return it as a QString
QString NetDevice::ipAsString(){
   struct ifreq ifr;
   memset(&ifr, 0, sizeof(struct ifreq));

   strncpy(ifr.ifr_name, name.toLocal8Bit(), IFNAMSIZ);
   int s = socket(PF_INET, SOCK_DGRAM, 0);
   
   ioctl(s, SIOCGIFADDR, &ifr);
   struct in_addr in = ((sockaddr_in *) &ifr.ifr_addr)->sin_addr;
   close(s); //close the file descriptor
   return QString(inet_ntoa(in));
}

//Fetch the IPv6 and return it as a QString
QString NetDevice::ipv6AsString(){
  //Get the sockaddr for the device
  struct sockaddr *sadd = 0;
  struct ifaddrs *addrs;
  if( 0!=getifaddrs( &addrs ) ){ qDebug() << "Could not get addrs"; return ""; }
  while(sadd==0 && addrs!=0){
    if( QString(addrs->ifa_name)==name && addrs->ifa_addr->sa_family==AF_INET6){
      //Found device (with IPv6 address)
      sadd = addrs->ifa_addr;
      break;
    }else{
      //Move to the next device
      addrs = addrs->ifa_next;
    }
  }
  // Causes crash in some cases
  //free(addrs);
  if(sadd==0){ qDebug() << "No socket address found"; return ""; }
  //Now get the IPv6 address in string form
  char straddr[INET6_ADDRSTRLEN];
  int err = getnameinfo(sadd, sadd->sa_len, straddr, sizeof(straddr),NULL, 0, NI_NUMERICHOST);
  if(err!=0){ 
    qDebug() << "getnameinfo error:" << gai_strerror(err);
    return "";
  }else{
    return QString(straddr);
  }
}


//Fetch the netmask and return it as a QString
QString NetDevice::netmaskAsString(){
   struct ifreq ifr;
   memset(&ifr, 0, sizeof(struct ifreq));

   strncpy(ifr.ifr_name, name.toLocal8Bit(), IFNAMSIZ);
   int s = socket(PF_INET, SOCK_DGRAM, 0);
   
   ioctl(s, SIOCGIFNETMASK, &ifr);
   struct in_addr in = ((sockaddr_in *) &ifr.ifr_addr)->sin_addr;
  close(s); //close the file descriptor
   return QString(inet_ntoa(in));
}

//Returns the description string for the device
QString NetDevice::desc(){
  QString name, num, parent;
  if( isWireless() ){ parent = getWifiParent(); }

  if(!parent.isEmpty()){
    name = num = parent;
      uint pos = name.indexOf(QRegExp("[0-9]+$"));
      name.truncate(pos);
      num.remove(0,pos);
  }else{
    name = devName();
    num = QString::number(devNum());
  }
  return General::sysctl("dev." + name + "." + num + ".%desc");
}

//Fetch the mac address as a QString
QString NetDevice::macAsString(){
   int mib[6];
   size_t len;
   char *buf;
   struct sockaddr_dl *sdl;
   char *ptr;

   mib[0] = CTL_NET;
   mib[1] = AF_ROUTE;
   mib[2] = 0;
   mib[3] = AF_LINK;
   mib[4] = NET_RT_IFLIST;
   mib[5] = if_nametoindex(name.toLocal8Bit());

   //First find the size of the return, so we can adjust buf accordingly
   sysctl(mib, 6, NULL, &len, NULL, 0);
   buf = (char *) malloc(len);
   sysctl(mib, 6, buf, &len, NULL, 0);

   sdl = (sockaddr_dl *)(((if_msghdr *)buf)+1);
   ptr = (char *) LLADDR(sdl);
   
   QString mac;
   for (uint i=0; i < 6; i++){
      mac += QString::number(*(ptr+i), 16).right(2).rightJustified(2, '0');
      if(i<5){ mac += ":"; }
   }
   return mac;
}

//Get the status of the device (active, associated, etc...)
QString NetDevice::mediaStatusAsString(){
   struct ifmediareq ifm;
   memset(&ifm, 0, sizeof(struct ifmediareq));

   strncpy(ifm.ifm_name, name.toLocal8Bit(), IFNAMSIZ);
   int s = socket(AF_INET, SOCK_DGRAM, 0);

   ioctl(s, SIOCGIFMEDIA, &ifm);
   QString status;

   switch (IFM_TYPE(ifm.ifm_active)){
      case IFM_IEEE80211:
         if (ifm.ifm_status & IFM_ACTIVE) status = "associated";
         else status = "no carrier";
         break;
      default:
         if (ifm.ifm_status & IFM_ACTIVE) status = "active";
         else status = "no carrier";
   }
  close(s); //close the file descriptor
  return status;
}

QString NetDevice::gatewayAsString(){
  QString info = General::RunCommand("nice netstat -n -r").split("\n").filter(name).filter("default").join("\n");
  if(info.isEmpty()){ return ""; }
  //Pull the gateway out of the first line (<default> <gateway>)
  info = info.replace("\t"," ").section("\n",0,0).simplified(); //ensure proper parsing
  return  info.section(" ",1,1);
}

//Check if a device is wireless or not
bool NetDevice::isWireless(){
   struct ifmediareq ifm;
   memset(&ifm, 0, sizeof(struct ifmediareq));

   strncpy(ifm.ifm_name, name.toLocal8Bit(), IFNAMSIZ);
   int s = socket(AF_INET, SOCK_DGRAM, 0);

   ioctl(s, SIOCGIFMEDIA, &ifm);
  bool iswifi = (IFM_TYPE(ifm.ifm_active) == IFM_IEEE80211);
  close(s); //close the file descriptor
   return iswifi;
}

//Get the parent device (if this is a wireless wlan)
QString NetDevice::getWifiParent(){
   if(!name.contains("wlan")){ return ""; }
   return General::sysctl("net.wlan." + QString::number(this->devNum()) + ".%parent");
}

//See if the device is setup to use DHCP
bool NetDevice::usesDHCP(){
  //The system does not keep track of how the device's address was assigned
  //  so the closest we can get to this is to see if the system is setup to use 
  //  DHCP on startup (in /etc/rc.conf) (Ken Moore - 6/24/15)
  return !Network::readRcConf().filter(name).filter("DHCP").isEmpty();
}

//See if the device is currently in use
bool NetDevice::isUp(){
   struct ifreq ifr;
   memset(&ifr, 0, sizeof(struct ifreq));

   strncpy(ifr.ifr_name, name.toLocal8Bit(), IFNAMSIZ);
   int s = socket(AF_INET, SOCK_DGRAM, 0);

   ioctl(s, SIOCGIFFLAGS, &ifr);
   bool isup = (ifr.ifr_flags & IFF_UP);
   close(s); //close the file descriptor
   return  isup;
}

//Determine the number of packets received by the device
long NetDevice::packetsRx(){
   int mib[6];
   size_t len;
   char *buf;
   struct if_msghdr *ifm;

   mib[0] = CTL_NET;
   mib[1] = AF_ROUTE;
   mib[2] = 0;
   mib[3] = AF_LINK;
   mib[4] = NET_RT_IFLIST;
   mib[5] = if_nametoindex(name.toLocal8Bit());

   //First find the size of the return, so we can adjust buf accordingly
   sysctl(mib, 6, NULL, &len, NULL, 0);
   buf = (char *) malloc(len);
   sysctl(mib, 6, buf, &len, NULL, 0);

   ifm = (if_msghdr *) buf;

   return ifm->ifm_data.ifi_ipackets;
}

//Determine the number of packets transmitted by the device
long NetDevice::packetsTx(){
   int mib[6];
   size_t len;
   char *buf;
   struct if_msghdr *ifm;

   mib[0] = CTL_NET;
   mib[1] = AF_ROUTE;
   mib[2] = 0;
   mib[3] = AF_LINK;
   mib[4] = NET_RT_IFLIST;
   mib[5] = if_nametoindex(name.toLocal8Bit());

   //First find the size of the return, so we can adjust buf accordingly
   sysctl(mib, 6, NULL, &len, NULL, 0);
   buf = (char *) malloc(len);
   sysctl(mib, 6, buf, &len, NULL, 0);

   ifm = (if_msghdr *) buf;

   return ifm->ifm_data.ifi_opackets;
}

//Determine the number of errors received
long NetDevice::errorsRx(){
   int mib[6];
   size_t len;
   char *buf;
   struct if_msghdr *ifm;

   mib[0] = CTL_NET;
   mib[1] = AF_ROUTE;
   mib[2] = 0;
   mib[3] = AF_LINK;
   mib[4] = NET_RT_IFLIST;
   mib[5] = if_nametoindex(name.toLocal8Bit());

   //First find the size of the return, so we can adjust buf accordingly
   sysctl(mib, 6, NULL, &len, NULL, 0);
   buf = (char *) malloc(len);
   sysctl(mib, 6, buf, &len, NULL, 0);

   ifm = (if_msghdr *) buf;

   return ifm->ifm_data.ifi_ierrors;
}

//Determine the number of errors transmitted
long NetDevice::errorsTx(){
   int mib[6];
   size_t len;
   char *buf;
   struct if_msghdr *ifm;

   mib[0] = CTL_NET;
   mib[1] = AF_ROUTE;
   mib[2] = 0;
   mib[3] = AF_LINK;
   mib[4] = NET_RT_IFLIST;
   mib[5] = if_nametoindex(name.toLocal8Bit());

   //First find the size of the return, so we can adjust buf accordingly
   sysctl(mib, 6, NULL, &len, NULL, 0);
   buf = (char *) malloc(len);
   sysctl(mib, 6, buf, &len, NULL, 0);

   ifm = (if_msghdr *) buf;

   return ifm->ifm_data.ifi_oerrors;
}

//=========================
//   SETTING FUNCTIONS (requires root)
//=========================
void NetDevice::setUp(bool up){
  //This only sets it up/down for the current session - does not change usage on next boot
  General::RunCommand("ifconfig "+name+" "+ (up ? "up": "down") );
}
