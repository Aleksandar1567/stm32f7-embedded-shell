#pragma once
#include "stm32f7xx_hal.h"
#include "lfs.h"
#include "Logger.hpp"
#include <string.h>

// Uncomment to enable real-time UART output during FS mount/format.
// Useful when debugging early-boot crashes where the Logger buffer is lost on reset.
// When commented out, only the Logger (circular buffer) is used.
// #define FS_VERBOSE_UART

#ifdef FS_VERBOSE_UART
#  include "Uart.hpp"
   extern Uart uart;
#  define FS_VPRINT(fmt, ...)  uart.printf(fmt "\r\n", ##__VA_ARGS__)
#else
#  define FS_VPRINT(fmt, ...)  do {} while(0)
#endif

// Sector 6: 0x08060000 - 0x0807FFFF (128 KB)
// Sector 7: 0x08080000 - 0x0809FFFF (128 KB)
// Each sector = one LittleFS block (must be >= physical erase unit)
#define FS_FLASH_ADDR   0x08060000UL
#define FS_BLOCK_SIZE   (128UL * 1024UL)   // 128 KB = one physical sector
#define FS_BLOCK_COUNT  2                  // Sectors 6 and 7

class Filesystem {
public:
    bool mount() {
        Logger::debug("FS mount: addr=0x%08lX blk_sz=%lu blk_cnt=%lu",
                      (unsigned long)FS_FLASH_ADDR,
                      (unsigned long)FS_BLOCK_SIZE,
                      (unsigned long)FS_BLOCK_COUNT);
        FS_VPRINT("FS> addr=0x%08lX blk_sz=%lu blk_cnt=%lu",
                  (unsigned long)FS_FLASH_ADDR,
                  (unsigned long)FS_BLOCK_SIZE,
                  (unsigned long)FS_BLOCK_COUNT);

        FS_VPRINT("FS> calling lfs_mount...");
        int err = lfs_mount(&lfs_, &cfg_);
        Logger::debug("FS lfs_mount: %d", err);
        FS_VPRINT("FS> lfs_mount returned: %d", err);

        if (err == LFS_ERR_OK) {
            Logger::info("FS mount: OK");
            FS_VPRINT("FS> mount OK");
            return true;
        }

        Logger::warn("FS mount failed (%d), formatting...", err);
        FS_VPRINT("FS> mount failed (%d), calling lfs_format...", err);
        int fmt = lfs_format(&lfs_, &cfg_);
        Logger::debug("FS lfs_format: %d", fmt);
        FS_VPRINT("FS> lfs_format returned: %d", fmt);

        if (fmt != LFS_ERR_OK) {
            Logger::error("FS format FAILED (%d)", fmt);
            FS_VPRINT("FS> format FAILED (%d)", fmt);
            return false;
        }

        FS_VPRINT("FS> format OK, calling lfs_mount again...");
        int err2 = lfs_mount(&lfs_, &cfg_);
        Logger::debug("FS lfs_mount (after format): %d", err2);
        FS_VPRINT("FS> lfs_mount (after format) returned: %d", err2);

        if (err2 == LFS_ERR_OK) {
            Logger::info("FS mount: OK (after format)");
            FS_VPRINT("FS> mount OK (after format)");
            return true;
        }

        Logger::error("FS mount after format FAILED (%d)", err2);
        FS_VPRINT("FS> mount after format FAILED (%d)", err2);
        return false;
    }

    void unmount() {
        lfs_unmount(&lfs_);
    }

    // Returns false if the root directory cannot be opened
    bool ls(char *out, size_t outlen) {
        lfs_dir_t dir;
        if (lfs_dir_open(&lfs_, &dir, "/") < 0) return false;

        out[0] = '\0';
        struct lfs_info info;
        while (lfs_dir_read(&lfs_, &dir, &info) > 0) {
            // LFS_NAME_MAX is 255, so line must be large enough to hold the full name
            char line[280];
            if (info.type == LFS_TYPE_DIR)
                snprintf(line, sizeof(line), "[DIR]  %s\r\n", info.name);
            else
                snprintf(line, sizeof(line), "[FILE] %-20s %lu B\r\n",
                         info.name, (unsigned long)info.size);
            strncat(out, line, outlen - strlen(out) - 1);
        }
        if (out[0] == '\0') strncat(out, "(empty)\r\n", outlen - 1);
        lfs_dir_close(&lfs_, &dir);
        return true;
    }

