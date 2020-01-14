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

#include <stdint.h>
#include <stdbool.h>

#include "crc.h"

#ifndef INC_TM_H_
#define INC_TM_H_

/**
 * The maximum allowed frame size
 */
#define MAX_FRAME_LEN                   	1024
#define TM_VERSION_NUMBER					0
#define TM_SEC_HDR_VERSION_NUMBER			0
#define	TM_PRIMARY_HDR_LEN					6
#define	TM_OCF_LENGTH						4
#define TM_CRC_LENGTH						2
#define	TM_FIRST_HDR_PTR_NO_PKT_START		0x07FF

typedef enum {
	TM_OCF_NOTPRESENT 	= 0,
	TM_OCF_PRESENT 		= 1
} tm_ocf_flag_t;

typedef enum {
	TM_CRC_NOTPRESENT 	= 0,
	TM_CRC_PRESENT 		= 1
} tm_crc_flag_t;

typedef enum {
	TM_SEC_HDR_NOTPRESENT 	= 0,
	TM_SEC_HDR_PRESENT 		= 1
} tm_sec_hdr_flag_t;

typedef enum {
	TYPE_OS_ID 		= 0,
	TYPE_VCA_SDU 	= 1
} tm_sync_flag_t;

struct tm_master_channel_id {
	uint8_t		version_num 	: 2;
	uint16_t	spacecraft_id	: 10;
};

/**
 * Data field status struct
 */
struct tm_df_status {
	uint8_t					sec_hdr			: 1;
	uint8_t					sync			: 1;
	uint8_t					pkt_order 		: 1;
	uint8_t					seg_len_id		: 2;
	uint16_t				first_hdr_ptr	: 11;
};

/**
 * Data field status of primary header
 */
struct tm_primary_hdr {
	struct tm_master_channel_id 	mcid;
	uint8_t							vcid 			: 3;
	uint8_t							ocf				: 1;
	uint8_t							*mc_frame_cnt;			// This field needs to be shared among all vcs
	uint8_t							vc_frame_cnt;
	struct tm_df_status				status;
};

/**
 * Secondary header ID struct
 */
struct tm_sec_hdr_id {
	uint8_t		version_num : 2;
	uint8_t		length		: 6;
};

/**
 * Secondary header struct
 */
struct tm_sec_hdr {
	struct tm_sec_hdr_id		sec_hdr_id;
	uint8_t						*sec_hdr_data_field;
};

/**
 * Mission parameters struct
 */
struct tm_mission_params {
	uint16_t					frame_len;			/* Fixed frame length*/
	uint16_t					header_len;			/* Header length - Including secondary if present*/
	uint16_t					max_data_len;		/* Maximum allowed data size per FDU*/
	uint8_t						crc_present;		/* CRC present flag*/
	uint16_t					max_vcs;			/* Max number oc VCs allowed*/
	uint16_t					tx_fifo_max_size;	/* Max TX FIFO capacity*/
	uint8_t						idle_packet;		/* Idle packet for stuffing*/
};

struct tm_transfer_frame {
	struct tm_primary_hdr		primary_hdr;		/* The primary header struct*/
	struct tm_sec_hdr			secondary_hdr;		/* The secondary header struct*/
	uint32_t					ocf;				/* The OCF struct */
	uint16_t					crc;				/* CRC value*/
	uint8_t					*data;				/* Pointer to FDU*/
	struct tm_mission_params	mission;			/* Mission specific parameters*/
};

/**
 * Initializes the TM config structure
 * @param spacecraft_id the spacecraft ID
 * @param mc_count the master channel count. Must be common
 * for all TM structures of different VCs
 * @param vcid the virtual channel ID
 * @param mcid master channel ID struct
 * @param ocf_flag flag for the presence or not of OCF
 * @param sec_hdr_fleg flag for the presence or not of
 * the secondary header
 * @param sync_flag sync flag
 * @param sec_hdr_len length of secondary header if present
 * @param sec_hdr pointer to the secondary header structure
 * if present
 * @param OCF value
 * @param crc_flag flag for the presence or not of CRC
 * @param frame_len Fixed frame length
 * @param max_vcs Max number oc VCs allowed
 * @param fifo_max_size Max TX FIFO capacity
 *
 * @param
 */
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
        uint16_t fifo_max_size);

/**
 * Packs a TM structure into a buffer to be transmitted
 * @param frame_params the TM config struct
 * @param pkt_out the buffer where the packet will be placed
 * @param data_in the input data buffer
 * @param length the length of the data_in buffer
 */
int
tm_pack(struct tm_transfer_frame *frame_params, uint8_t *pkt_out,
        uint8_t *data_in, uint16_t length);

/**
 * Unpacks a received buffer and populates the corresponding fields of
 * a TM config structure
 * @param frame_params the TM config struct
 * @param pkt_in the received packet buffer
 */
int
tm_unpack(struct tm_transfer_frame *frame_params, uint8_t *pkt_in);

#endif /* INC_TM_H_ */
