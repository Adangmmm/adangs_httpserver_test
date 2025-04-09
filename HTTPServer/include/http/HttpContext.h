#pragma once

#include <iostream>
#include <muduo/net/TcpServer.h>

#include "HttpRequest.h"

namespace http{

// 用于在HTTP请求解析过程中存储当前解析的状态、请求内容（HttpRequest），并提供解析方法。
class HttpContext{
public:
    // 构建一个有限状态机的状态枚举，表示当前解析进度
    enum HttpRequestParseState{
        kExpectRequestLine,  // 正在解析请求行（第一行） 如 get /index HTTP/1.1
        kExpectHeaders,  // 正在解析请求头
        kExpectBody,    // 正在解析请求体
        kGotAll,        // 解析完成
    };

    HttpContext()
        : state_(kExpectRequestLine)
    {
    }

    // 从缓冲区buf中解析请求内容，将解析数据填充到request_中
    bool parseRequest(muduo::net::Buffer *buf, muduo::Timestamp receiverTime);
    // 是否解析完成
    bool gotAll() const {
        return state_ == kGotAll;
    }

    // 将状态重置为初始值；
    // 创建一个临时空的 HttpRequest 对象，交换到 request_ 中以清空旧数据。
    // 这样写是为了避免构造新对象时调用 request_ 的析构函数，性能更好。
    void reset(){
        state_ = kExpectRequestLine;
        HttpRequest dummyData;
        request_.swap(dummyData);  // 交换数据，避免深拷贝
    }

    // 分别返回常量和可修改引用， 用于访问和修改HttpRequest对象
    const HttpRequest &request() const {
        return request_;
    }
    HttpRequest &request(){
        return request_;
    }

private:
    // 解析第一行请求行
    bool processRequestLine(const char *start, const cahr *end);
private:
    HttpRequestParseState state_;  // 当前解析状态    
    HttpRequest request_;  // 当前正在构建的请求对象

};

}