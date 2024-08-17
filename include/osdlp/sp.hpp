/*
 *  Open Space Data Link Protocol
 *
 *  Copyright (C) 2020-2024, Libre Space Foundation <http://libre.space>
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
 *
 *  SPDX-License-Identifier: GNU General Public License v3.0 or later
 */

#pragma once

#include <cstdint>

namespace osdlp
{

/**
 * @brief The CCSDS Space Packet class
 *
 * This class implements the Space Packet PDU which is the basis of the Space
 * Packet Protocol.
 *
 * The Space Packet consists from a fixed sized primary header, an optional
 * variable-sized secondary header and a variable-sized user data field. The
 * maximum size of the the Space Packet without the primary header must be less
 * than 65536 bytes.
 *
 * \image html spp/spp.png
 * \image latex spp/spp.png "Space Packet Structural Components
 *
 */
class sp
{
public:
  /**
   * @brief Packet version number
   *
   * @note CCSDS 133.0-B-2: The Version Number is used to reserve the
   * possibility of introducing other packet structures. This Recommended
   * Standard defines Version 1 CCSDS Packet whose binary encoded Version Number
   * is ‘000’.
   *
   */
  enum class version : uint8_t
  {
    CCSDS_V1 = 0 /*!< Version 1 CCSDS Packet */
  };

  /**
   * @brief Packet type
   *
   *  \b 4.1.3.3.2.3: For a telemetry (or reporting) Packet, this bit shall be
   * set to ‘0’; for a telecommand (or requesting) Packet, this bit shall be set
   * to ‘1’.
   */
  enum class type : uint8_t
  {
    TC = 0, /*!< Telecommand packet*/
    TM = 1  /*!< Telemetry packet*/
  };

  /**
   * @brief The SPP primary header
   *
   * The Packet Primary Header is mandatory and shall consist of four fields,
   * positioned contiguously, in the following sequence:
   * -# Packet Version Number (3 bits, mandatory);
   * -# Packet Identification Field (13 bits, mandatory);
   * -# Packet Sequence Control Field (16 bits, mandatory);
   * -# Packet Data Length (16 bits, mandatory).
   * \image html spp/primary_hdr.png
   * \image latex spp/primary_hdr.png "Packet Primary Header"
   */
  class primary_hdr
  {
  };

  /**
   * @brief The SPP secondary header
   * @note This is an optional header
   *
   * @note The purpose of the Packet Secondary Header is to allow (but not
   * require) a mission- specific means for consistently placing ancillary data
   * (time, internal data field format, spacecraft position/attitude, etc.) in
   * the same location within a Space Packet. The format of the secondary
   * header, if present, is managed and mission specific.
   *
   */
  class secondary_hdr
  {
  };

private:
  uint16_t m_pd_length;
  bool     m_has_secondary;
};

} // namespace osdlp
