#include "../../include/middleware/MiddlewareChain.h"
#include <muduo/base/Logging.h>

namespace http{
namespace middleware{

void MiddlewareChain::addMiddleware(std::shared_ptr<Middleware> middleware){
    middlewares_.push_back(middleware);
}

void MiddlewareChain::processBefore(HttpRequest &request){
    for(auto &middleware : middlewares_){
        middleware->before(request);
    }
}

void MiddlewareChain::processAfter(HttpResponse &response){
    // try中写出 可能抛出异常的代码
    try{
        // 反向处理响应，因为中间件的“后处理”逻辑要与请求阶段顺序相反，像栈一样后进先出。
        for(auto it = middlewares_.rbegin(); it != middlewares_.rend(); ++it){
            if(*it){
                (*it)->after(response);
            }
        }
    }
    // std::exception	C++ 标准库中所有异常的基类
    // &e	引用传参，避免拷贝，提高效率
    // const	表示不会修改这个异常对象
    // 下面这句代码的意思：“捕获任何继承自 std::exception 的异常对象，并绑定到引用变量 e 上（只读）。”
    catch(const std::exception &e){
        //  e.what() 来获取异常的错误描述信息（通常是字符串）。
        LOG_ERROR << "Error in middleware after processing: " << e.what();
    }
}

}
}