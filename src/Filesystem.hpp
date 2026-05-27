#pragma once
#include "stm32f7xx_hal.h"
#include "lfs.h"
#include <string.h>

// Sector 11 — 0x081E0000 — 128KB
#define FS_FLASH_ADDR   0x081E0000UL
#define FS_BLOCK_SIZE   (128 * 1024UL)
#define FS_BLOCK_COUNT  1

class Filesystem {
public:
    bool mount() {
        if (lfs_mount(&lfs_, &cfg_) != LFS_ERR_OK) {
            lfs_format(&lfs_, &cfg_);
            return lfs_mount(&lfs_, &cfg_) == LFS_ERR_OK;
        }
        return true;
    }

    void unmount() {
        lfs_unmount(&lfs_);
    }

    // Vraća false ako ne može otvoriti dir
    bool ls(char *out, size_t outlen) {
        lfs_dir_t dir;
        if (lfs_dir_open(&lfs_, &dir, "/") < 0) return false;

        out[0] = '\0';
        struct lfs_info info;
        while (lfs_dir_read(&lfs_, &dir, &info) > 0) {
            char line[64];
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
        memcpy(buf, (void *)(FS_FLASH_ADDR + block * FS_BLOCK_SIZE + off), size);
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
        (void)c; (void)block;
        HAL_FLASH_Unlock();
        FLASH_EraseInitTypeDef e = {0};
        e.TypeErase    = FLASH_TYPEERASE_SECTORS;
        e.Sector       = FLASH_SECTOR_11;
        e.NbSectors    = 1;
        e.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        uint32_t err = 0;
        HAL_StatusTypeDef s = HAL_FLASHEx_Erase(&e, &err);
        HAL_FLASH_Lock();
        return (s == HAL_OK) ? LFS_ERR_OK : LFS_ERR_IO;
    }

    static int s_sync(const struct lfs_config *c) { (void)c; return LFS_ERR_OK; }

    static uint8_t read_buf_[256];
    static uint8_t prog_buf_[256];
    static uint8_t lookahead_buf_[16];

    lfs_t lfs_;
    struct lfs_config cfg_;

public:
    Filesystem() {
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
        cfg_.read_buffer      = read_buf_;
        cfg_.prog_buffer      = prog_buf_;
        cfg_.lookahead_buffer = lookahead_buf_;
    }
};

uint8_t Filesystem::read_buf_[256];
uint8_t Filesystem::prog_buf_[256];
uint8_t Filesystem::lookahead_buf_[16];