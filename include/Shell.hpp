#pragma once

/**
 * @brief Parse and execute a shell command received over UART.
 * @param line Null-terminated command string (may be modified in-place).
 */
void parse_cmd(char *line);
