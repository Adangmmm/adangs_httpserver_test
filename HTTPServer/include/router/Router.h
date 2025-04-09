#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <regex>
#include <vector>

#include "RouterHandler.h"
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

namespace http{
namespace router{

class Router{
public:
    // 封装了对象式路由处理器，继承自RouterHandler
    using HandlerPtr = std::shared_ptr<RouterHandler>;
    // 封装了函数式路由处理器
    using HandlerCallback = std::function<void(const HttpRequest &, HttpResponse *)>;

    // 路由键 （method方法+URI请求路径）
    struct RouteKey{
        HttpRequest::Method method;
        std::string path;

        bool operator==(const RouteKey &other) const {
            return method == other.method && path == other. path;
        }
    };

    // 为RouteKey定义哈希函数
    struct RouteKeyHash{
        size_t operator()(cosnt RouteKey &key) const{
            // static_cast<int>(key.method)：把枚举类型显式转换为对应的整数
            // std::hash<int>{}：创建了一个 int 类型的哈希函数对象
            // std::hash<int>{}(...)：调用该哈希函数，计算出这个整数的哈希值
            size_t methodHash = std::hash<int>{}(static_cast<int>(key.method));
            size_t pathHash = std::hash<std::string>{}(key.path);
            return methodHash * 31 + pathHash;
        }
    };

    // 注册对象式路由处理器
    void registerHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler);
    // 注册回调函数式路由处理器
    void registerCallback(HttpRequest::Method method, const std::string &path, HandlerCallback &callback);

    // 注册动态路由处理器对象
    void addRegexHandler(HttpRequest::Method method, const std::string &path, HandlerPtr handler){
        std::regex pathRegex = convertToRegex(path);
        regexHandlers_.emplace_back(method, pathRegex, handler);
    }
    // 注册动态路由处理器函数
    void addRegexCallback(HttpRequest::Method method, const std::string &path, HandlerCallback &callback){
        std::regex pathRegex = convertToRegex(path);
        regexCallbacks_.emplace_back(method, pathRegex, callback);
    }

    // 该类的核心函数 ，处理请求
    bool route(const HttpRequest &req, HttpResponse *resp);



private:
    // 将路径模式（如"/user/:id"）转化为正则表达式
    std::regex convertToRegex(const std::string &pathPattern){
        // 例子 "/user/:id" → "/user/([^/]+)"
        //      "/blog/:bid/comments/:cid" → "/blog/([^/]+)/comments/([^/]+)"
        std::string regexPattern = "^" + std::regex_replace(pathPattern, std::regex(R"(/:([^/]+))"), R"(/([^/]+))") + "$";
        return std::regex(regexPattern);
    }

    // 从正则匹配结果中提取动态路径参数，存储进 HTTP 请求对象中。
    void extractPathParameters(const std::smatch &match, HttpRequest &request){
        // smatch 代表 string match，它本质上是一个类似数组的对象，存储了正则表达式匹配到的内容。
        for(size_t i = 1; i < match.size(), ++i){
            request.setPathParameters("param" + std::to_string(i), match[i].str());
        }
        /* 例子:
        pathPattern: "/user/:id/post/:pid"
        实际访问:    "/user/42/post/99"
        匹配结果 match：
            match[0] = "/user/42/post/99"
            match[1] = "42"
            match[2] = "99"

        循环会执行：
            request.setPathParameters("param1", "42");
            request.setPathParameters("param2", "99");
        */
    }

private:
    // 用于存储动态路由的信息
    struct RouterCallbackObj{
        HttpRequest::Method method_;
        std::regex pathRegex_;
        HandlerCallback callback_;
        RouterCallbackObj(HttpRequest::Method method, std::regex pathRegex, HandlerCallback callback)
            : method_(method), pathRegex_(pathRegex), callback_(callback) {}
    };

    struct RouterHandlerObj{
        HttpRequest::Method method_;
        std::regex pathRegex_;
        HandlerPtr handler_;
        RouterHandlerObj(HttpRequest::Method method, std::regex pathRegex, HandlerPtr handler)
            : method_(method), pathRegex_(pathRegex), handler_(handler) {}
    };

    // 存放精准路由
    std::unordered_map<RouteKey, HandlerPtr, RouteKeyHash> handlers_;
    std::unordered_map<RouteKey, HandlerCallback, RouteKeyHash> callbacks_;
    // 存放正则匹配路由（动态路由）
    std::vector<RouterHandlerObj> regexHandlers_;
    std::vector<RouterCallbackObj> regexCallbacks_;
};

}
}
