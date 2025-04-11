#include <iomanip>
#include <iosrteam>
#include <sstream>

#include "../../include/session/SessionManager.h"

namespace http{
namespace session{

SessionManager::SessionManager(std::unique_ptr<SessionStorage> storage)
    // std::move(storage)：表示接收外部传入的 storage 实现权；
    : storage_(std::move(storage))
    // rng_ 使用系统熵源初始化（通过 std::random_device），确保生成的 sessionId 随机性好，难以预测；
    , rng_(std::random_device{}())
{
}

std::shared_ptr<Session> SessionManager::getSession(const HttpRequest &req, HttpResponse *resp){
    // 从请求Cookie中获取sessionId
    std::string sessionId = getSessionIdFromCookie(req);

    std::shared_ptr<Session> session;
    // 尝试从storage里获取session
    if(!sessionId.empty()){
        session = storage_->load(sessionId);
    }
    // 如果没找到或已过期，则创建新的session
    if(!session || session->isExpired()){
        sessionId = generateSessionId();
        // 这是 C++11 提供的智能指针创建函数，用于创建一个 std::shared_ptr<Session> 对象。
        // 分配一个 Session 对象；使用传入的参数构造它；返回一个 shared_ptr<Session> 指针，自动管理内存。
        session = std::make_shared<Session>(sessionId, this);
        setSessionCookie(sessionId, resp);
    }
    // 如果是老session，没过期
    else{
        // 更新管理器，防止指针断联
        session->setManager(this);
    }

    session->refresh;
    storage_->save(session);  // 这里可能有问题，需要确保正确保存会话
    return session;
}

void SessionManager::destroySession(const std::sting &sessionId){
    storage_->remove(sessionId);
}

void SessionManager::cleanExpiredSessions(){
    // 注意：这个实现依赖于具体的存储实现
    // 对于内存存储，可以在加载时检查是否过期 在MemorySessionStorage类中已经实现,这里就不用写了
    // 对于其他存储的实现，可能需要定期清理过期会话
}

// 生成唯一的会话标识符，确保会话的唯一性和安全性
// 生成一个长度为 32 的随机十六进制字符串
std::string SessionManager::generateSessionId(){
    // 字符串流，像 std::cout 一样支持 << 操作；
    std::stringstream ss;
    // 这是 C++11 中的随机数分布器，用于生成均匀分布的随机整数；
    // 它的意思是：每次调用 dist(...) 时，都会返回 0 到 15（含）的随机整数；这对应了十六进制的一个字符（0~f）。
    std::uniform_int_distribution<> dist(0, 15);

    // 
    for(int i = 0; i < 32; ++i){
        // std::hex 是格式控制符，告诉 stringstream 用十六进制格式写入；
        // rng_ 是在构造函数中初始化的随机数引擎（Mersenne Twister 引擎）：
        ss << std::hex <<dist(rng_);
    }
    return ss.str();
}

std::string SessionManager::getSessionIdFromCookie(const HttpRequest &req){
    std::string sessionId;
    std::string cookie = req.getHeader("Cookie");

    if(!cookie.empty()){
        size_t pos = cookie.find("sessionId=");
        if(pos != std::string::npos){
            pos += 10;  // 跳过"sessionId="
            size_t end = cookie.find(';', pos);
            if(end != std::string::npos){
                // 如果找到了，就取出 [pos, end) 的子串；
                sessionId = cookie.substr(pos, end - pos);
            }
            else{
                // 如果没找到，说明 sessionId 是 cookie 的最后一个键值对，直接取到末尾。
                sessionId = cookie.substr(pos);
            }
        }
    }
    return sessionId;
}

void SessionManager::setSessionCookie(const::string &sessionId, HttpResponse *resp){
    // HttpOnly 防止 JS 脚本访问（有助于防止 XSS）；
    // Path=/ 表示整个网站路径都有效。
    std::string cookie = "sessionId=" + sessionId + "; Path=/; HttpOnly";
    resp->addHeader("Set-Cookie", cookie);
}


}
}