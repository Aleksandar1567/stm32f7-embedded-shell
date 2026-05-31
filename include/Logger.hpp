#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <cstdint>
#include <cstdio>
#include <cstring>

/**
 * @brief Non-blocking circular buffer logger
 * 
 * Stores log messages in a circular buffer. Main thread writes to buffer
 * (doesn't block), and you can read logs anytime via terminal without
 * interrupting the system.
 */
class Logger
{
public:
    static const uint16_t LOG_BUFFER_SIZE = 4096;
    static const uint16_t MAX_LOG_MSG = 256;

    /**
     * @brief Initialize logger
     */
    static void init();

    /**
     * @brief Non-blocking log message (printf-style)
     * Stores message in circular buffer, returns immediately
     */
    static void log(const char* fmt, ...);

    /**
     * @brief Log with ERROR level prefix
     */
    static void error(const char* fmt, ...);

    /**
     * @brief Log with WARN level prefix
     */
    static void warn(const char* fmt, ...);

    /**
     * @brief Log with INFO level prefix
     */
    static void info(const char* fmt, ...);

    /**
     * @brief Log with DEBUG level prefix
     */
    static void debug(const char* fmt, ...);

    /**
     * @brief Dump all pending logs to UART (blocking)
     * Call this when you want to see the logs
     */
    static void flush();

    /**
     * @brief Get current number of unread log bytes
     */
    static uint16_t getUnreadCount();

    /**
     * @brief Clear all logs
     */
    static void clear();

private:
    static char m_buffer[LOG_BUFFER_SIZE];
    static uint16_t m_writePos;
    static uint16_t m_readPos;
    static bool m_bufferFull;

    /**
     * @brief Internal non-blocking write to buffer
     */
    static void _write(const char* data, uint16_t len);

    /**
     * @brief Format log message with timestamp and level
     */
    static void _logWithLevel(const char* level, const char* fmt, va_list args);
};

#endif // LOGGER_HPP
