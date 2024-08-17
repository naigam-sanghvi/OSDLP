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

#include <cstddef>
#include <cstdint>
#include <etl/vector.h>
#include <type_traits>

/**
 * @defgroup sp CCSDS Space Packet
 *
 * The CCSDS Space Packet implementation with the maximum Packet Data Field
 * defined at compile time
 *
 * @ingroup osdlp
 */
namespace osdlp
{

/**
 * @brief The base class of the Space Packet
 * This is the base class of the Space Packet. Can be also used as a reference
 * type, opposed to the osdlp::sp, which cannot due to the template
 * requirements.
 *
 * @ingroup sp
 */
class isp
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
   * @brief Sequence Flags of the Primary Header
   *
   * @note The use of the Sequence Flags is not mandatory for the users of the
   * SPP. However, the Sequence Flags may be used by the user of the Packet
   * Service to indicate that the User Data contained within the Space Packet is
   * a segment of a larger set of application data
   */
  enum class seq_flags : uint8_t
  {
    CONTINUATION =
        0, /*!< the Space Packet contains a continuation segment of User Data */
    FIRST_SEGMENT =
        1, /*!< the Space Packet contains the first segment of User Data*/
    LAST_SEGMENT =
        2, /*!< the Space Packet contains the last segment of User Data */
    UNSEGMENTED = 3 /*!< the Space Packet contains unsegmented User Data */
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
  public:
    /**
     * @brief The size in bytes
     *
     */
    static constexpr size_t len = 6;
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
  public:
    const uint8_t *
    cbegin();
    const uint8_t *
    cend();

    size_t
    size() const;

    bool
    valid() const;

  private:
  };

protected:
  isp(size_t max_pdf_len, size_t sec_hdr_len, etl::ivector<uint8_t> &buf)
      : m_pdf_max_len(max_pdf_len), m_buf(buf), m_sec_hdr_len()
  {
  }

  isp(size_t max_pdf_len, etl::ivector<uint8_t> &buf) : isp(max_pdf_len, 0, buf)
  {
  }

  /**
   * @brief Get the presence or absence of a Secondary Header in the Space
   * Packet
   *
   * @return true if there is a secondary header
   * @return false if there is no secondary header
   */
  bool
  has_secondary_hdr() const
  {
    return m_sec_hdr_len > 0;
  }

  /**
   * @brief Get the length of the Secondary Header
   *
   * @return size_t  the length of the Secondary Header or 0, if there is no
   * secondary header
   */
  size_t
  secondary_hdr_len()
  {
    return m_sec_hdr_len;
  }

  /**
   * @brief Fills the PDF of the Space Packet with a specific value
   *
   * @param val the value to fill
   *
   * @note This method does not change the existing size of the PDF
   */
  void
  fill_pdf(uint8_t val)
  {
    etl::fill(m_buf.begin() + primary_hdr::len, m_buf.end(), val);
  }

  /**
   * @brief Fills the User Data Field (UDF) of the Space Packet with a specific
   * value
   *
   * @param val the value to fill
   *
   * @note This method does not change the existing size of the PDF or the UDF
   */
  void
  fill_udf(uint8_t val)
  {
    etl::fill(m_buf.begin() + primary_hdr::len + m_sec_hdr_len, m_buf.end(),
              val);
  }

  /**
   * @brief Checks if the SP is valid
   *
   * Checks if all mandatory internal fields of the SP and their requirements
   * are met
   *
   * @return true
   * @return false
   */
  bool
  is_valid() const
  {
    return false;
  }

private:
  const size_t           m_pdf_max_len;
  const size_t           m_sec_hdr_len;
  etl::ivector<uint8_t> &m_buf;
};

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
 * The OSDLP library supports the secondary size, however its size should be
 * known at compile time.
 *
 * \image html spp/spp.png
 * \image latex spp/spp.png "Space Packet Structural Components
 *
 * @tparam PDF_MAX_SIZE The maximum size of the Packet Data Field (PDF).
 * @tparam SECONDARY_SIZE CCSDS SP may have a secondary header. Its size is
 * mission specific and should be arranged beforehand. OSDLP supports the
 * secondary header, but requires its size at compile time. If no secondary
 * header is needed, set this parameter to 0.
 *
 * @ingroup sp
 *
 */
template <const size_t PDF_MAX_SIZE, const size_t SECONDARY_SIZE = 0>
class sp : public isp
{
public:
  sp(version v, type t, uint16_t apid, seq_flags flags, uint16_t seq_cnt)
      : isp(PDF_MAX_SIZE, m_buf)
  {
  }

  /**
   * @brief Construct an empty Space Packet
   *
   */
  sp() : isp(PDF_MAX_SIZE, m_buf) {}

  /**
   * @brief Construct a Space Packet based on a given input byte stream
   *
   * @tparam IteratorT
   * @tparam std::enable_if_t<std::is_same_v<
   * typename std::iterator_traits<IteratorT>::value_type, uint8_t>>
   * @param begin an iterator to the start of the byte stream
   * @param end an iterator to the end of the byte stream
   */
  template <typename IteratorT,
            typename = typename std::enable_if_t<std::is_same_v<
                typename std::iterator_traits<IteratorT>::value_type, uint8_t>>>
  sp(IteratorT begin, IteratorT end) : isp(PDF_MAX_SIZE, m_buf)
  {
  }

private:
  etl::vector<uint8_t, PDF_MAX_SIZE + primary_hdr::len> m_buf;
};

} // namespace osdlp
