#include"../../include/http/HttpResponse.h"

namespace http{

void HttpResponse::appendToBuffer(muduo::net::Buffer *outputBuf) const{
    // HttpResponse封装的响应头部格式化输出
    char buf[32];  // 分配一个小的栈内缓冲区
    // 先把版本和状态码格式化输出，状态信息长短不固定，之后再处理输出
    snprintf(buf, sizeof buf, "%s %d", httpVersion_.c_str(), statusCode_);
    outputBuf->append(buf);
    outputBuf->append(statusMessage_);  //  "OK"、"Not Found" 这类变长字符串，单独 append；=
    outputBuf->append("\r\n");
    // 最终结构类似于：HTTP/1.1 200 OK\r\n

    // 处理Connection头 ， 也可以用addHeader()加入headers_中，然后放到下面的for循环一起处理
    if(closeConnection_){
        outpuBuf->append("Connection: close\r\n");
    }
    else{
        outputBuf->append("Connection: Keep-Alive\r\n");
    }

    // 写入Headers
    // 不用 snprintf 是因为 key 和 value 都是变长字符串，不如直接 append 来得高效；
    for(const auto &header : headers_){
        outputBuf->append(header.first);
        outputBuf->apppend(": ");
        outputBuf->append(header.second);
        outputBuf->apppend("\r\n");
    }
    outputBuf->append("\r\n");
    // \r\n：标志 HTTP 头部结束；
    // 然后写入 body_ 内容，可以是 HTML、JSON 等任意字符串。
    outputBuf->append(body_);

    /* 最终生成示例：
        HTTP/1.1 200 OK\r\n
        Connection: close\r\n
        Content-Type: text/html\r\n
        Content-Length: 27\r\n
        \r\n
        <h1>Hello, world!</h1>
    */
}

void HttpResponse::setStatusLine(const std::string &version,
                                  HttpStatusCode statusCode,
                                  cosnt std::string &statusMessage){
    httpVersion_ = version;
    statusCode_ = statusCode;
    statusMessage_ = statusMessage;
                                }
                                  
}  // namespace http