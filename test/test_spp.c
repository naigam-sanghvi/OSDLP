/*
 * Open Space Data Link Protocol
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

#include "test.h"

void
test_spp_hdr_only(void **state)
{
	int32_t len;
	uint8_t out_buf[TC_MAX_SDU_SIZE] = { };
	struct spp_prim_hdr out_hdr = { };
	struct spp_prim_hdr prim_hdr1 = {
		.version = SPP_PACKET_VERSION_NUMBER,
		.is_tc = 0,
		.has_sec_hdr = 0,
		.apid = 0xAB,
		.seq_flag = SPP_SEQ_FLAG_UNSEGMENTED,
		.seq_count = 4,
		.packet_data_len = 0,
	};
	uint8_t packet1[] = { 0x00, 0xAB, 0xC0, 0x04, 0x00, 0x00 };
	struct spp_prim_hdr prim_hdr2 = {
		.version = 2,
		.is_tc = 1,
		.has_sec_hdr = 1,
		.apid = SPP_APID_IDLE,
		.seq_flag = SPP_SEQ_FLAG_FIRST_SEGMENT,
		.seq_count = 0x1234,
		.packet_data_len = TC_MAX_SDU_SIZE,
	};
	uint8_t packet2[] = { 0x5F, 0xFF, 0x52, 0x34, 0x04, 0x00 };

	/* Pack SPP primary header 1 */
	len = osdlp_spp_pack(&prim_hdr1, NULL, out_buf, sizeof(out_buf));
	assert_int_equal(len, sizeof(packet1));
	assert_memory_equal(out_buf, packet1, len);

	/* Unpack SPP primary header 1 */
	len = osdlp_spp_unpack(&out_hdr, packet1, sizeof(packet1), NULL);
	assert_int_equal(len, sizeof(packet1));
	assert_int_equal(out_hdr.version, prim_hdr1.version);
	assert_int_equal(out_hdr.is_tc, prim_hdr1.is_tc);
	assert_int_equal(out_hdr.has_sec_hdr, prim_hdr1.has_sec_hdr);
	assert_int_equal(out_hdr.apid, prim_hdr1.apid);
	assert_int_equal(out_hdr.seq_flag, prim_hdr1.seq_flag);
	assert_int_equal(out_hdr.seq_count, prim_hdr1.seq_count);
	assert_int_equal(out_hdr.packet_data_len, prim_hdr1.packet_data_len);

	/* Pack SPP primary header 2 */
	len = osdlp_spp_pack(&prim_hdr2, NULL, out_buf, sizeof(out_buf));
	assert_int_equal(len, sizeof(packet2));
	assert_memory_equal(out_buf, packet2, len);

	/* Unpack SPP primary header 2 */
	len = osdlp_spp_unpack(&out_hdr, packet2, sizeof(packet2), NULL);
	assert_int_equal(len, sizeof(packet2));
	assert_int_equal(out_hdr.version, prim_hdr2.version);
	assert_int_equal(out_hdr.is_tc, prim_hdr2.is_tc);
	assert_int_equal(out_hdr.has_sec_hdr, prim_hdr2.has_sec_hdr);
	assert_int_equal(out_hdr.apid, prim_hdr2.apid);
	assert_int_equal(out_hdr.seq_flag, prim_hdr2.seq_flag);
	assert_int_equal(out_hdr.seq_count, prim_hdr2.seq_count);
	assert_int_equal(out_hdr.packet_data_len, prim_hdr2.packet_data_len);
}

