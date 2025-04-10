#pragma once

#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

namespace http{
namespace middleware{

// 定义一个抽象基类（接口类）
class Middleware{
public:

    // 定义虚析构函数，确保多态删除时不内存泄漏
    virtual ~Middleware() = default;

    // 纯虚函数，由子类实现。引用传递，允许修改请求内容
    // 在请求被处理器处理前执行的逻辑（比如认证、日志、CORS等）
    virtual void before(HttpRequest &request) = 0;

    // 在响应生成后、返回客户端前，可以做一些统一的处理（比如添加响应头、包装格式、日志记录等）
    // 修改response会影响最终返回内容
    virtual void after(HttpResponse &response) = 0;

    // 用于构建中间件链（责任链模式）
    // 吧下一个中间件保存在nextMiddleware_之阵中，以便在当前中间件执行完后继续传递
    void setNext(std::shared_ptr<Middleware> next){
        nextMiddleware_ = next;
    }

protected:
    std::shared_ptr<Middleware> nextMiddleware_;
};

}

}