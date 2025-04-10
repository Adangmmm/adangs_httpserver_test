#pragma once

#include <vector>
#include <memory>
#include "Middleware.h"

namespace http{
namespace middleware{

class MiddlewareChain{
public:

    void addMiddleware(std::shared_ptr<Middleware> middleware);
    void processBefore(HttpRequest &request);
    void processAfter(HttpResponse &response);

private:
    // 保存所有添加的中间件。
    // 使用 shared_ptr 是为了支持多态（调用虚函数）并自动管理内存。
    // 使用 vector 是为了保证中间件按添加顺序依次执行。
    std::vector<std::shared_ptr<Middleware>> middlewares_;

};

}
}