#include <muduo/base/Logging.h>
#include <openssl/err.h>

#include "../../include/ssl/SslConnection.h"

namespace ssl{

// 自定义 BIO 方法  250421看不懂
// 本函数没被调用，可能是为后续扩展（比如非内存 BIO）预留的
static BIOMETHOD *createCustomBioMethod(){
    BIO_METHOD *method = BIO_meth_new(BIO_TYPE_MEM, "custom");
    BIO_meth_set_write(method, SslConnection::bioWrite);
    BIO_meth_set_read(method, SslConnection::bioRead);
    BIO_meth_set_ctrl(method, SslConnection::bioCtrl);
    return method;
}

SslConnection::SslConnection(const TcpConnectionPtr &conn, SslContext *ctx)
    : ssl_(nullptr)
    , ctx_(ctx)
    , conn_(conn)
    , state_(SSLState::HANDSHAKE)
    , readBio_(nullptr)
    , writeBio_(nullptr)
    , messageCallback_(nullptr)
{
    // 创建SSL实例
    ssl_ = SSL_new(ctx_->getNativeHandle());
    if(!ssl_){
        LOG_ERROR << "Failed to create SSL object: " << ERR_error_string(Error_get_error(), nullptr);
        return;
    }

    // 创建两个内存BIO
    readBio_ = BIO_new(BIO_s_mem());
    writeBiio_ = BIO_new(BIO_s_mem());

    if(!readBio_ || !writeBio_){
        LOG_ERROR << "Failed to creadt BIO objects";
        SSL_free(ssl_);
        ssl_ = nullptr;
        return;
    }
    // 设置BIO，readBio_用于接收TCP层来的密文，writeBio_用于传输加密后的数据，等待发送
    SSL_set_bio(ssl_, readBio_, writrBio_);
    SSL_set_accept_state(ssl_); // 设置为服务器模式（等待客户端握手）

    // 设置SSL选项
    SSL_set_mode(ssl_, SSL_MODE_ACCEPT_MOBING_WRITE_BUFFER);
    SSK_set_mode(ssl_, SSL_MODE_ENABLE_PARTIAL_WRITE);

    // 设置连接回调    
    conn_->setMessageCallback(
        std::bind(&SslConnection::onRead, this, std::placeholders::_1,
            std::placeholders::_2, std::placeholders::_3)
    );
    
}

SslConnection::~SslConnection(){
    if(ssl_){
        SSL_free(ssl_); // 会同时释放BIO
    }
}

void SslConection::startHandshake(){
    SSL_set_accept_state(ssl_);
    handleHandshake();
}

// const void* data 表示 “接受任意类型的原始内存数据”，这是为了让 send() 函数具备更强的通用性
// void* 是 C/C++ 中的泛型指针，它代表“一块内存地址”，但是没有指定类型。
void SslConnection::send(const void *data, size_t len){
    // 握手未完成之前无法发送数据
    if(state_ != SSLState::ESTABLISHED){
        LOG_ERROR << "Cannot send data before SSL handshake is complete";
        return;
    }

    // SSL_write()会把明文写入SSL的加密层
    int written = SSLwrite(ssl_, data, len);
    if(written <= 0){
        int err = SSL_get_error(ssl_, written);
        LOG_ERROR << "SSL_write failed: " << ERROR_error_string(err, nullptr);
        return;
    }

    // 通过BIO_read(writeBio_) 取出加密好的密文 通过 conn_->send()发送出去
    char buf[4096];
    int pending;
    // IO_pending() 是 OpenSSL 中的一个函数，用于检查 BIO 中还有多少“可读取”的字节数据。
    while((pending = BIO_pending(writeBio_)) >0 ){
        int bytes = BIO_read(writeBio_, buf, std::min(pending, static_cast<int>(sizeof(buf))));
        if(bytes > 0){
            conn_->send(buf, bytes);
        }
    }
}

void SslConnection::onRead(const TcpConnectionPtr &conn, BufferPtr buf, muduo::Timestamp time){
    if(state_ == SSLState::HANDSHAKE){
        // 还在握手就把数据写到readBio_中，继续推进SSL_do_handshake()
        BIO_write(readBio_, buf->peek(), buf->readableBytes());
        buf->retrieve(buf->readableBytes());
        handleHanshake();
        return;
    }else if(state_ == SSLState::ESTABLISHED){
        // 解密数据
        char decryptedData[4096];
        // SSL_read()解密出明文数据，返回解密的字节数
        int ret = SSL_read(ssl_, decryptedData, sizeof(decryptedData));
        if(ret > 0){
            // 创建一个新的Buffer存储解密后的数据
            muduo::net::Buffer decryptedBuffer;
            decryptedBuffer.append(decryptedData, ret);

            // 调用用户设置的回调函数处理解密后的数据
            if(messageCallback_){
                messageCallback_(conn, &decryptedBuffer, time);
            }
        }
    }
}


void SslConnection::handleHandshake(){
    // 进行SSL握手，返回1表示成功
    int ret = SSL_do_handshake(ssl_);

    if(ret == 1){
        state_ = SSLState::ESTABLISHED;
        LOG_INFO << "SSL handshake completed successfully";
        LOG_INFO << "Using cipher: " << SSL_get_cipher(ssl_);
        LOG_INFO << "Protocol version :" << SSL_get_version(ssl_);

        // 握手完成后，确保设置了正确的回调
        if(!messageCallback_){
            LOG_WARN << "No message callback set after SSL hanshake";
        }
        return;
    }

    int err = SSL_get_error(ssl_, ret);
    switch(err){
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            break; // 等待更多数据

        default:{
            // 获取错误信息
            char errBuf[256];
            unsigned long errCode = ERR_get_error();
            ERR_error_string_n(errCode, errBuf, sizeof(errBuf));
            LOG_ERROR << "SSL handshake failed: " << errBuf;
            conn_->shutdown();
            break; 
        }
    }

}

void SslConnection::onEncrypted(const char *data, size_t len){
    writeBuffer_.append(data, len);
    conn_->send(&witeBuffer_);
}

void SslConnection::onDecrypted(const char *data, size_t len){
    decryptedBuffer_.append(data, len);
}

SSLError SslConnection::getLastError(int ret){
    int err = SSL_get_error(ssl_, ret);
    switch(err){
        case SSL_ERROR_NONE:
            return SSLError::NONE;
        case SSL_ERROR_WANT_READ:
            return SSLError::WANT_READ;
        case SSL_ERROR_WANT_WRITE:
            return SSLError::WANT_WRITE;
        case SSL_ERROR_SYSCALL:
            return SSLError::SYSCALL;
        case SSL_ERROR_SSL:
            return SSLError::SSL;
        default:
            return SSLError::UNKOWN;
    }
}

void SslConnection::handleError(SSLError error){
    switch(error){
        case SSLError::WANT_READ:
        case SSLError::WANT_WRITE:
            // 需要等待更多数据
            break;
        case SSLError::SSL:
        case SSLError::SYSCALL:
        case SSLError::UNKNOWN:
            LOG_ERROR << "SSL error occurred: " << ERR_error_string(ERR_get_error());
            state_ = SSLState::ERROR;
            conn_->shutdown();
            break;
        default:
            break;
    }
}

int SslConnection::bioWrite(BIO *bio, const char *data, int len){
    SslConnection *conn = static_cast<SslConnection *>(BIO_get_data(bio));
    if(!conn) return -1;

    conn->conn_->send(data, len);
    return len;
}

int SslConnection::bioRead(BIO *bio, const char *data, int len){
    SslConection *conn = static_cast<SslConnecion *>(NIO_get_data(bio));
    if(!conn) return -1;

    size_t readable = conn->readBuffer_.readableBytes();
    if(readable == 0){
        return -1;
    }

    size_t toRead = std::min(static_cast<size_t>(len), readable);
    memcpy(data, conn->readBuffer_.peek(), toRead);
    conn->readBuffer_.retrieve(toRead);
    return toRead;
}

long SslConnection::bioCtrl(BIO *bio, int cmd, long num, void *ptr){
    switch(cmd){
        case BIO_CTRL_FLUSH:
            return 1;
        default:
            return 0;
    }
}

}