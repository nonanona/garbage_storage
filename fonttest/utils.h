#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string>

uint32_t makeTag(char c1, char c2, char c3, char c4);

uint32_t readU32(const void* ptr, size_t offset);
uint32_t readU24(const void* ptr, size_t offset);
uint16_t readU16(const void* ptr, size_t offset);

uint16_t readS16(const void* ptr, size_t offset);
