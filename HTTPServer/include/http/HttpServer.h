#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>

#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "../router/Router.h"
#include "../session/SessionManager.h"
#include "../middleware/MiddlewareChain.h"
#include "../middleware/cors/CorsMiddleware.h"
#include "../ssl/SslConnection.h"
#include "../ssl/SslContext.h"

class HttpRequest;
class HttpResponse;

namespace http{

class HttpServer : muduo::noncopyable{
public:
    using HttpCallback = std::function<void (const HttpRequest &, HttpResponse *)>;

    HttpServer(int port,
               const std::string &name,
               bool useSSL = false,
               muduo::net::TcpServer::Option option = muduo::net::TcpServer::kNoReusePort);

    // 基础配置
    void setThreadNum(int numThreads){
        server_.setThreadNum(numThreads);
    }

    void start();

    muduo::net::EventLoop *getLoop() const{
        return server_.getLoop();
    }

    // 注册回调
    void setHttpCallback(const HttpCallback &cb){
        httpCallback_ = cb;
    }

    // 路由注册接口
    void Get(const std::string &path, const HttpCallback &cb){
        router_.registerCallback(HttpRequest::kGet, path, cb);
    }

    void Get(const std::string &path, router::Router::HandlerPtr handler){
        router_.registerHandler(HttpRequest::kGet, path, handler);
    }
    void Post(const std::string &path, const HttpCallback &cb){
        router_.registerCallback(HttpRequest::kPost, path, cb)
    }

    void Post(const std::string &path, router::Router::HandlerPtr handler){
        router_.regiserHandler(HttpRequest::kPost, path, handler)
    }

    void addRoute(HttpRequest::Method method, const std::string &path, const router::Router::HandlerCallback &cb){
        router_.addRegexCallback(method, path, cb);
    }

    void addRoute(HttpRequest::Method method, const std::string &path, router::Router::HandlerPtr handler){
        router_.addRegexHandler(method, path, handler);
    }

    // 会话管理
    void setSessionManager(std::unique_ptr<session::SessionManager> manager){
        sessionManager_ = std::move(manager);
    }

    session::SessionManager *getSessionManager() const {
        return sessionManager_.get();   // 返回智能指针中保留的裸指针
    }

    // 添加中间件
    void addMiddleware(std::shared_ptr<middleware::Middleware> middleware){
        middlewareChain_.addMiddleware(middleware);
    }

    // SSL配置
    void enableSSL(bool enable){
        useSSL_ = enable;
    }

    void setSslConfig(const ssl::SslConfig &config);

private:
    void initialize();

    // 连接管理
    void onConnection(const muduo::net::TcpConnectionPtr &conn);
    // 数据接收与解析 onMessage()+HttpContext
    void onMessage(const muduo::net::TcpConnectionPtr &conn,
                   muduo::net::Buffer *buf,
                   muduo:Timestamp receiveTime);
    //
    void onRequest(const muduo::net::TcpConnectionPtr &, const httpRequest &);
    // 请求分发 handleRequest() + router_
    void handleRequest(const HttpRequest &req, HttpResponse *resp);


private:
    muduo::net::InetAddress                     listenAddr_;    // 监听地址
    muduo::net::TcpServer                       server_;
    muduo::net::EventLoop                       mainLoop_;
    HttpCallback                                httpCallback_;     // 请求回调
    router::Router                              router_;
    std::unique_ptr<session::SessionManager>    sessionManager_;
    middleware::MiddlewareChain                 middlewareChain_;
    std::unique_ptr<ssl::SslContext>            sslCtx_;
    bool                                        useSSL_;
    // TcpConnectionPtr -> SslConnection
    std::map<muduo::net::TcpConnectionPtr, std:unique_ptr<ssl::SslConnection>> sslConns_; 

};

}