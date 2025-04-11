#include "../../include/session/Session.h"
#include "../../include/session/SessionManager.h"

namespace http{
namespace session{

Session::Session(const std::string &sessionId, SessionManager *sessionManager, int maxAge = 3600)
    : sessionId_(sessionId)
    , maxAge_(maxAge)
    , sessionManager_(sessionManager)
{
    refresh();
}

bool Session::isExpired() const{
    return std::chrono::system_clock::now() > expiryTime_;
}

void Session::refresh(){
    expiryTime_ = std::chrono::system_clock::now() + std::chrono::seconds(maxAge_);
}


void Session::setValue(const std::string &key, const std::string &value){
    data_[key] = value;
    // 如果设置了manager，则自动保存更改
    if(sessionManager_){
        sessionManager_->updateSession(shared_from_this());
    }
}

std::string Session::getValue(const std::string &key) const{
    auto it = data_.find(key);
    return it != data_.end() ? it->second : std::string();
}

void Session::remove(const std::string &key){
    data_.erase(key);
}

void Session::clear(){
    data_.clear();
}


}
}