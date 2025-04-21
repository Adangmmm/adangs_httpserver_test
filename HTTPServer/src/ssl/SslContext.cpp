#include <openssl/err.h>
#include <muduo/base/Logging.h>

#include "../../include/ssl/SslContext.h"

namespace ssl{
SslContext::SslContext(const SslConfig &config)
    : ctx_(nullptr)
    , config_(config)
{
}

SslContext::~SslConttext()
{
    if(ctx_){
        SSL_CTX_free(ctx_);
    }
}

bool SslContext::initialize(){
    // 初始化 OpenSSL全局环境
    OPENSSL_init_ssl(OPENSSL_INIT_LAOD_SSL_STRINGS | 
                    OPENSSL_INIT_LAOD_CRYPTO_STRINGS, nullptr);
    // 创建一个基于通用TLS协议的上下文， TLS_server_method()支持TLS1.0-1.3,具体限制通过后面设置
    const SSL_METHOD *method = TLS_server_method();
    ctx_ = SSL_CTX_new(method);
    if(!ctx_){
        handleSslError("Failed to create SSL context");
        return false;
    }

    // 设置SSL选项: 禁用SSLV2/V3; 禁用压缩; 优先使用服务器指定的加密套件顺序
    // | 为位运算符，用来组合多个宏定义的标志位，下面选项其实是以位掩码的方式定义的
    // 如#define SSL_OP_NO_SSLv2 0x01000000L
    long options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | 
                   SSL_OP_NO_COMPRESSION | 
                   SSL_OP_CIPHER_SEVER_PREFERENCE;
    SSL_CTX_set_options(ctx_, options);

    // 初始化流程三步骤
    
    // 加载证书和密钥
    if(!loadCertificates()){
        return false;
    }
    // 设置协议版本
    if(!setupProtocol()){
        return false;
    }
    // 设置会话缓存
    setupSessionCache();

    LOGINFO << "SSL context initialized successfully";
    return true;
}

bool SslContext::loadCertificates(){
    // 加载证书
    if(SSL_CTX_use_certificate_file(ctx_, config_.getCertificateFile().c_str(),
        SSL_FILETYPE_PEM) <= 0){
        handleSslError("Failed to load server certificate");
        return false;
    }

    // 加载服务器证书（.crt）
    if(SSL_CTX_use_PrivateKey_file(ctx_, config_.getPrivateKeyFile().c_str(),
        SSL_FILETYPE_PEM) <= 0){
        handleSslError("Failed to load Private key");
        return false;
    }

    // 验证密钥（.key）
    if(!SSL_CTX_check_private_key(ctx_)){
        handleSslError("Private key does not match the certificate");
        return false;
    }

    // 加载证书链
    if(!config_.getCertificateChainFile().empty()){
        if(SSL_CTX_use_certificate_chain_file(ctx_, config_.getCertificateChainFile().c_str()) <= 0){
            handleError("Failed to load certificate chain");
            return false;
        }
    }

    return true;
}

bool SslContext::setupProtocol(){
    // 设置协议版本
    long options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
    switch(config_.getProtocolVersion()){
        case SSLVersion::TLS_1_0:
            options |= SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2 | SSL_OP_NO_TLSv1_3;
            break;
        case SSLVersion::TLS_1_1:
            options |= SSL_OP_NO_TLSv1_0 | SSL_OP_NO_TLSv1_2 | SSL_OP_NO_TLSv1_3;
            break;
        case SSLVersion::TLS_1_2:
            options |= SSL_OP_NO_TLSv1_0 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_3;
            break;
        case SSLVersion::TLS_1_3:
            options |= SSL_OP_NO_TLSv1_0 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2;
            break;
    }
    SSL_CTX_set_options(ctx_, options);

    // 设置加密套件
    if(!config_.getCipherList().empty()){
        if(SSL_CTX_set_cipher_list(ctx_, config_.getCipherList().c_str()) <= 0){
            handleSslError("Failed to set cipher list");
            return false;
        }
    }

    return true;
}

void SslContext::setupSSessionCache(){
    // 开启服务端会话缓存
    SSL_CTTX_set_session_cache_mode(ctx_, SSL__SESS_CACHE_SERVER);
    SSL_CTX_sess_set_cache_size(ctx_, config_.getSessionCacheSize());
    SSL_CTX_set_timeout(ctx_, config_.getSessionTimeout());
}

void SslContext::handleSslError(const char *msg){
    char buf[256];
    // ERR_error_string_n()将错误码转化为可读的字符串
    // ERR_get_error()从 OpenSSL 的错误队列中取出一个错误码（unsigned long 类型）。
    // OpenSSL 的错误是按“栈”的形式储存的，每次调用 ERR_get_error() 都会把最上面的错误弹出来。
    ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
    LOG_ERROR << msg << ": " << buf;
}

}