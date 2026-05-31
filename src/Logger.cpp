#include "Logger.hpp"
#include "Uart.hpp"
#include "stm32f7xx_hal.h"
#include <cstdarg>

extern Uart uart;

// Static member initialization
char Logger::m_buffer[Logger::LOG_BUFFER_SIZE] = {0};
uint16_t Logger::m_writePos = 0;
uint16_t Logger::m_readPos = 0;
bool Logger::m_bufferFull = false;

void Logger::init()
{
    m_writePos = 0;
    m_readPos = 0;
    m_bufferFull = false;
    memset(m_buffer, 0, LOG_BUFFER_SIZE);
}

void Logger::_write(const char* data, uint16_t len)
{
    if (!data || len == 0) return;

    // If this write would overflow, wrap around
    if (m_writePos + len >= LOG_BUFFER_SIZE)
    {
        uint16_t firstPart = LOG_BUFFER_SIZE - m_writePos;
        if (firstPart > 0)
        {
            memcpy(&m_buffer[m_writePos], data, firstPart);
        }
        uint16_t secondPart = len - firstPart;
        memcpy(m_buffer, data + firstPart, secondPart);
        m_writePos = secondPart;
        m_bufferFull = true;
    }
    else
    {
        memcpy(&m_buffer[m_writePos], data, len);
        m_writePos += len;
    }
}

void Logger::_logWithLevel(const char* level, const char* fmt, va_list args)
{
    char msgBuffer[MAX_LOG_MSG];
    
    uint32_t timestamp = HAL_GetTick();
    int len = snprintf(msgBuffer, MAX_LOG_MSG, "[%s][%lu] ", level, timestamp);
    
    if (len > 0)
    {
        len += vsnprintf(msgBuffer + len, MAX_LOG_MSG - len, fmt, args);
        len += snprintf(msgBuffer + len, MAX_LOG_MSG - len, "\r\n");
        
        if (len > MAX_LOG_MSG) len = MAX_LOG_MSG;
        _write(msgBuffer, len);
    }
}

void Logger::log(const char* fmt, ...)
{
    if (!fmt) return;
    
    char msgBuffer[MAX_LOG_MSG];
    va_list args;
    va_start(args, fmt);
    
    int len = vsnprintf(msgBuffer, MAX_LOG_MSG, fmt, args);
    len += snprintf(msgBuffer + len, MAX_LOG_MSG - len, "\r\n");
    
    if (len > MAX_LOG_MSG) len = MAX_LOG_MSG;
    _write(msgBuffer, len);
    
    va_end(args);
}

void Logger::error(const char* fmt, ...)
{
    if (!fmt) return;
    
    va_list args;
    va_start(args, fmt);
    _logWithLevel("ERR", fmt, args);
    va_end(args);
}

void Logger::warn(const char* fmt, ...)
{
    if (!fmt) return;
    
    va_list args;
    va_start(args, fmt);
    _logWithLevel("WRN", fmt, args);
    va_end(args);
}

void Logger::info(const char* fmt, ...)
{
    if (!fmt) return;
    
    va_list args;
    va_start(args, fmt);
    _logWithLevel("INF", fmt, args);
    va_end(args);
}

void Logger::debug(const char* fmt, ...)
{
    if (!fmt) return;
    
    va_list args;
    va_start(args, fmt);
    _logWithLevel("DBG", fmt, args);
    va_end(args);
}

void Logger::flush()
{
    uint16_t count = getUnreadCount();
    
    if (count == 0)
    {
        uart.println("--- No logs ---");
        return;
    }
    
    uart.println("\r\n=== LOGS ===");
    
    char tempBuf[256];
    uint16_t tempLen = 0;
    
    if (!m_bufferFull)
    {
        // Normal case: data is between readPos and writePos
        if (m_readPos < m_writePos)
        {
            for (uint16_t i = m_readPos; i < m_writePos; i++)
            {
                tempBuf[tempLen++] = m_buffer[i];
                if (tempLen >= sizeof(tempBuf) - 1)
                {
                    tempBuf[tempLen] = '\0';
                    uart.print(tempBuf);
                    tempLen = 0;
                }
            }
        }
        m_readPos = m_writePos;
    }
    else
    {
        // Buffer wrapped: read from readPos to end, then from start to writePos
        for (uint16_t i = m_readPos; i < LOG_BUFFER_SIZE; i++)
        {
            tempBuf[tempLen++] = m_buffer[i];
            if (tempLen >= sizeof(tempBuf) - 1)
            {
                tempBuf[tempLen] = '\0';
                uart.print(tempBuf);
                tempLen = 0;
            }
        }
        
        for (uint16_t i = 0; i < m_writePos; i++)
        {
            tempBuf[tempLen++] = m_buffer[i];
            if (tempLen >= sizeof(tempBuf) - 1)
            {
                tempBuf[tempLen] = '\0';
                uart.print(tempBuf);
                tempLen = 0;
            }
        }
        
        m_readPos = m_writePos;
        m_bufferFull = false;
    }
    
    // Flush remaining data
    if (tempLen > 0)
    {
        tempBuf[tempLen] = '\0';
        uart.print(tempBuf);
    }
    
    uart.println("\r\n=== END ===\r\n");
}

uint16_t Logger::getUnreadCount()
{
    if (m_bufferFull)
        return LOG_BUFFER_SIZE;
    
    if (m_writePos >= m_readPos)
        return m_writePos - m_readPos;
    else
        return LOG_BUFFER_SIZE - m_readPos + m_writePos;
}

void Logger::clear()
{
    m_writePos = 0;
    m_readPos = 0;
    m_bufferFull = false;
    memset(m_buffer, 0, LOG_BUFFER_SIZE);
}
