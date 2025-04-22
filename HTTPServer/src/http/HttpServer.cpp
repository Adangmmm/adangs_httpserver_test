#include "../../include/http/HttpServer.h"

#include <any>
#include <functional>
#include <memory>

namespace http{

// 默认http回应函数
void defaultHttpCallback(const HttpRequest &, HttpResponse *resp){
    resp->setStatusCode(HttpResponse::k404NotFound);
    resp->setStatusMessage("Not Found");
    resp->setCloseConnection(true);
}

HttpServer::HttpServer(int port,
                       const std::string &name,
                       bool useSSL,
                       muduo::net::TcpServer::Option option)
    : lisenAddr_(port)
    , server_(&mainLoop_, ListenAddr_, name, option)
    , useSSL_(useSSL)
    , httpCallback_(std::bind(&HttpServer::handlerRequest, this, std::placeholders::_1, std::placeholders::_2))
{
        initialize();
}

void HttpServer::start(){
    LOG_WARN << "httpServer[" << server_.name() << "] start listening on " << sever_.ipPort();
    server_.start();
    mainLoop_.loop();
}

void HttpServer::initialize(){
    // 设置回调
    server_.setConnectionCallback(std::bind(&HttpServer::onConnection, this,
                                            std::placeholders::_1));

    server_.setMassageCallback(std::bind(&HttpServer::onMessage, this,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3));
}

void HttpServer::setSslConfig(const ssl::SslConfig &config){
    if(useSSL_){
        sslCtx_ = std::make_unique<ssl::SslContext>(config);    // 创建一个unique_ptr指针，括号内是构造初始化
        if(!sslCtx_->initialize()){
            LOG_ERROR << "Failed to initialize SSL context";
            abort();    // 终止程序
        }
    }
}

void HttpServer::onConnection(const muduo::net::TcpConnectionPtr &conn){
    if(conn->connected()){
        if(useSSL_){
            // 如果开启了 SSL，就为这个连接创建一个 SslConnection 对象（专门处理 SSL 握手 & 解密）
            auto sslConn = std::make_unique<ssl:SslConnection>(conn, sslCtx_.get());
            // 设置 SSL 解密完成后的数据处理回调
            sslConn->setMessageCallback(std::bind(&HttpServer::onMessage, this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        std::placeholders::_3));
            // 把连接和 sslConn 映射起来
            sslConns_[conn] = std::move(sslConn);

            // 开始SSL握手
            sslConns_[conn]->startHandshake();
        }
        // 每个连接都设置一个 HttpContext 作为解析状态机
        // 解析收到的数据流，并构建出HTTP请求
        conn->setContext(HttpContext());
    }else{
        if(useSSL_){
            // 如果之前有开启 SSL，则从 sslConns_ 映射表中移除这个连接的 SslConnection 对象，释放资源。
            sslConns_.erase(conn);
        }
    }
}

void HttpServer::onMessage(const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::Buffer *buf,
                           muduo::Timestamp receiveTime)
{
    try{
        // 是否支持SSL
        if(useSSL_){
            LOG_INFO << "onMessage useSSL_ is true";
            // 1.查找对应的SSL链接
            auto it = sslConns_.find(conn);
            if(it != sslConns_.end()){
                LOG_INFO << "onMessage sslConns_ is not empty";
                // 2.SSL的onRead方法处理数据
                it->second->onRead(conn, buf, receiveTime);

                // 3.如果SSL握手未完成，直接返回
                if(!it->second->isHandshakeCompleted()){
                    LOG_INFO << "onMessage ssl handshake is not completed";
                    return;
                }

                // 4. 从SSL链接的解密缓冲区中获取数据
                muduo::net::Buffer *decryptedBuf = it->second->getDecryptedBuffer();
                if(decryptedBuf->readableBytes() == 0) return;

                // 5. 使用解密后的数据进行HTTP处理
                buf = decryptedBuf;
                LOG_INFO << "onMessage decryptedBuf is not empty";
            }
        }
        // HttpContext对象用于解析buf中的报文请求，并把关键信息封装进HttpRequest对象中
        HttpContext *context = boost::any_cast<HttpContext>(conn->getMutableContext());
        //解析请求内容
        if(!context->parseRequest(buf, receiveTime)){
            // 如果出错了
            conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
            conn->shutdown();
        }
        // buf中解析出完整数据包了，封装响应报文
        if(context->gotAll()){
            onRequest(conn, context->request());
            // 重置状态机，准备下一个请求
            context->reset();
        }
    }
    // 捕获异常
    catch(const std::exception &e){
        LOG_ERROR << "Exception in onMessage: " << e.what();
        conn->send("HTTP/1.1 400 Bad Reqyest \r\n\r\n");
        conn->shutdown();
    }
}

void HttpServer::onRequest(const muduo::net::TcpConnectionPTr &conn, const HttpRequest &req){
    // 检查Connection头部字段是否为close，决定当前响应之后是否关闭TCP链接
    const std::string &connection = req.getHeader("Connection");
    bool close = ((connection == "close") || (req.getVersion() == "HTTP/1.0" && connection != "Keep-Alive"));
    HttpResponse response(close);

    // 调用请求处理回调， 实际就是执行handleRequest
    httpCallback_(req, &response);

    // 准备数据，发送响应
    muduo::net::Buffer buf;
    response.appendToBuffer(&buf);
    LOG_INFO << "Sending response:\n" << buf.toStringPiece().as_string();
    conn->send(&buf);

    // 如果是短连接，则返回响应报文后就断开连接
    if(response.closeConnection()){
        conn->shutdown();
    }
}

void HttpServer::handleRequest(const HttpRequest &req, HttpResponse *resp){
    try{
        // 处理请求前的中间件
        HttpRequest mutableReq = req;
        middlewareChain_.processBefore(mutableReq);
        // 路由处理
        if(!router_.route(mutableReq, resp)){
            LOG_INFO << "Request URL: " << req.method << " " << "" << req.path();
            LOG_INFO << "Not found route, return 404";
            resp->setStatusCode(HttpResponse::k404NotFound);
            resp->setStatuesMessage("Not Found");
            resp->setCloseConnection(true); 
        }
        // 处理响应后的中间件
        // 在已经生成响应后
        middlewareChain_.processAfter(*resp);
    }
    catch(const HttpResponse &res){
        // 处理中间件抛出的响应（如CORS预检请求）
        *resp = res;
    }
    catch(const std::exception &e){
        // 错误处理
        resp->setStatusCode(HttpResponse::k500InternalServerError);
        resp->setBody(e.whwat());
    }
}

}