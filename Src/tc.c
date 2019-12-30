/*
 *  Open Space Data Link Protocol
 *
 *  Copyright (C) 2019 Libre Space Foundation (https://libre.space)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <tc.h>
#include <crc.h>
#include <cop.h>

#define	TC_TRANSFER_FRAME_PRIMARY_HEADER		5

int
tc_init(struct tc_transfer_frame *tc_tf,
        uint16_t scid,
        uint16_t max_fdu_size,
        uint8_t vcid,
        uint8_t mapid,
        tc_crc_flag_t crc_flag,
        tc_seg_hdr_t seg_hdr_flag,
        tc_bypass_t bypass,
        tc_ctrl_t ctrl_cmd,
        uint8_t *util_buffer,
        struct cop_params cop)
{
	if (max_fdu_size <= TC_TRANSFER_FRAME_PRIMARY_HEADER) {
		return 1;
	}
	struct tc_mission_params m;
	tc_tf->primary_hdr.version_num 			= TC_VERSION_NUMBER;
	tc_tf->primary_hdr.spacecraft_id		= scid & 0x03ff;
	tc_tf->primary_hdr.vcid					= vcid;
	tc_tf->primary_hdr.bypass				= bypass;
	tc_tf->primary_hdr.ctrl_cmd				= ctrl_cmd;
	tc_tf->primary_hdr.frame_len			= 0;
	tc_tf->primary_hdr.frame_seq_num		= 0;
	tc_tf->primary_hdr.rsvd_spare			= 0;
	m.crc_flag								= crc_flag;
	m.seg_hdr_flag							= seg_hdr_flag;
	m.max_fdu_size							= max_fdu_size;
	m.max_data_size							= max_fdu_size;
	m.util.buffer							= util_buffer;
	m.fixed_overhead_len					= TC_TRANSFER_FRAME_PRIMARY_HEADER;

	if (m.seg_hdr_flag == TC_SEG_HDR_PRESENT) {
		m.fixed_overhead_len += 1;
	}
	if (m.crc_flag == TC_CRC_PRESENT) {
		m.fixed_overhead_len += 2;
	}
	m.max_data_size							= m.max_fdu_size - m.fixed_overhead_len;
	m.util.loop_state 						= LOOP_CLOSED;
	tc_tf->mission							= m;
	tc_tf->frame_data.seg_hdr.map_id 		= mapid & 0x3f;
	tc_tf->cop_cfg 							= cop;
	return 0;
}

int
tc_unpack(struct tc_transfer_frame *tc_tf,  uint8_t *pkt_in)
{
	struct tc_primary_hdr tc_p_hdr;
	tc_p_hdr.version_num   	= ((pkt_in[0] >> 6) & 0x03);
	tc_p_hdr.bypass   		= ((pkt_in[0] >> 5) & 0x01);
	tc_p_hdr.ctrl_cmd 		= ((pkt_in[0] >> 4) & 0x01);
	tc_p_hdr.rsvd_spare    	= ((pkt_in[0] >> 2) & 0x03);
	tc_p_hdr.spacecraft_id 	= (((pkt_in[0] & 0x03) << 8) | pkt_in[1]);

	tc_p_hdr.vcid 			= ((pkt_in[2] >> 2) & 0x3f);
	tc_p_hdr.frame_len 		= (((pkt_in[2] & 0x03) << 8) | pkt_in[3]);
	tc_p_hdr.frame_seq_num 	= pkt_in[4];

	tc_tf->primary_hdr 		= tc_p_hdr;

	tc_tf->frame_data.data_len = tc_p_hdr.frame_len + 1 -
	                             tc_tf->mission.fixed_overhead_len;
	if (tc_p_hdr.ctrl_cmd  == TC_DATA) {
		if (tc_tf->mission.seg_hdr_flag) {
			tc_tf->frame_data.seg_hdr.seq_flag 	= (pkt_in[5] >> 6) & 0x03;
			tc_tf->frame_data.seg_hdr.map_id 	= pkt_in[5] & 0x3f;
			tc_tf->frame_data.data = &pkt_in[6];
		}
	} else {
		tc_tf->frame_data.data = &pkt_in[5];
	}
	if (tc_tf->mission.crc_flag == TC_CRC_PRESENT) {
		tc_tf->crc = pkt_in[tc_p_hdr.frame_len - 1] << 8;
		tc_tf->crc |= pkt_in[tc_p_hdr.frame_len];
	}
	return 0;
}

int
tc_pack(struct tc_transfer_frame *tc_tf, uint8_t *pkt_out,
        uint8_t *data_in, uint16_t length)
{
	uint16_t crc = 0;
	uint16_t packet_len = tc_tf->mission.fixed_overhead_len + length - 1;
	pkt_out[0] = ((tc_tf->primary_hdr.version_num & 0x03) << 6);
	pkt_out[0] |= ((tc_tf->primary_hdr.bypass & 0x01) << 5);
	pkt_out[0] |= ((tc_tf->primary_hdr.ctrl_cmd & 0x01) << 4);
	pkt_out[0] |= ((tc_tf->primary_hdr.rsvd_spare & 0x03) << 2);
	pkt_out[0] |= ((tc_tf->primary_hdr.spacecraft_id >> 8) & 0x03);

	pkt_out[1] = tc_tf->primary_hdr.spacecraft_id & 0xff;

	pkt_out[2] = ((tc_tf->primary_hdr.vcid & 0x3f) << 2);

	pkt_out[2] |= ((packet_len >> 8) & 0x03);
	pkt_out[3] = packet_len & 0xff;

	if (tc_tf->primary_hdr.bypass == TYPE_A) {
		pkt_out[4] = tc_tf->cop_cfg.fop.vs & 0xff;
	} else {
		pkt_out[4] = 0;
	}
	if (tc_tf->mission.seg_hdr_flag) {
		pkt_out[5] = ((tc_tf->frame_data.seg_hdr.seq_flag & 0x03) << 6);
		pkt_out[5] |= (tc_tf->frame_data.seg_hdr.map_id & 0x03);
		memcpy(&pkt_out[6], data_in, length * sizeof(uint8_t));
	} else {
		memcpy(&pkt_out[5], data_in, length * sizeof(uint8_t));
	}
	if (tc_tf->mission.crc_flag == TC_CRC_PRESENT) {
		crc = calc_crc(pkt_out, packet_len - 1);
		pkt_out[packet_len - 1] = (crc >> 8) & 0xff;
		pkt_out[packet_len] = crc & 0xff;
		tc_tf->crc = crc;
	}
	return 0;
}
