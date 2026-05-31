#include "Filesystem.hpp"

// Static buffer definitions — exactly one translation unit
uint8_t Filesystem::read_buf_[256];
uint8_t Filesystem::prog_buf_[256];
uint8_t Filesystem::lookahead_buf_[16];
