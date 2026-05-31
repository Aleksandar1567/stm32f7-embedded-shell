#include "Shell.hpp"
#include "Uart.hpp"
#include "Filesystem.hpp"
#include "Logger.hpp"
#include <string.h>

extern Uart       uart;
extern Filesystem fs;
extern bool       fs_mounted;

void parse_cmd(char *line)
{
    char out[512];

    if (strcmp(line, "logs") == 0) {
        Logger::flush();
        return;
    }
    if (strcmp(line, "logs clear") == 0) {
        Logger::clear();
        uart.println("Logs cleared");
        return;
    }
    if (strcmp(line, "ls") == 0) {
        if (!fs_mounted) {
            uart.println("ERR: Filesystem not mounted");
            Logger::error("ls: FS not mounted");
            return;
        }
        fs.ls(out, sizeof(out));
        Logger::info("ls output:");
        uart.print(out);
        return;
    }
    if (strncmp(line, "read ", 5) == 0) {
        if (!fs_mounted) {
            uart.println("ERR: Filesystem not mounted");
            Logger::error("read: FS not mounted");
            return;
        }
        if (fs.read(line + 5, out, sizeof(out))) {
            Logger::info("read %s: OK", line + 5);
            uart.println(out);
        } else {
            Logger::error("read: not found: %s", line + 5);
            uart.println("ERR: file not found");
        }
        return;
    }
    if (strncmp(line, "rm ", 3) == 0) {
        if (!fs_mounted) {
            uart.println("ERR: Filesystem not mounted");
            Logger::error("rm: FS not mounted");
            return;
        }
        bool ok = fs.remove(line + 3);
        Logger::info("rm %s: %s", line + 3, ok ? "OK" : "FAIL");
        uart.println(ok ? "OK" : "ERR: cannot remove");
        return;
    }
    if (strncmp(line, "write ", 6) == 0) {
        if (!fs_mounted) {
            uart.println("ERR: Filesystem not mounted");
            Logger::error("write: FS not mounted");
            return;
        }
        char *rest  = line + 6;
        char *space = strchr(rest, ' ');
        if (!space) {
            uart.println("Usage: write <file> <data>");
            return;
        }
        *space = '\0';
        bool ok = fs.write(rest, space + 1);
        Logger::info("write %s: %s", rest, ok ? "OK" : "FAIL");
        uart.println(ok ? "OK" : "ERR: cannot write");
        return;
    }
    if (strcmp(line, "help") == 0) {
        uart.println("Commands:");
        uart.println("  logs              - show all logs");
        uart.println("  logs clear        - clear logs");
        uart.println("  ls                - list files");
        uart.println("  read <file>       - read file");
        uart.println("  write <file> <data>");
        uart.println("  rm <file>         - remove file");
        return;
    }

    Logger::warn("Unknown command: %s", line);
    uart.println("Unknown command. Type 'help'.");
}
