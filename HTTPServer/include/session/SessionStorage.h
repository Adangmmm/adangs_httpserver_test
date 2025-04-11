#pragma once

#include <memory>

#include "Session.h"

namespace http{
namespace session{

// 抽象接口类
class SessionStorage{
public:
    virtual ~SessionStorage() = default;

    virtual void save(std::shared_ptr<Session> session) = 0;    // 保存
    virtual std::shared_ptr<Session> load(const std::string &sessionId) = 0;  // 加载
    virtual void remove(const std::string &sessionId) = 0;  // 删除
};

// 基于内存的会话存储实现
class MemorySessionStorage : public SessionStorage{
public:
    void save(std::shared_ptr<Session> session) override;
    std::shared_ptr<Session> load(const std::string &sessionId) override;
    void remove(const std::string &sessionId) override;

private:
    std::unordered_map<std::string, std::shared_ptr<Session>> sessions_;
};


}
}