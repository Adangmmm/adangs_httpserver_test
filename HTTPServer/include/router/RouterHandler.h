#pragma once

#include <string>
#include <memory>

#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

namespace http{
namespace router{

// 抽象基类，用于定义一个通用的处理器接口
class RouterHandler{
public:
    // 确保子类被正确析构，避免内存泄漏  default让编译器自动生成默认析构函数
    virtual ~RouterHandler() = default;
    // 表示一个接口，必须由子类实现
    // 传入引用只读取请求内容，传入指针修改处理响应内容
    virtual void handle(const HttpRequest &req, HttpResponse *resp) = 0;
};

}  // nanmespace router
}  // namespace http