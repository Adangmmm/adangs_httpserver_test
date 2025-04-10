#pragma once

#include <string>
#include <vector>

namespace http{
namespace middleware{

struct CorsConfig{
    // 允许访问的来源域名
    std::vector<std::string> allowOrigins;
    // 允许访问的方法
    std::vector<std::string> allowMethods;
    // 允许客户端带的自定义请求头
    std::vector<std::string> allowHeaders;
    // 是否允许带上 cookie 或 Authorization
    bool allowCredential = false;
    // 预检请求结果缓存的最大时间
    int maxAge = 3600;

    // 静态函数：不需要创建 CorsConfig 对象也可以调用：
    // 例如: auto config = CorsConfig::defaultConfig();
    static CorsConfig defaultConfig(){
        CorsConfig config;
        // 允许所有域访问（"*"）
        config.allowOrigins = {"*"};
        config.allowMethods = {"GET", "POST", "PUT", "DELETE", "OPTIONS"};
        config.allowHeaders = {"Content-Type", "Authorization"};
        // - 默认不允许带 credentials
        // 预检请求缓存时间：1 小时（3600 秒）
        return config;
    }
};


}
}