/*
 * Space Packet Protocol
 *
 * Copyright (C) 2021 TU Darmstadt Space Technology e.V.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <string.h>

#include "osdlp_spp.h"

int32_t
osdlp_spp_pack(struct spp_prim_hdr *prim_hdr, uint8_t *in_pack_data,
               uint8_t *out_buf, size_t buf_size)
{
	int32_t out_len; /*output data length */ /* N:length in bytes */

	/* but allow in_pack_data to e NULL */
	if (!prim_hdr || !out_buf)
		return -1;

	out_len = sizeof(struct spp_prim_hdr);
	if (buf_size < out_len)
		return -1;

	/* primary header */ /* N: & signifies bitwose AND operation,<< and  >> mean left shifting and right shifting of bits respectively, and | means bitwise OR operation */
	/* N: Example number - 1010 1010 1010 1111, take AND with 00000111 (= 0x07), answer - 0000 0000 0000 0111, shift left by 5 bits - 0000 0000 1110 0000 */
	/* N: Since out_but is uint8_t, answer will be truncated to last 8 bits (1110 0000), take OR operation with another number 0001 1001 , final answer - 1111 1001 */
	out_buf[0] = ((prim_hdr->version & 0x07) << 5) | /* N: last 3 bits of version are useful */
	             ((prim_hdr->is_tc & 0x01) << 4) | /* N: last bit of is_tc is useful */
	             ((prim_hdr->has_sec_hdr & 0x01) << 3) | /* N:  last bit of has_sec_hdr is useful */
	             ((prim_hdr->apid >> 8) & 0x07); /* N: bit 6-16 of apid are useful, bit 6-8 stored here */
	out_buf[1] = prim_hdr->apid & 0xFF; /* N: bit 6-16 of apid are useful, bit 9-16 stored here */
	out_buf[2] = ((prim_hdr->seq_flag & 0x03) << 6) | /* N: last 2 bits of seq_flag stored here */
	             ((prim_hdr->seq_count >> 8) & 0x3F); /* N: bit 3-16 of seq_cout are useful, bit 3-8 stored here */
	out_buf[3] = prim_hdr->seq_count & 0xFF; /* N: bit 3-16 of seq_cout are useful, bit 9-16 stored here */
	out_buf[4] = (prim_hdr->packet_data_len >> 8) & 0xFF; /* N: bit 1-16 of packet_data_len are useful, bit 1-8 stored here */
	out_buf[5] = prim_hdr->packet_data_len & 0xFF; /* N: bit 1-16 of packet_data_len are useful, bit 9-16 stored here */

	/* if packet data (secondary header + user data) */
	if (in_pack_data) {
		out_len += prim_hdr->packet_data_len + 1; /* N: increase out_len if in_pack_data is not null */
		if (buf_size < out_len)
			return -1;

		memcpy(&out_buf[sizeof(struct spp_prim_hdr)], in_pack_data, 
		       (prim_hdr->packet_data_len + 1)); /* N: copies packet data to out_buf after the spp_prim_hdr */
	}

	return out_len;
}

int32_t
osdlp_spp_unpack(struct spp_prim_hdr *prim_hdr, uint8_t *in_buf, size_t in_len,
                 uint8_t **out_pack_data)
{
	int32_t parsed_len; /* length of parsed data */

	/* but allow out_pack_data to be NULL */
	if (!in_buf || !prim_hdr)
		return -1;

	parsed_len = sizeof(struct spp_prim_hdr);
	if (in_len < parsed_len)
		return -1;

	/* primary header */
	prim_hdr->version = ((in_buf[0] >> 5) & 0x07); /* N: first 3 bits of in_buf[0] are useful */
	prim_hdr->is_tc = ((in_buf[0] >> 4) & 0x01); /* N: bit 4 of in_buf[0] is useful */
	prim_hdr->has_sec_hdr = ((in_buf[0] >> 3) & 0x01); /* N: bit 5 of in_buf[0] is useful */
	prim_hdr->apid = (((uint16_t)in_buf[0] & 0x07) << 8) | in_buf[1]; /* N: bit 6-8 of in_buf[0] and in_buf[1] is useful */
	prim_hdr->seq_flag = ((in_buf[2] >> 6) & 0x03); /* N: first 2 bit of in_buf[2] is useful */
	prim_hdr->seq_count = (((uint16_t)in_buf[2] & 0x3F) << 8) | in_buf[3]; /* N: bit 3-8 of in_buf[2] and in_buf[3] is useful */
	prim_hdr->packet_data_len = ((uint16_t)in_buf[4] << 8) | in_buf[5]; /* N: in_buf[4] and in_buf[5] are useful */

	/* packet data (secondary header + user data) */
	if (out_pack_data) {
		parsed_len += prim_hdr->packet_data_len + 1; /* N: increase parsed_len if out_pack_data is not null */
		if (in_len < parsed_len)
			return -1;

		*out_pack_data = &in_buf[sizeof(struct spp_prim_hdr)]; /* N: Sets *out_pack_data to point to the start of the packet data field */
	}

	return parsed_len;
}
