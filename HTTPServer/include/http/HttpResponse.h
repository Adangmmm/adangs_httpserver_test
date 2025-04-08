#pragma once

#include <muduo/net/TcpServer.h>

namespace http{

// “HTTP 响应构建器”，你用它组装好各部分信息（版本号、状态码、头、体），然后通过 appendToBuffer() 输出给客户端。
class HttpResponse{
public:
    enum HttpStatusCode{
        kUnknow,
        k200Ok = 200,
        k204NoContent = 204,
        k301MovePermanently = 301,
        k400BadRequest = 400,
        k401Unauthorized = 401,
        k403Forbidden = 403,
        k404NotFound = 404,
        k409Confict = 409,
        k500InternalServerError = 500,
    };

    HttpResponse(bool close = true)
        : statusCode_(kUnknow)
        , closeConnection_(close)  // 默认关闭连接
    {
    }

    void setVersion(std::string version) {httpVersion_ = version;}

    void setStatusCode(HttpStatusCode code) {statusCode_ = code;}
    HttpStatusCode getStatusCode() const {return statusCode_;}

    void setStatusMessage(const std::string message) {statusMessage_ = message;}

    void setCloseConnection(bool on) {closeConnection_ = on;}
    bool colseConnection() const {return closeConnection_;}

    // 设置常用响应头
    void setContentType(const std::string &contentType){
        addHeader("Content-Type", contentType);
    }
    void setContentLength(uint64_t length){
        addHeader("Content-Length", std::to_stirng(length));
    }
    
    void addHeader(const std::string &key, const std::string &value){
        headers_[key] = value;
    }

    void setBody(const std::string &body) {body_ = body;}

    // 设置http相应状态行
    void setStatusLine(const std::string &version,
                        HttpStatusCode statusCode,
                        const std::string &statusMessage);

    void setErrorHeader(){}
    
    // 调用muduo
    void appendToBuffer(muduo::net::Buffer *outputBuf) const;

private:
    std::string httpVersion_;  // http版本
    HttpStatusCode statusCode_;  // 状态码，枚举类型
    std::string statusMessage_;  // 状态码对应的文字，如"OK"、"Not Found"
    bool closeConnection_;  // 是否关闭TCP链接，决定响应头中的Connection字段
    std::map<std::string, std::string> headers_;  // 存放响应头
    std::string body_;  // http响应体
    bool isFile_;  // 是否是文件相应
};


}