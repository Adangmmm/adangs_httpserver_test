#include <iostream>

#include "../../include/session/SessionStorage.h"

namespace http{
namespace session{

void MemorySessionStorage::save(std::shared_ptr<Session> session){
    sessions_[session->getId()] = session;
}

std::shared_ptr<Session> MemorySessionStorage::load(const std::string &sessionId){
    auto it = sessions_.find(sessionId);
    if(it != sessions_.end()){
        if(!it->second->isExpired()){
            return it->second;
        }
        else{
            // 会话过期，则从存储中移除
            sessions_.erase(it);
        }
    }
    // 会话不存在，返回nullptr
    return nullptr;
}

void MemorySessionStorage::remove(const std::string &sessionId){
    sessions_.erase(sessionId);
}

}
}