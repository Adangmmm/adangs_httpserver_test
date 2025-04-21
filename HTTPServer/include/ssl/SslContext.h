#pragma once

#include <openssl/ssl.h>
#include <memory>
#include <muduo/base/noncopyable.h>

#include "SslConfig.h"

namespace ssl{
// 继承自防止拷贝的类，在管理底层资源如SSL_CTX时非常重要，防止资源重复释放或错误使用
class SslContext : muduo::noncopyable{
public:
    explicit SslContext(const SslConfig &config);
    ~SslContext();

    // 根据配置，初始化SSL_CTX
    bool initialize();
    // 获取底层OpenSSL的SSL_CTX *指针
    // 对外暴露原生句柄的做法常用于封装类
    SSL_CTX *getNativeHandle() const {return ctx_;}

private:
    bool loadCertificates();
    bool setupProtocol();
    void setupSessionCache();
    static void handleSslError(const char *msg);

private:
    // OpenSSL 的 SSL_CTX 结构体，用于存储 SSL 上下文信信息
    // 是整个类的核心资源，所有 SSL 通信都会基于它创建 SSL 实例。
    SSL_CTX *ctx_;
    SslConfig config_;

};


}