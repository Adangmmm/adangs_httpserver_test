#pragma once

#include "../../http/HttpRequest.h"
#include "../../http/HttpResponse.h"
#include "../Middleware.h"
#include  "CorsConfig.h"

namespace http{
namespace middleware{

class CorsMiddleware : public Middleware{
public:
    // explicit：防止隐式转换。
    // 接收一个 CorsConfig 对象，默认使用 defaultConfig()。用于初始化中间件的 CORS 策略。
    explicit CorsMiddleware(const CorsConfig & config == CorsConfig::defaultConfig());

    // 在请求到达主处理逻辑之前执行 → 可用于拦截预检请求。
    void before(HttpRequest &request) override;
    // 在响应返回客户端之前执行 → 添加 CORS 响应头。
    void after(HttpResponse &response) override;

    // 工具函数：字符串拼接  将字符串列表按某个分隔符拼接起来，比如把多个允许的 Header 合成一行用 , 分隔。
    // 例如 join({"GET", "POST", "OPTIONS"}, ", ")
    // 结果："GET, POST, OPTIONS"
    std::string join(const std::vector<std::string> &strings, cosnt std::string &delimiter);

private:
    // 检查请求头中的 Origin 是否包含在 config_.allowedOrigins 中。
    bool isOriginAllowed(const std::string &origin) const;
    // 处理预检请求(OPTIONS) 如果是OPTIONS， 构造一个响应并加上必要的CORS头，提前返回响应，避免进入主业务逻辑
    void handlePreflightRequest(const HttpRequest &request, HttpResponse &response);
    // 添加响应头
    void addCorsHeaders(HttpResponse &response, const std::string &origin);

private:
    // 存储本实例的 CORS 策略配置，用于判断和生成头部。
    CorsConfig config_;

};


}
}