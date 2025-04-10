#include <algorithm> // 使用 STL 算法，如 std::find 来查找特定元素。
#include <sstream>  //用于构建返回的拼接字符串。
#include <iostream>

#include "../../../include/middleware/cors/CorsMiddleware.h"
#include <muduo/base/Logging.h>

namespace http{
namespace middleware{

CorsMiddleware::CorsMiddleware(const CorsConfig &config) : config_(config) {}

void CorsMiddleware::before(HttpRequest &request){
    LOG_DEBUG << "CorsMiddleware::before - Processing request";

    // 如果是预检请求，则调用handlePreflightRequest 处理预检请求，
    // 然后通过 throw response 抛出响应，避免进入后续的处理流程。
    if(request.method() == HttpRequest::Method::kOptions){
        LOG_INFO << "Processing CORS preflight request";
        HttpResponse response;
        handlePreflightRequest(request, response);
        throw response;
    }
}

void CorsMiddleware::after(HttpResponse &response){
    LOG_DEBUG << "CoreMiddleware::after - Processing response";

    if(!config_.allowOrigins.empty()){
        // 如果允许所有源
        if(std::find(config_.allowOrigins.begin(), config_.allowOrigins.end(), "*") != config_.allowOrigins.end()){
            addCorsHeaders(response, "*");
        }
        else{
            // 添加第一个允许的源
            addCorsHeaders(response, config_.allowOrigins[0]);
        }
    }
}

bool CorsMiddleware::isOriginAllowed(const std::string &origin) const{
    // 若没有设置允许来源（allowedOrigins 为空），则视为允许所有源
    return config_.allowOrigins.empty() ||
           std::find(config_.allowOrigins.begin(), config_.allowOrigins.end(), "*") != config_.allowOrigins.end() ||
           std::find(config_.allowOrigins.begin(), config_.allowOrigins.end(), origin) != config_.allowOrigins.end();
}

void CorsMiddleware::handlePreflightRequest(const HttpRequest &request, HttpResponse &response){
    // 不修改源请求
    const std::string &origin = request.getHeader("Origin");

    // 源不在允许范围内
    if(!isOriginAllowed(origin)){
        LOG_WARN << "Origin not allowed: " << origin;
        // 返回403
        response.setStatusCode(HttpResponse::k403Forbidden);
        return;
    }
    
    addCorsHeaders(response, origin);
    response.setStatusCode(HttpResponse::k204NoContent);
    LOG_INFO << "Preflight request processed successfully";
}

void CorsMiddleware::addCorsHeaders(HttpResponse &response, const std::string &origin){
    try{
        response.addHeader("Access-Control-Allow-Origin", origin);

        if(config_.allowCredential){
            response.addHeader("Access-Control-Allow-Credentials", "true");
        }

        if(!config_.allowMethods.empty()){
            response.addHeader("Access-Control-Allow-Methods", join(config_.allowMethods, ","));
        }

        if(!config_.allowHeaders,empty()){
            response.addHeader("Access-Control-Allow-Headers", join(config_.allowHeaders), ",");
        }

        response.addHeader("Access-Control-Max-Age", std::to_string(config_.maxAge));

        LOG_DEBUG << "CORS heades added succesfully";
    }
    catch(cosnt std::exception &e){
        LOG_ERROR << "Eroor adding CORS headers" << e.what();
    }
}

std::string CorsMiddleware::join(const std::vector<std::string> &strings, const std::string &delimiter){
    std::ostringstream result;
    for(size_t i = 0; i < strings.size(); ++i){
        if(i > 0) result << delimiter;
        result << strings[i];
    }
    return result.str();
}

}
}