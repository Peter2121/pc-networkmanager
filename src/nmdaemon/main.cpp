#include <iostream>
//#include <stdio.h>
//#include <string.h>
#include <unistd.h>
//#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
//#include <stdlib.h>
#include <thread>

#include "json.hpp"

#define LOGURU_WITH_STREAMS 1
#include "loguru.hpp"

#include "sockpp/unix_acceptor.h"
#include "sockpp/version.h"

#include "nmdaemon.h"
#include "nmworker.h"
#include "workers.h"
#if defined (WORKER_DUMMY)
#include "dummy_worker.h"
#endif
#if defined (WORKER_SYSTEM)
#include "system_worker.h"
#endif
#if defined (WORKER_INTERFACE)
#include "if_worker.h"
#endif

using namespace std;

/*
void run_echo(sockpp::unix_socket sock)
{
    int n;
    char buf[512];

    while ((n = sock.read(buf, sizeof(buf))) > 0)
        sock.write_n(buf, n);

    LOG_S(INFO) << "Connection closed";
}
*/

int main(int argc, char* argv[])
{
    const string DEFAULT_SOCKET_PATH = "/var/run/nmd.socket";

//    const int OPT_ON = 1;
    const int MAXBUF = 512;
    char buf[MAXBUF];
    bool res;
    std::vector<nmdaemon*> daemons;
    std::vector<nmworker*> workers;
    string sock_path = DEFAULT_SOCKET_PATH;
    struct sockaddr_un sock_addr;
    memset(&sock_addr, 0, sizeof(struct sockaddr_un));
    sock_addr.sun_family = AF_UNIX;
    memset(&buf, 0, MAXBUF*sizeof(char));

    loguru::init(argc, argv);

    if ( getuid() != 0 )
    {
        LOG_S(WARNING) << "This program must be run as root!";
    }

#if defined (WORKER_DUMMY)
    workers.push_back(new WORKER_DUMMY());
#endif

#if defined (WORKER_SYSTEM)
    workers.push_back(new WORKER_SYSTEM());
#endif

#if defined (WORKER_INTERFACE)
    workers.push_back(new WORKER_INTERFACE());
#endif

    sockpp::socket_initializer sockInit;
    sockpp::unix_acceptor sockAcc;

    res = sockAcc.open(sockpp::unix_address(sock_path));

    if (!res)
    {
        LOG_S(ERROR) << "Error creating the acceptor: " << sockAcc.last_error_str();
        return -1;
    }
    LOG_S(INFO) << "Acceptor bound to address: '" << sockAcc.address();

    if(chmod(DEFAULT_SOCKET_PATH.c_str(), S_ISGID|S_IRGRP|S_IWGRP)<0)
    {
        LOG_S(WARNING) << "Cannot change permissions for socket: '" << sockAcc.address();
    }

    while (true)
    {
        auto sock = sockAcc.accept();
        LOG_S(INFO) << "Received a connection";

        if (!sock) {
            LOG_S(ERROR) << "Error accepting incoming connection: " << sockAcc.last_error_str();
        }
        else {
            nmdaemon* daemon = new nmdaemon(workers);
            daemons.push_back(daemon);
            thread thr(&nmdaemon::sock_receiver, daemon, move(sock));
//            thread thr(run_echo, std::move(sock));
            thr.detach();
        }
    }
/*
    if ( (sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
      LOG_S(ERROR) << "socket error";
      exit(-1);
    }

    strncpy(sock_addr.sun_path, sock_path.c_str(), sizeof(sock_addr.sun_path)-1);

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &OPT_ON, sizeof(int)) < 0)
    {
      LOG_S(ERROR) << "setsockopt(SO_REUSEADDR) failed";
      exit(-1);
    }

    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEPORT, &OPT_ON, sizeof(int)) < 0)
    {
      LOG_S(ERROR) << "setsockopt(SO_REUSEPORT) failed";
      exit(-1);
    }

    if ( ::bind(sock_fd, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr_un))<0 )
    {
      LOG_S(ERROR) << "bind error: " << errno;
      exit(-1);
    }
    else {
      LOG_S(INFO) << "Correctly bind to " << sock_path;
    }

    if (listen(sock_fd, 5) == -1)
    {
      LOG_S(ERROR) << "listen error";
      exit(-1);
    }

    while (true) {
      if ( (cl = accept(sock_fd, NULL, NULL)) == -1)
      {
        LOG_S(ERROR) << "accept error";
        continue;
      }

      while ( (read_count=read(cl,buf,sizeof(buf))) > 0) {
        LOG_S(INFO) << "read " << read_count << " bytes: " << buf;
      }

      if (read_count == -1) {
        LOG_S(ERROR) << "read error";
        exit(-1);
      }
      else if (read_count == 0)
      {
        LOG_S(INFO) << "EOF" << endl;
        close(cl);
      }
    }
*/
    return 0;
}
