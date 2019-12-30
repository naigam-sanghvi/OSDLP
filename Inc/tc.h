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

#ifndef INC_TC_H_
#define INC_TC_H_

#include <stdbool.h>
#include <stdint.h>
#include <cop.h>

/**
 * The maximum allowed frame size
 */
#define MAX_FRAME_LEN			1024
#define TC_VERSION_NUMBER		0

typedef enum {
	TYPE_A 		= 0,
	TYPE_B		= 1
} tc_bypass_t;

typedef enum {
	TC_DATA		= 0,
	TC_COMMAND	= 1
} tc_ctrl_t;

/*State of reception of segmented FDUs*/
typedef enum {
	LOOP_OPEN		= 0,
	LOOP_CLOSED		= 1
} loop_state_t;

typedef enum {
	TC_CRC_NOTPRESENT 	= 0,
	TC_CRC_PRESENT 		= 1
} tc_crc_flag_t;

typedef enum {
	TC_SEG_HDR_NOTPRESENT	= 0,
	TC_SEG_HDR_PRESENT		= 1
} tc_seg_hdr_t;

typedef enum {
	SEG_ENDED		= 0,
	SEG_IN_PROGRESS	= 1
} seg_status_t;

struct tc_primary_hdr {
	uint8_t				version_num 	: 2;	/* TC version number*/
	uint8_t				bypass			: 1;	/* Bypass flag*/
	uint8_t				ctrl_cmd		: 1;	/* Control command*/
	uint8_t				rsvd_spare		: 2;	/* Raserved - spare*/
	uint16_t			spacecraft_id	: 10;	/* Spacecraft ID*/
	uint8_t				vcid 			: 6;	/* Cirtual Channel ID*/
	uint16_t			frame_len		: 10;	/* Frame length (Total octets - 1)*/
	uint8_t				frame_seq_num;			/* Frame sequence number*/
};

/**
 * Sequence flag type
 */
typedef enum {
	TC_CONT_SEG 	= 0,
	TC_FIRST_SEG 	= 1,
	TC_LAST_SEG 	= 2,
	TC_UNSEG 		= 3
} tc_seq_flag_t;

struct tc_seg_hdr {
	uint8_t			seq_flag 	: 2; 	/* Sequence flag*/
	uint8_t			map_id		: 6;	/* MAP ID*/
};

/**
 * Frame data field struct
 */
struct	tc_fdf {
	struct tc_seg_hdr	seg_hdr;
	uint8_t 			*data;
	uint16_t			data_len;
};

/**
 * Utility buffer for use in segmentation operations
 */
struct util_buf {
	uint8_t			*buffer;
	uint16_t 			buffered_length;
	uint8_t				loop_state;
};

/**
 * Mission specific parameters
 */
struct tc_mission_params {
	struct	util_buf	util;
	uint8_t				crc_flag;			/**/
	uint8_t				seg_hdr_flag;
	uint16_t			max_data_size;
	uint16_t			max_fdu_size;
	uint16_t			fixed_overhead_len;
};

/**
 * Segmentation status struct
 */
struct segment_status {
	uint8_t		flag;
	uint16_t	octets_txed;
};


struct tc_transfer_frame {
	struct tc_primary_hdr		primary_hdr;	/* Primary header struct*/
	struct tc_mission_params	mission;		/* Mission params*/
	struct cop_params			cop_cfg;		/* COP configuration struct*/
	struct tc_fdf				frame_data;		/* Frame data structure*/
	struct segment_status		seg_status;		/* Segment status struct*/
	uint16_t					crc;			/* CRC*/
};

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
        struct cop_params cop);
/**
 * Unpacks a received buffer and populates the corresponding fields of
 * a TC config structure
 * @param frame_params the TC config struct
 * @param pkt_in the received packet buffer
 */
int
tc_unpack(struct tc_transfer_frame *tc_tf,  uint8_t *pkt_in);

/**
 * Packs a TC structure into a buffer to be transmitted
 * @param frame_params the TC config struct
 * @param pkt_out the buffer where the packet will be placed
 * @param data_in the input data buffer
 * @param length the length of the data_in buffer
 */
int
tc_pack(struct tc_transfer_frame *tc_tf, uint8_t *pkt_out,
        uint8_t *data_in, uint16_t length);

#endif /* INC_TC_H_ */
