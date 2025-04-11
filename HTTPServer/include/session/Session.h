
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <chrono>

namespace http{
namespace session{

class SessionManager;

// 允许在类的成员函数中安全地获取 shared_ptr<Session>，通常用于异步网络编程，防止对象在操作中被提前销毁。
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(const std::string &sessionId, SessionManager *sessionManager, int maxAge = 3600);

    const std::string &getId() const {
        return sessionId_;
    }

    void setManager(SessionManager *SessionManager){
        sessionManager_ = SessionManager;
    }
    SessionManager *getManager(){
        // 裸指针注意生命周期管理
        return sessionManager_;
    }

    bool isExpired() const;
    void refresh(); // 刷新过期时间

    // 数据存取
    void setValue(const std::string &key, const std::string &value);
    std::string getValue(const std::string &key) const;
    void remove(const std::string &key);
    void clear();

private:
    std::string sessionId_;
    // 保存任意会话数据
    std::unordered_map<std::string, std::string> data_;
    // 过期时间点
    std::chrono::system_clock::time_point expiryTime_;
    // 最长生命周期
    int maxAge_;
    // 未使用智能指针避免循环引用
    SessionManager *sessionManager_;
};


} 
}