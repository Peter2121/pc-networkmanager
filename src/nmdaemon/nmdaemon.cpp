#include "nmdaemon.h"

nmdaemon::nmdaemon(vector<nmworker*> wrks) : req_access(), work_access(), requests()
{
    running_sock_receiver = false;
    running_dispatcher = false;
    workers = wrks;
}

void nmdaemon::sock_receiver(sockpp::unix_socket sockin)
{
    unsigned long nread=0;
    unsigned long nwrite=0;
    char buf[NM_MAXBUF];
    string strBuf;
    bool res;

    if(running_sock_receiver == true)
    {
        LOG_S(ERROR) << "Socket receiver is already running";
        return;
    }
    running_sock_receiver = true;

    thread dispatch_thread(&nmdaemon::dispatcher, this, sockin.clone());

    std::unique_lock<std::mutex> ulmut_req(req_access);
    req_access.unlock();

    std::unique_lock<std::mutex> ulmut_sock(sock_access_write);
    sock_access_write.unlock();

    std::unique_lock<std::mutex> ulmut_work(work_access);
    work_access.unlock();
/*
    while (true)
    {
        nread = sockin.read(buf, sizeof(buf));
        if(nread <= 0)
            break;
        strBuf = "Received invalid request";
        sockin.write_n(strBuf.c_str(), sizeof(buf));
    }
*/
    while (true)
    {
        nread = sockin.read(buf, sizeof(buf));
        if(nread <= 0)
            break;
        strBuf = string(buf);
        nmcommand_data* request = new nmcommand_data(strBuf);
        res = false;
        if(request->isValid())
        {
            for(auto worker : workers)
            {
                res = worker->isValidCmd(request);
                if(res) break;
            }
            if(res)
            {
                LOG_S(INFO) << "Received valid request: " << strBuf;
                req_access.lock();
                requests.emplace(request);
                req_access.unlock();
            }
        }
        if(!res)
        {
            LOG_S(INFO) << "Received invalid request: " << strBuf;
            json jsResult = { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, { JSON_PARAM_ERR, JSON_DATA_ERR_INVALID_REQUEST } };
            string strResult = jsResult.dump();
            strlcpy(buf,strResult.c_str(),strResult.length()+1);
            sock_access_write.lock();
            nwrite = sockin.write_n(buf, sizeof(buf));
            sock_access_write.unlock();
            if(nwrite != sizeof(buf))
                LOG_S(WARNING) << "Error writing to socket: wrote " << nwrite << " bytes, expected " << sizeof(buf) << "bytes";
        }
        this_thread::sleep_for(chrono::milliseconds(200));
    }

    LOG_S(INFO) << "Connection closed";    
}

void nmdaemon::dispatcher(sockpp::unix_socket sockout)
{
    json jsonRes;
    string strResult;
    char buf[NM_MAXBUF];

    if(running_dispatcher == true)
    {
        LOG_S(ERROR) << "Dispatcher is already running";
        return;
    }
    running_dispatcher = true;

    std::unique_lock<std::mutex> ulmut_req(req_access);
    req_access.unlock();

    std::unique_lock<std::mutex> ulmut_sock(sock_access_write);
    sock_access_write.unlock();

    std::unique_lock<std::mutex> ulmut_work(work_access);
    work_access.unlock();

    while(true)
    {
        if(!requests.empty())
        {
            req_access.lock();
            nmcommand_data* request = requests.front();
            requests.pop();
            req_access.unlock();
            memset(&buf, 0, NM_MAXBUF*sizeof(char));
            for(auto worker : workers)
            {
                if(worker->getScope() == request->getCommand().scope)
                {
                    work_access.lock();
                    jsonRes = worker->execCmd(request);
                    work_access.unlock();
                    break;
                }
            }
//            string strReqData = request->getJsonData().dump();
//            strResult = jsonRes.dump();
            strResult = jsonRes.dump(2);
            if(strResult.length() >= NM_MAXBUF)
            {
                LOG_S(ERROR) << "No place for result, rebuild with increased NM_MAXBUF value";
                jsonRes = { { JSON_PARAM_RESULT, JSON_PARAM_ERR }, { JSON_PARAM_ERR, JSON_DATA_ERR_INTERNAL_ERROR } };
                strResult = jsonRes.dump();
            }
            strlcpy(buf,strResult.c_str(),NM_MAXBUF);
/*
            for(int i=0; i<300; i++)
            {
                if(!sock_access_write.try_lock())
                {
                    LOG_S(WARNING) << "Cannot lock socket, retrying...";
                    this_thread::sleep_for(chrono::milliseconds(50));
                }
                else
                {
                    LOG_S(WARNING) << "Socket locked successfully";
                    break;
                }
            }
*/
            sock_access_write.lock();
            sockout.write_n(buf, sizeof(buf));
            sock_access_write.unlock();
        }
        this_thread::sleep_for(chrono::milliseconds(500));
    }
}