    bool write(const char *path, const char *data) {
        lfs_file_t file;
        if (lfs_file_open(&lfs_, &file, path,
                          LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) < 0)
            return false;
        lfs_file_write(&lfs_, &file, data, strlen(data));
        lfs_file_close(&lfs_, &file);
        return true;
    }

    bool read(const char *path, char *out, size_t outlen) {
        lfs_file_t file;
        if (lfs_file_open(&lfs_, &file, path, LFS_O_RDONLY) < 0)
            return false;
        int n = lfs_file_read(&lfs_, &file, out, outlen - 1);
        lfs_file_close(&lfs_, &file);
        if (n < 0) return false;
        out[n] = '\0';
        return true;
    }

    bool remove(const char *path) {
        return lfs_remove(&lfs_, path) == LFS_ERR_OK;
    }

private:
    static int s_read(const struct lfs_config *c,
                      lfs_block_t block, lfs_off_t off,
                      void *buf, lfs_size_t size)
    {
        (void)c;
        uint32_t addr = FS_FLASH_ADDR + block * FS_BLOCK_SIZE + off;
        memcpy(buf, (void *)addr, size);
        return LFS_ERR_OK;
    }

    static int s_prog(const struct lfs_config *c,
                  lfs_block_t block, lfs_off_t off,
                  const void *buf, lfs_size_t size)
    {
        (void)c;
        uint32_t addr = FS_FLASH_ADDR + block * FS_BLOCK_SIZE + off;
        HAL_FLASH_Unlock();
        for (uint32_t i = 0; i < size; i += 4) {
            uint32_t word;
            memcpy(&word, (const uint8_t *)buf + i, 4);
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word) != HAL_OK) {
                HAL_FLASH_Lock();
                return LFS_ERR_IO;
            }
        }
        HAL_FLASH_Lock();
        return LFS_ERR_OK;
    }

    static int s_erase(const struct lfs_config *c, lfs_block_t block)
    {
        (void)c;
        // block 0 -> FLASH_SECTOR_6, block 1 -> FLASH_SECTOR_7
        Logger::debug("FS erase: block %lu (sector %lu)", (unsigned long)block, (unsigned long)(FLASH_SECTOR_6 + block));
        HAL_FLASH_Unlock();
        FLASH_EraseInitTypeDef e = {0};
        e.TypeErase    = FLASH_TYPEERASE_SECTORS;
        e.Sector       = FLASH_SECTOR_6 + block;
        e.NbSectors    = 1;
        e.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        
        uint32_t err_sector = 0;
        HAL_StatusTypeDef s = HAL_FLASHEx_Erase(&e, &err_sector);
        HAL_FLASH_Lock();
        
        if (s != HAL_OK) {
            Logger::error("FS erase FAILED: block %lu, err_sector=%lu", (unsigned long)block, (unsigned long)err_sector);
            return LFS_ERR_IO;
        }
        return LFS_ERR_OK;
    }

    static int s_sync(const struct lfs_config *c) { (void)c; return LFS_ERR_OK; }

    static uint8_t read_buf_[256];
    static uint8_t prog_buf_[256];
    static uint8_t lookahead_buf_[16];

    lfs_t lfs_;
    struct lfs_config cfg_;

public:
    Filesystem() {
        memset(&lfs_, 0, sizeof(lfs_));
        memset(&cfg_, 0, sizeof(cfg_));
        cfg_.read             = s_read;
        cfg_.prog             = s_prog;
        cfg_.erase            = s_erase;
        cfg_.sync             = s_sync;
        cfg_.read_size        = 256;
        cfg_.prog_size        = 256;
        cfg_.block_size       = FS_BLOCK_SIZE;
        cfg_.block_count      = FS_BLOCK_COUNT;
        cfg_.cache_size       = 256;
        cfg_.lookahead_size   = 16;
        cfg_.block_cycles     = -1;   // disable wear-leveling (required, 0 is invalid)
        cfg_.read_buffer      = read_buf_;
        cfg_.prog_buffer      = prog_buf_;
        cfg_.lookahead_buffer = lookahead_buf_;
    }
};