#include "log.h"
#include <stdio.h>
#include <stdarg.h>

void log_msg_level(log_level_t level, const char* TAG, const char *format, ...) {
    const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    
    // colorful outputs
    const char *color_codes[] = {"\033[90m", "\033[92m", "\033[93m", "\033[91m"};
    const char *reset_color = "\033[0m";
    
    printf("%s[%s][%s] ", color_codes[level], level_str[level], TAG);
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    printf("%s\r\n", reset_color);
}