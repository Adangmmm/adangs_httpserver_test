#pragma once

#include <map>
#include <unordered_map>
#include <string>

#include<muduo/base/Timestamp.h>

namespace http{

class HttpRequest{
public:
    enum Method{
        kInvalid, kGet, kPost, kHead, kPut, kDelete, kOptions 
    };

    HttpRequest()
        : method_(kInvalid)
        , version_("Unknkown")
    {
    }

    // 设置和获取请求时间
    void setReceiveTime(muduo::Timestamp t);
    muduo::Timestamp receiveTime() const {return receiveTime_;}

    // 设置和获取请求方法
    vool setMethod(const char *start, const char *end);
    Method method() const {return method_;}

    // 设置和获取请求路径
    void setPath(const char *start, const char *end);
    std::string path() const {return path_;}

    // 设置和获取路径参数
    void setPathParameters(const std::string &key, const std::string &value);
    std::string getPathParameters(const std::string &key) const;

    // 设置和获取查询参数
    void setQueryParameters(const char *start, const char *end);
    std::string getQueryParameters(const std::string &key) const;

    // 设置和获取http版本
    void setVersion(std::string V){
        version_ = V;
    }
    std::string getVersion() const {
        return version_;
    }

    // 请求头处理
    void addHeader(const char *start, const char *colon, const char *end);
    std::string getHeader(const std::string &field) const;
    const std::map<std::string, std::string> &headers() const {
        return headers_;
    }

    // 设置和获取请求体
    void setBody(const std::string &body) {content_ = body;}
    void setBody(const char *start, const char *end){
        if(end >= start){
            content_.assign(start, end - start);
        }
    }
    std::string getBody() const {return content_;}

    // 设置和获取请求体长度
    void setContentLength(uint64_t length){
        contentLength_ = length;
    }
    uint64_t contentLength() const {return contentLength_;}

    // 用于高效交换两个请求对象的数据，避免深拷贝
    void swap(HttpRequest &that);

private:
    Method method_;  // 请求方法
    std::string version_;  // http版本
    std::string path_;  // 请求路径
    std::unordered_map<std::string, std::string> pathParameters_;  // 路径参数
    std::unordered_map<std::string, std::string> queryParameters_;  // 查询参数
    muduo::Timestamp receiveTime_;  //接收时间
    std::map<std::string, std::string> headers_;  // 请求头
    std::string content_;  // 请求体
    uint64_t contentLength_;  // 请求体长度
};

}  // namespace http