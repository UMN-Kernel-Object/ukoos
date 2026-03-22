/*
 * SPDX-FileCopyrightText: 2025 ukoOS Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "udp.h"

u16 calc_chksum(u16 *buf,u64 size) {
	u32 sum = 0; // larger than u16 to catch overflow

	while (size > 1) {
		sum += *buf++;
		size -= sizeof(u16);
	}
	if (size > 0) {
		sum += *(u8*)buf; // we have only 1 byte left
	}

	// strip off overflow and wrap it around if it occured.
	sum = (sum & 0x0000FFFF) + (sum >> 16);
	sum += (sum >> 16); // previous addition can result in overflow if first bit is 1

	return (u16)~sum;

}
