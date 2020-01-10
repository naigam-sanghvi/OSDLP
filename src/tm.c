/*
 *  Open Space Data Link Protocol
 *
 *  Copyright (C) 2020 Libre Space Foundation (https://libre.space)
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

#include "tm.h"

#include <stdlib.h>
#include <string.h>

int
tm_init(struct tm_transfer_frame *tm_tf,
        uint16_t spacecraft_id,
        uint8_t *mc_count,
        uint8_t vcid,
        uint8_t mcid,
        tm_ocf_flag_t ocf_flag,
        tm_sec_hdr_flag_t sec_hdr_fleg,
        tm_sync_flag_t sync_flag,
        uint8_t sec_hdr_len,
        uint8_t *sec_hdr,
        uint32_t ocf,
        tm_crc_flag_t crc_flag,
        uint16_t frame_len,
        uint16_t max_vcs,
        uint16_t fifo_max_size)
{
	tm_tf->primary_hdr.mcid.version_num 	= TM_VERSION_NUMBER;
	tm_tf->primary_hdr.mcid.spacecraft_id 	= spacecraft_id & 0x03ff;
	tm_tf->primary_hdr.vcid 				= vcid;
	tm_tf->primary_hdr.ocf 					= ocf_flag & 0x01;
	tm_tf->primary_hdr.mc_frame_cnt			= mc_count;
	tm_tf->primary_hdr.vc_frame_cnt 		= 0;
	tm_tf->primary_hdr.status.sec_hdr 		= sec_hdr_fleg & 0x01;
	tm_tf->primary_hdr.status.sync 			= sync_flag & 0x01;
	tm_tf->primary_hdr.status.pkt_order		= 0;
	tm_tf->primary_hdr.status.seg_len_id 	= 0x03;

	tm_tf->mission.crc_present				= crc_flag;
	tm_tf->mission.frame_len				= frame_len;
	tm_tf->mission.max_vcs					= max_vcs;
	tm_tf->mission.tx_fifo_max_size			= fifo_max_size;

	/* Prepare the idle packet*/
	tm_tf->mission.idle_packet = 0;
	tm_tf->mission.idle_packet |= (0x07 << 5);

	uint16_t occupied = TM_PRIMARY_HDR_LEN;
	uint16_t occupied_header = TM_PRIMARY_HDR_LEN;
	if (tm_tf->primary_hdr.status.sec_hdr == TM_SEC_HDR_PRESENT) {
		tm_tf->secondary_hdr.sec_hdr_id.version_num = 0;
		tm_tf->secondary_hdr.sec_hdr_id.length = sec_hdr_len;
		memcpy(tm_tf->secondary_hdr.sec_hdr_data_field,
		       sec_hdr,
		       sec_hdr_len * sizeof(uint8_t));
		occupied += tm_tf->secondary_hdr.sec_hdr_id.length + 1;
		occupied_header = occupied;
	}
	if (tm_tf->primary_hdr.ocf == TM_OCF_PRESENT) {
		tm_tf->ocf = ocf;
		occupied += TM_OCF_LENGTH;
	}
	if (tm_tf->mission.crc_present == TM_CRC_PRESENT) {
		occupied += TM_CRC_LENGTH;
	}

	tm_tf->mission.max_data_len = tm_tf->mission.frame_len - occupied;
	tm_tf->mission.header_len = occupied_header;

	return 0;
}

