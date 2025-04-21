#pragma once

#include <memory>
#include <openssl/ssl.h>
#include <muduo/base/noncopyable.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>

#include "SslContext.h"

namespace ssl{
using MessageCallback = std::function<void(const std::shared_ptr<muduo::net::TcpConnection> &,
                                               muduo::net::Buffer *,
                                               muduo::Timestamp)>;

class SslConnection : muduo::noncopyable{
public:
    using TcpConnectionPtr = std::shared_ptr<muduo::net::tcpConnection>;
    using BufferPtr = muduo::net::Buffer*;

    SslConnection(const TcpConnectionPtr &conn, SslContext *ctx);
    ~SslConnection();

    void startHandshake();
    void send(const void *data, size_t len);    // // 加密并发送数据
    // 来自TCP层的数据进入这里，被写入BIO再交给OpenSSL解密
    void onRead(const TcpConnectionPtr &conn, BufferPtr buf, muduo::Timestamp time);

    bool isHandshakeCompleted() const {return state_ == SSLState::ESTABLISHED;}
    muduo::net::Buffer *getDecryptedBuffer() {return &decryptedBuffer_;}
    void setMessageCallback(const MessageCallback &cb) {messageCallback_ = cb;}

    // SSL BIO回调函数（底层OpenSSL接口）
    // 这些是为了让 OpenSSL 的 BIO 层能读写 Muduo 的 Buffer
    static int bioWrite(BIO *bio, const char *data, int len);
    static int bioRead(BIO *bio, char *data, int len);
    static long bioCtrl(BIO *bio, int cmd, long num, void *ptr);

private:
    void handleHandshake();
    // 处理收到的密文
    void onEncrypted(const char *data, size_t len);
    // 处理解密的明文
    void onDecrypted(const char *data, size_t len);
    SSLError getLastError(int ret);
    void handleError(SSLError error);


private:
    SSL *ssl_;        // ssl连接
    SslContext *ctx_;  // ssl上下文
    TcpConnectionPtr conn_; // tcp连接
    SSLState state_;
    BIO *readBio_;
    BIO *writeBio_;
    muduo::net::Buffer readBuffer_;
    muduo::net::Buffer writeBuffer_;
    // 已被解密的数据缓冲区
    muduo::net::Buffer decryptedBuffer_;
    MessageCallback messageCallback_;

};


}