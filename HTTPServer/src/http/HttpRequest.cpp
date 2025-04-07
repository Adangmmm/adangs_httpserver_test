#include "../../include/http/HttpRequest.h"

namespace http{

void HttpRequest::setReceiveTime(muduo::Timestamp t){
    receiveTime_ = t;
}

bool HttpRequest::setMethod(const char *start, const char *end){
    // 断言：assert 是一个宏，用于在运行时检查一个条件是否为真，如果条件不满足，则运行时将终止程序的执行并输出一条错误信息。
    assert(method_ == kInvalid);
    std::string m(start, end);
    if(m == "GET"){
        method_ = kGet;
    }
    else if(m == "POST"){
        method_ = kPost;
    }
    else if(m == "PUT"){
        method_ = kPut;
    }
    else if(m == "DELETE"){
        method_ = kDelete;
    }
    else if(m == "OPTIONS"){
        method_ = kOptions;
    }
    else{
        method_ = kInvalid;
    }
    return method_ != kInvalid;
}

void HttpRequest::setPath(const char *start, const char *end){
    // assign() 用于将指定范围内的字符复制到当前字符串中
    path_.assign(start, end);
}

void HttpRequest::setPathParameters(const std::string &key, const std::string &value){
    pathParameters_[key] = value;
}
std::string HttpRequest::getPathParameters(const std::string &key) const {
    auto it = pathParameters_.find(key);
    if(it != pathParameters_.end()){
        return it->second;
    }
    // 没有的话返回空字符串
    return "";
}

/* 查询参数例子
GET /login?username=admin&password=123 HTTP/1.1
start = "username=admin&password=123"
queryParameters_["username"] = "admin";
queryParameters_["password"] = "123";
*/
void HttpRequest::setQueryParameters(const char *start, const char *end){
    std::string argumentStr(start, end);
    // size_type (size_t)是 std::string 中定义的无符号整数类型，用来表示字符串的长度或者索引。
    std::string::size_type pos = 0;  // 用于记录找到的&的位置
    std::string::size_type prev = 0;  // 用于记录上一次&的后一个字符，即当前参数的起始位置

    while((pos = argumentStr.find('&', prev)) != std::string::npos){
        std::string pair = argumentStr.substr(prev, pos - prev);
        std::string::size_type equalPos = pair.find('=');

        if(equalPos != std::string::npos){
            std::string key = pair.substr(0, equalPos);
            std::string value = pair.substr(equalPos + 1);
            queryParameters_[key] = value;
        }
        prev = pos + 1;  // 更新prev为&的下一个位置
    }

    // 因为最后没有&，所以单独处理最后一个参数
    std::string lastPair = argumentStr.substr(prev);
    std::string::size_type equalPos = lastPair.find('=');
    
    if(equalPos != std::string::npos){
        std::string key = lastPair.substr(0, equalPos);
        std::string value = lastPair.substr(equalPos + 1);
        queryParameters_[key] = value;
    }
}
std::string HttpRequest::getQueryParameters(const std::string &key) const {
    auto it = queryParameters_.find(key);
    if(it != queryParameters_.end()){
        return it->second;
    }
    return "";
}

void HttpRequest::addHeader(const char *start, const char *colon, const char *end){
    // colon是冒号的位置
    std::string key(start, colon);
    ++colon;
    while(colon < end && isspace(*colon)){
        // 跳过空格
        ++colon;
    }

    std::string value(colon, end);
    while(!value.empty() && isspace(value[value.size() - 1])){
        // 跳过结尾空格
        value.resize(value.size() - 1);
    }
    headers_[key] = value;
}
std::string HttpRequest::getHeader(const std::string &field) const {
    auto it = headers_.find(field);
    if(it != headers_.end()){
        return it->second;
    }
    return "";
}

// std::swap 会高效地交换两个变量（指针层次的交换）；
// 应用于多线程场景或容器管理时的资源转移、重用。
void HttpRequest::swap(HttpRequest &that){
    std::swap(method_, that.method_);
    std::swap(path_, that.path_);
    std::swap(pathParameters_, that.pathParameters_);
    std::swap(queryParameters_, that.queryParameters_);
    std::swap(version_, that.version_);
    std::swap(headers_, that.headers_);
    std::swap(receiveTime_, that.receiveTime_);    
}

}  // namespace http