int
tm_pack(struct tm_transfer_frame *tm_tf, uint8_t *pkt_out,
        uint8_t *data_in, uint16_t length)
{
	pkt_out[0] = ((tm_tf->primary_hdr.mcid.version_num & 0x03) << 6);
	pkt_out[0] |= ((tm_tf->primary_hdr.mcid.spacecraft_id >> 4) & 0x3f);
	pkt_out[1] = ((tm_tf->primary_hdr.mcid.spacecraft_id & 0x0f) << 4);
	pkt_out[1] |= ((tm_tf->primary_hdr.vcid & 0x07) << 1);
	pkt_out[1] |= (tm_tf->primary_hdr.ocf & 0x01);

	pkt_out[2] = (*tm_tf->primary_hdr.mc_frame_cnt & 0xff);
	pkt_out[3] = (tm_tf->primary_hdr.vc_frame_cnt & 0xff);

	pkt_out[4] = ((tm_tf->primary_hdr.status.sec_hdr & 0x01) << 7);
	pkt_out[4] |= ((tm_tf->primary_hdr.status.sync & 0x01) << 6);
	pkt_out[4] |= ((tm_tf->primary_hdr.status.pkt_order & 0x01) << 5);
	pkt_out[4] |= ((tm_tf->primary_hdr.status.seg_len_id & 0x03) << 3);
	pkt_out[4] |= ((tm_tf->primary_hdr.status.first_hdr_ptr >> 8) & 0x07);
	pkt_out[5] = tm_tf->primary_hdr.status.first_hdr_ptr & 0xff;

	if (tm_tf->primary_hdr.status.sec_hdr == TM_SEC_HDR_PRESENT) {
		pkt_out[6] = ((tm_tf->secondary_hdr.sec_hdr_id.version_num &
		               0x03) << 6);
		pkt_out[6] |= (tm_tf->secondary_hdr.sec_hdr_id.length & 0x3f);
		memcpy(&pkt_out[7], tm_tf->secondary_hdr.sec_hdr_data_field,
		       sizeof(uint8_t)*tm_tf->secondary_hdr.sec_hdr_id.length);
	}
	memcpy(&pkt_out[tm_tf->mission.header_len],
	       data_in, length * sizeof(uint8_t));

	/* Add OCF */
	if (tm_tf->primary_hdr.ocf == TM_OCF_PRESENT) {
		pkt_out[tm_tf->mission.header_len + tm_tf->mission.max_data_len] =
		        (tm_tf->ocf >> 24) & 0xff;
		pkt_out[tm_tf->mission.header_len + tm_tf->mission.max_data_len + 1] =
		        (tm_tf->ocf >> 16) & 0xff;
		pkt_out[tm_tf->mission.header_len + tm_tf->mission.max_data_len + 2] =
		        (tm_tf->ocf >> 8) & 0xff;
		pkt_out[tm_tf->mission.header_len + tm_tf->mission.max_data_len + 3] =
		        tm_tf->ocf & 0xff;
	}

	/* Add CRC */
	if (tm_tf->mission.crc_present == TM_CRC_PRESENT) {
		uint16_t crc = 0;
		pkt_out[tm_tf->mission.frame_len - 2] = 0;
		pkt_out[tm_tf->mission.frame_len - 1] = 0;
		crc = calc_crc(pkt_out, tm_tf->mission.frame_len - 2);

		pkt_out[tm_tf->mission.frame_len - 2] = (crc >> 8) & 0xff;
		pkt_out[tm_tf->mission.frame_len - 1] = crc & 0xff;
		tm_tf->crc = crc;
	}

	/* Add idle packets */
	if (length < tm_tf->mission.max_data_len) {
		memset(&pkt_out[tm_tf->mission.header_len + length],
		       tm_tf->mission.idle_packet,
		       (tm_tf->mission.max_data_len - length) * sizeof(uint8_t));
	}
	return 0;
}

int
tm_unpack(struct tm_transfer_frame *tm_tf, uint8_t *pkt_in)
{
	tm_tf->primary_hdr.mcid.version_num 	= ((pkt_in[0] >> 6) && 0x03);
	tm_tf->primary_hdr.mcid.spacecraft_id 	= ((pkt_in[0] & 0x3f) << 4) | ((
	                        pkt_in[1] >> 4) & 0x0f);
	tm_tf->primary_hdr.vcid 				= (pkt_in[1] >> 1) & 0x07;
	tm_tf->primary_hdr.ocf	 				= pkt_in[1] & 0x01;
	*tm_tf->primary_hdr.mc_frame_cnt 		= pkt_in[2];
	tm_tf->primary_hdr.vc_frame_cnt 		= pkt_in[3];
	tm_tf->primary_hdr.status.sec_hdr 		= (pkt_in[4] >> 7) & 0x01;
	tm_tf->primary_hdr.status.sync 			= (pkt_in[4] >> 6) & 0x01;
	tm_tf->primary_hdr.status.pkt_order 	= (pkt_in[4] >> 5) & 0x01;
	tm_tf->primary_hdr.status.seg_len_id 	= (pkt_in[4] >> 3) & 0x03;
	tm_tf->primary_hdr.status.first_hdr_ptr	= (pkt_in[4] & 0x07) | pkt_in[5];

	if (tm_tf->primary_hdr.status.sec_hdr == TM_SEC_HDR_PRESENT) {
		tm_tf->secondary_hdr.sec_hdr_id.version_num 	= (pkt_in[6] >> 6) & 0x03;
		tm_tf->secondary_hdr.sec_hdr_id.length			= pkt_in[6] & 0x3f;
	}
	tm_tf->data = &pkt_in[tm_tf->mission.header_len];

	if (tm_tf->primary_hdr.ocf == TM_OCF_PRESENT) {
		tm_tf->ocf = pkt_in[tm_tf->mission.header_len + tm_tf->mission.max_data_len] <<
		             24;
		tm_tf->ocf |= pkt_in[tm_tf->mission.header_len + tm_tf->mission.max_data_len +
		                                               1] << 16;
		tm_tf->ocf |= pkt_in[tm_tf->mission.header_len + tm_tf->mission.max_data_len +
		                                               2] << 8;
		tm_tf->ocf |= pkt_in[tm_tf->mission.header_len + tm_tf->mission.max_data_len +
		                                               3];
	}
	if (tm_tf->mission.crc_present == TM_CRC_PRESENT) {
		//tm_tf->crc = calc_crc(pkt_)
		tm_tf->crc = pkt_in[tm_tf->mission.frame_len - 2] << 8;
		tm_tf->crc |= pkt_in[tm_tf->mission.frame_len - 1];
	}


	return 0;
}





