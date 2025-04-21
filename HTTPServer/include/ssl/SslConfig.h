#pragma once

#include <vector>
#include <string>

#include "SslTypes.h"

namespace ssl{
class SslConfig{
public:
    SslConfig();
    ~SslConfig() = default;

    // Setters
    void setCertificateFile(const std::string &certFile) {certFile_ = certFile;}
    void setPrivateKeyFile(const std::string &keyFile) {keyFile_ = keyFile;}
    void setCertificateChainFile(const std::string &chainFile) {chainFile_ = chainFile;}

    void setPortocoVersion(SSLVersion version) {version_ = version;}
    void setCipherList(const std::string &cipherList) {cipherList_ = cipherList;}

    void setVerifyClient(bool verify) {verifyClient_ = verify;}
    void setVerifyDepth(int depth) {verifyDepth_ = depth;}

    void setSessionTImeout(int seconds) {sessionTimeout_ = seconds;}
    void setSessionCacheSiZE(long size) {sessionCacheSize_ = size;}

    // Getters
    const std::string &getCertificateFile() const {return certFile_;}
    const std::string &getPrivateKeyFile() const {return keyFile_;}
    const std::string &getCertificateChainFile() const {return chainFile_;}

    SSLVersion getProtocaVersion() const {return version_;}
    const std::string &getCipherList() const {return cipherList_;}

    bool getVerifyClient() const {return verifyClient_;}
    int getVeryfiDepth() const {return verifyDepth_;}

    int getSessionTimeout() const {return sessionTimeout_;}
    long getSessionCacheSize() const {return sessionCacheSize_;}


    // 关于getters中有的返回的是const引用，有的返回的是值
    // 对于重量级对象（比如 std::string、std::vector、自定义类对象）：返回 const 引用（const T&），避免不必要的拷贝，提升性能。同时用 const 限制调用者不能修改返回的内容。
    // 对于轻量级的基础类型（如 bool、int、double）：直接返回值本身（不是引用）因为这些类型本身就是 按值传递成本极低，没必要引用

private:
    std::string certFile_;  // 证书文件
    std::string keyFile_;   // 私钥文件
    std::string chainFile_; // 证书链文件
    SSLVersion version_;    // 协议版本
    std::string cipherList_;    // 加密套件

    bool verifyClient_;     // 是否验证客户端整数
    int verifyDepth_;       // 验证深度
    int sessionTimeout_;    // 会话超时时间
    long sessionCacheSize_; // 会话缓存大小
};

} // namespace ssl