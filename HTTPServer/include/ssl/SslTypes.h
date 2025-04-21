#pragma once

#include <string>

namespace ssl{

// 强类型枚举（C++11 引入），相比普通 enum：
//                          类型更安全（不能隐式转换为 int）
//                          更清晰的作用域（要用 SSLVersion::TLS_1_2 这样访问）

// 协议版本
enum class SSLVersion{
    TLS_1_0,
    TLS_1_1,
    TLS_1_2,
    TLS_1_3
};

enum class SSLError{
    NONE,
    WANT_READ,
    WABT_WTITE,
    SYSCALL,
    SSL,
    UNKNOWN
};

enum class SSLState{
    HANDSHAKE,
    ESTABLISHED,
    SHUTDOWN,
    ERROR
};

}