void
test_spp_hdr_with_data(void **state)
{
	int32_t len;
	uint8_t out_buf[TC_MAX_SDU_SIZE] = { };
	uint8_t in_pack_data[TC_MAX_SDU_SIZE - sizeof(struct spp_prim_hdr)];
	struct spp_prim_hdr out_hdr = { };
	uint8_t *out_pack_data = NULL;
	struct spp_prim_hdr prim_hdr = {
		.version = 2,
		.is_tc = 1,
		.has_sec_hdr = 1,
		.apid = SPP_APID_IDLE,
		.seq_flag = SPP_SEQ_FLAG_FIRST_SEGMENT,
		.seq_count = 0x1234,
		.packet_data_len = (TC_MAX_SDU_SIZE - sizeof(struct spp_prim_hdr) - 1),
	};
	uint8_t packet[] = { 0x5F, 0xFF, 0x52, 0x34, 0x03, 0xF9 };

	for (int i = 0; i < sizeof(in_pack_data); i++) {
		in_pack_data[i] = (i & 0xFF);
	}

	/* Pack SPP primary header with packet data */
	len = osdlp_spp_pack(&prim_hdr, in_pack_data, out_buf, sizeof(out_buf));
	assert_int_equal(len, TC_MAX_SDU_SIZE);
	assert_memory_equal(out_buf, packet, sizeof(struct spp_prim_hdr));
	assert_memory_equal((out_buf + sizeof(struct spp_prim_hdr)), in_pack_data,
	                    sizeof(in_pack_data));

	/* Unpack SPP primary header with packet data */
	len = osdlp_spp_unpack(&out_hdr, out_buf, sizeof(out_buf), &out_pack_data);
	assert_int_equal(len, TC_MAX_SDU_SIZE);
	assert_int_equal(out_hdr.version, prim_hdr.version);
	assert_int_equal(out_hdr.is_tc, prim_hdr.is_tc);
	assert_int_equal(out_hdr.has_sec_hdr, prim_hdr.has_sec_hdr);
	assert_int_equal(out_hdr.apid, prim_hdr.apid);
	assert_int_equal(out_hdr.seq_flag, prim_hdr.seq_flag);
	assert_int_equal(out_hdr.seq_count, prim_hdr.seq_count);
	assert_int_equal(out_hdr.packet_data_len, prim_hdr.packet_data_len);
	assert_int_equal(out_pack_data, (out_buf + sizeof(struct spp_prim_hdr)));
}

void
test_spp_invalid(void **state)
{
	int32_t len;
	uint8_t out_buf[TC_MAX_SDU_SIZE] = { };
	uint8_t in_pack_data[TC_MAX_SDU_SIZE];
	struct spp_prim_hdr out_hdr = { };
	uint8_t *out_pack_data = NULL;
	struct spp_prim_hdr prim_hdr = {
		.version = 2,
		.is_tc = 1,
		.has_sec_hdr = 1,
		.apid = SPP_APID_IDLE,
		.seq_flag = SPP_SEQ_FLAG_FIRST_SEGMENT,
		.seq_count = 0x1234,
		.packet_data_len = (TC_MAX_SDU_SIZE - sizeof(struct spp_prim_hdr) - 1),
	};

	/* Pack SPP with invalid arguments */
	len = osdlp_spp_pack(NULL, NULL, out_buf, sizeof(out_buf));
	assert_int_equal(len, -1);
	len = osdlp_spp_pack(&prim_hdr, NULL, NULL, 0);
	assert_int_equal(len, -1);

	/* Pack SPP with too small buffer */
	len = osdlp_spp_pack(&prim_hdr, NULL, out_buf,
	                     (sizeof(struct spp_prim_hdr) - 1));
	assert_int_equal(len, -1);
	len = osdlp_spp_pack(&prim_hdr, in_pack_data, out_buf,
	                     (TC_MAX_SDU_SIZE - 1));
	assert_int_equal(len, -1);

	/* Unpack SPP with invalid arguments */
	len = osdlp_spp_unpack(&out_hdr, NULL, 0, NULL);
	assert_int_equal(len, -1);
	len = osdlp_spp_unpack(NULL, out_buf, sizeof(out_buf), NULL);
	assert_int_equal(len, -1);

	/* Unpack SPP with too small buffer */
	len = osdlp_spp_unpack(&out_hdr, out_buf,
	                       (sizeof(struct spp_prim_hdr) - 1), NULL);
	assert_int_equal(len, -1);
	len = osdlp_spp_unpack(&out_hdr, out_buf, (TC_MAX_SDU_SIZE - 1),
	                       &out_pack_data);
	assert_int_equal(len, -1);
}
