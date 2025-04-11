#pragma once

#include <memory>
#include <random>

#include "SessionStorage.h"
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

namespace http{
namespace session{

class SessionManager{
public:
    explicit SessionManager(std::unique_ptr<SessionStorage> storage);

    // 从请求中查找是否已有sessionId; 找不到或者session过期，创建新的session；
    // 创建了新的session，通过setSessionCookie()写入响应Cookie
    std::shared_ptr<Session> getSession(const HttpRequest &req, HttpResponse *resp);

    // 销毁会话，一般用于登出or安全清除的场景
    void destroySession(const std::string &sessionId);

    // 清理过期会话
    // 遍历已有会话，删除所有已经过期的
    void cleanExpiredSessions();

    // 更新会话，立即保存
    void updateSession(std::shared_ptr<Session> session){
        storage_->save(session);
    }

private:
    // 生成唯一的Session ID
    std::string generateSessionId();
    // 从Cookie中获取sessionId
    std::string getSessionIdFromCookie(const HttpRequest &req);
    // 设置Cookie返回给客户端
    void setSessionCookie(const std::string &sessionId, HttpResponse *resp);

private:
    // 指向具体的存储实现
    std::unique_ptr<SessionStorage> storage_;
    // 使用梅森旋转算法的伪随机数生成器
    std::mt19937 rng_;

};


}
}