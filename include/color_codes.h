#ifndef COLOR_CODES_H
#define COLOR_CODES_H

// ANSI 颜色代码 inline用于避免多次定义和链接错误，const char* 用于字符串常量
namespace Color
{
    inline const char *RESET = "\033[0m";
    inline const char *RED = "\033[31m";
    inline const char *GREEN = "\033[32m";
    inline const char *YELLOW = "\033[33m";
    inline const char *CYAN = "\033[36m";
}

#endif // COLOR_CODES_H
