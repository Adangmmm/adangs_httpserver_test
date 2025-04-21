#include "../../include/ssl/SslConfig.h"

namespace ssl{
SslConfig::SslConfig()
    : version_(SSLVersion::TLS_1_2)
    // 这个字符串是 OpenSSL 中用于指定**加密套件（Cipher Suites）**的语法。
    /*"
    HIGH"：表示优先选择强加密算法（例如 AES-256、ChaCha20 等），这个是 OpenSSL 内置的一个套件组别。
    "!aNULL"：排除所有没有身份认证的加密套件（anonymous NULL authentication）。这些是不安全的（例如匿名 Diffie-Hellman）。
    "!MDS"：排除使用 MDS（如 MD5）的加密套件，MD5 已被认为是不安全的哈希算法。
    */
    // "HIGH:!aNULL:!MDS"： 表示只允许使用强加密，排除匿名认证和弱哈希算法 的套件。
    , cipherList_("HIGH:!aNULL:!MDS")
    , verifyClient_(false)
    , verifyDepth_(4)
    , sessionTimeout_(300)
    // 指的是 SSL 会话缓存中最多能存储多少个会话条目。单位是“个条目”，而不是字节。
    // 启用会话缓存机制，默认缓存最多 20480 个 SSL 会话，以提升性能。
    , sessionCacheSize_(20480L)
{
}

}