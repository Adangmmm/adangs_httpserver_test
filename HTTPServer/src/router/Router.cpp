#include "../../include/router/Router.h"
#include <muduo/base/Logging.h>

namespace http{
namespace router{

void Router::registerHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler){
    RouteKey key{method, path};
    // 将这个处理器存入 handlers_（一个 unordered_map），通过 std::move 将 handler 的所有权转移到容器中。
    handlers_[key] = std::move(handler);
}

void Router::registerCallback(HttpRequest::Method method, const std::string &path, HandlerCallback &callback){
    RouteKey key{method, path};
    handlers_[key] = std::move(callback);
}

// 根据传入的http请求req，查找并调用对应的路由处理器或回调函数，最终生成响应resp
bool Router::route(const HttpRequest &req, HttpResponse *resp){
    RouteKey key{req.method(), req.path()};

    // 查找精确对象式路由处理器
    auto handlerIt = handlers_.find(key);
    if(handlerIt != handlers_.end()){
        handlerIt->second->handle(req, resp);
        return true;
    }

    // 查找精确路由函数
    auto callbackIt = callbacks_.find(key);
    if(callbackIt != callbacks_.end()){
        callbackIt->second(req, resp);
        return true;
    }

    // 查找动态路由处理器（正则）
    /* C++17结构化绑定语法
    for (const RouteHandlerObj& obj : regexHandlers_) {
        const auto& method = obj.method_;
        const auto& pathRegex = obj.pathRegex_;
        const auto& handler = obj.handler_;
        ...
    }
    */
    for(const auto &[method, pathRegex, handler] : regexHandlers_){
        std::smatch match;
        std::string pathStr(req.path());
        // 方法和路径匹配，则执行处理器
        /* bool std::regex_match(const std::string& s, std::smatch& m, const std::regex& re);
            s：要匹配的字符串；
            m：匹配结果（std::smatch 是 std::match_results<std::string::const_iterator> 的别名）；
            re：正则表达式；如果整个字符串和正则表达式完全匹配，返回 true，否则返回 false。
        */
        if(method == req.method() && std::regex_match(pathStr, match, pathRegex)){
            // 克隆一个新的请求对象，不影响原始请求
            HttpRequest newReq(req);
            extractPathParameters(match, newReq);

            handler->handle(newReq, resp);
            return true;
        }
    }

    // 查找动态路由处理函数（正则）
    for(const auto &[method, pathRegex, callback] : regexCallbacks_){
        std::smatch match;
        std::string pathStr(req.path());
        // 方法和路径匹配，则执行处理函数
        if(method == req.method() && std::regex_match(pathStr, match, pathRegex)){
            HttpRequest newReq(req);
            extractPathParameters(match ,newReq);

            callback(newReq, resp);
            return true;
        }
    }

    return false;

}

}
}