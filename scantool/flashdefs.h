#ifndef _FLASHDEFS_H
#define _FLASHDEFS_H

/* (c) fenugrec 2016
 * GPLv3
 *
 * defs for SH705x flash block areas
 */

#include <stdint.h>

struct flashblock {
	uint32_t start;
	uint32_t len;
};

const struct flashblock fblocks_7058[] = {
	{0x00000000,	0x00001000},
	{0x00001000,	0x00001000},
	{0x00002000,	0x00001000},
	{0x00003000,	0x00001000},
	{0x00004000,	0x00001000},
	{0x00005000,	0x00001000},
	{0x00006000,	0x00001000},
	{0x00007000,	0x00001000},
	{0x00008000,	0x00018000},
	{0x00020000,	0x00020000},
	{0x00040000,	0x00020000},
	{0x00060000,	0x00020000},
	{0x00080000,	0x00020000},
	{0x000A0000,	0x00020000},
	{0x000C0000,	0x00020000},
	{0x000E0000,	0x00020000},
};

#endif // _FLASHDEFS_H
