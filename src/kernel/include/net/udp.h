#include <types.h>

// Implementation of https://www.rfc-editor.org/rfc/rfc768

struct udp_header 
{
	u16 srcport;
	u16 dstport;
	u16 len;
	u16 chksum;
}

/*
 * 
 * Checksum is the 16-bit one's complement of the one's complement sum of a
 * pseudo header of information from the IP header, the UDP header, and the
 * data,  padded  with zero octets  at the end (if  necessary)  to  make  a
 * multiple of two octets.
 *
 * The pseudo  header  conceptually prefixed to the UDP header contains the
 * source  address,  the destination  address,  the protocol,  and the  UDP
 * length.   This information gives protection against misrouted datagrams.
 * This checksum procedure is the same as is used in TCP.
 *
 * 									0      7 8     15 16    23 24    31
 * 								 +--------+--------+--------+--------+
 * 								 |          source address           |
 * 								 +--------+--------+--------+--------+
 * 								 |        destination address        |
 * 								 +--------+--------+--------+--------+
 * 								 |  zero  |protocol|   UDP length    |
 * 								 +--------+--------+--------+--------+
 *
 * If the computed  checksum  is zero,  it is transmitted  as all ones (the
 * equivalent  in one's complement  arithmetic).   An all zero  transmitted
 * checksum  value means that the transmitter  generated  no checksum  (for
 * debugging or for higher level protocols that don't care).
 * 
 * 
 */
u16 calc_chksum(u8 *buf, u64 size); 


u64 udp_read(int fd, udp_header *header, void * data, u64 len);
u64 udp_write(int fd, udp_header *header, void *data, u64 len);



