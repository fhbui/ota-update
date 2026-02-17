#ifndef __LOG_H
#define __LOG_H

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

#define SHOW_DEBUG_LOG		1

#if SHOW_DEBUG_LOG==1
#define LOG_DEBUG(tag, ...) log_msg_level(LOG_DEBUG, tag, __VA_ARGS__)
#endif

#define LOG_INFO(tag, ...)  log_msg_level(LOG_INFO, tag, __VA_ARGS__)
#define LOG_WARN(tag, ...)  log_msg_level(LOG_WARN, tag, __VA_ARGS__)
#define LOG_ERROR(tag, ...) log_msg_level(LOG_ERROR, tag, __VA_ARGS__)

#endif