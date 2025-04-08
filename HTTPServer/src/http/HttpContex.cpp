#include "../../include/http/HttpContext.h"

using namespace muduo;
using namespace muduo::net;

namespace http{

bool HttpContext::parseRequest(Buffer *buf, Timestamp, receiveTime){
    bool ok = true;  // 解析每行请求格式是否正确
    bool hasMore = true;  // 是否有更多数据需要解析
    while(hasMore){
        if(state_ == kExpectRequestLine){   // 根据状态来执行相应逻辑段
            const char *crlf = buf->findCRLF();  // findCRLF()返回\r的指针, 寻找请求行行尾
            if(crlf){
                ok = processRequestLine(buf->peek(), crlf);
                if(ok){
                    request_.setReceiveTime(receiveTime);  // 设置请求时间
                    buf->retrieveUntil(crlf + 2);  // 移动指针，跳过请求行
                    state_ = kExpectHeaders;  // 更新状态
                }
                else{  // 请求行解析失败
                    hasMore = false;
                }
            }
            else{   // 没有找到\r\n，请求行不完整
                hasMore = false;
            }
        }

        else if(state_ == kExpectHeaders){
            const char *crlf = buf->findCRLF();
            if(crlf){
                // Header键值对分隔符  冒号
                const char *colon = std::find(buf->peek(), crlf, ':');
                if(colon < crlf){
                    request_.addHeader(buf->peek(), colon, crlf);  // 添加请求头
                }
                else if(buf->peek() == crlf){
                    // 空行，结束Header
                    // 根据请求方法 和 Content-Length 判断是否要继续读取body
                    if(request_.method() == HttpRequest::kPost ||
                       request_.method() == HttpRequest::kPut){
                        std:: string contentLength = request_.getHeader("Content-Length");
                        if(!contentLength.empty()){
                            request_.setContentLength(std::stoi(contentLength));
                            if(request_.contentLength() > 0){
                                // Put 和 Post 请求，并且存在内容，则继续解析请求体
                                state_ = kExpectBody;
                            }
                            else{
                                // 没有请求体，解析完成
                                state_ = kGotAll;
                                hasMore = false;
                            }
                        }
                        else{
                            // POST/PUT没有Content-Length，HTTP语法错误
                            ok = false;
                            hasMore = false;
                        }
                    }
                }
                buf->retrieveUntil(crlf + 2);  //开始读指针指向下一行数据
            }
            else{
                hasMore = false;  // 没有找到\r\n，请求头不完整
            }
        }

        else if(state_ == kExpectBody){
            if(buf->readableBytes() < request_.contentLength()){
                // 缓冲区中的数据不完整，等待更多数据到来
                hasMore = false;
                return true;
            }

            // 读取Content-Length指定的数据长度
            std::string body(buf->peek(), buf->peek() + request_.contentLength());
            request_.setBody(body);

            buf->retrieve(request_.contentLength());  // 移动指针，跳过请求体

            state_ = kGotAll;  // 解析完成
            hasMore = false;
        }
    }
    return ok;  // ok为false代表报文语法解析错误
}



// 解析请求行
bool HttpContext::processRequestLine(const char *begin, const char *end){
    bool succeed = false;
    const char *start = begin;
    const char *space = std::find(start, end, ' ');  // 找到第一个空格

    // 解析形如 GET /search?q=chat HTTP/1.1\r\n 的请求行

    // 第一个空格前是请求方法（GET / POST 等）→ request_.setMethod()
    if(space != end && reques_.setMethod(start, space)){
        start = space + 1;
        // 第二个空格
        space = std::find(start, end, ' ');
        if(space != end){
            const char *argumentStart = std::find(start, space, '?');
            // 有问号，说明是带参数的请求
            if(argumentStart != space){
                requet_.setPath(start, argumentStart);  // 设置路径   /search
                request_.setQueryParameters(argumentStart + 1, space);  //设置查询参数   q=chat
            }
            // 不带参数的请求
            else{
                request_.setPath(start, space);
            }

            start = space + 1;  // 更新start
            // 剩下的版本信息满足格式化要求的话，成功
            succeed = ((end - start == 8) && std::equal(start, end -1, "HTTP/1."));
            if(succeed){
                if(*(end - 1) == '1'){
                    request_.setVersion("HTTP/1.1");
                }
                else if(*(end - 1) == '0'){
                    request_.setVersion("HTTP/1.0");
                }
                else{
                    succeed = false;  // 不支持的版本号
                }
            }
        }
    }
    return succeed;
}


}  // namespace http