#include "doctest.h"

extern "C" {
#include "../sys/nio_disk_protocol.h"
}

#include <array>
#include <cstdint>

TEST_CASE("NIO disk builds slot requests used by info and clear-changed")
{
    std::array<std::uint8_t, NIO_DISK_SLOT_REQUEST_SIZE> req{};

    CHECK(nio_disk_build_slot_request(3, req.data(), req.size()) == req.size());
    CHECK(req[0] == NIO_DISK_VERSION);
    CHECK(req[1] == 3);
}

TEST_CASE("NIO disk builds multi-sector read request")
{
    std::array<std::uint8_t, NIO_DISK_READ_SECTORS_REQUEST_SIZE> req{};

    CHECK(nio_disk_build_read_sectors_request(2, 0x12345678UL, 16, 8192,
                                              req.data(), req.size()) == req.size());
    CHECK(req[0] == NIO_DISK_VERSION);
    CHECK(req[1] == 2);
    CHECK(nio_disk_get_u32le(&req[2]) == 0x12345678UL);
    CHECK(nio_disk_get_u16le(&req[6]) == 16);
    CHECK(nio_disk_get_u16le(&req[8]) == 8192);
}

TEST_CASE("NIO disk validates clear-changed response shape")
{
    std::array<std::uint8_t, NIO_DISK_CLEAR_CHANGED_RESPONSE_MIN_SIZE> resp =
        {NIO_DISK_VERSION, 0, 0, 0, 2};

    CHECK(nio_disk_validate_response(resp.data(), resp.size(),
                                     NIO_DISK_CLEAR_CHANGED_RESPONSE_MIN_SIZE));

    resp[0] = 0xff;
    CHECK_FALSE(nio_disk_validate_response(resp.data(), resp.size(),
                                           NIO_DISK_CLEAR_CHANGED_RESPONSE_MIN_SIZE));
}

TEST_CASE("NIO disk parses read-sector and read-sectors response lengths")
{
    std::array<std::uint8_t, 32> resp{};
    std::uint16_t data_len = 0;

    resp[0] = NIO_DISK_VERSION;
    nio_disk_put_u16le(&resp[9], 4);
    CHECK(nio_disk_parse_read_sector_response(resp.data(), 15, 8, &data_len));
    CHECK(data_len == 4);

    CHECK_FALSE(nio_disk_parse_read_sector_response(resp.data(), 15, 3, &data_len));
    CHECK_FALSE(nio_disk_parse_read_sector_response(resp.data(), 14, 8, &data_len));

    resp = {};
    resp[0] = NIO_DISK_VERSION;
    nio_disk_put_u16le(&resp[11], 8);
    CHECK(nio_disk_parse_read_sectors_response(resp.data(), 21, 16, &data_len));
    CHECK(data_len == 8);

    CHECK_FALSE(nio_disk_parse_read_sectors_response(resp.data(), 21, 7, &data_len));
    CHECK_FALSE(nio_disk_parse_read_sectors_response(resp.data(), 20, 16, &data_len));
}

TEST_CASE("NIO disk parses info response including geometry and optional error")
{
    std::array<std::uint8_t, 13> resp{};
    nio_disk_info_t info{};

    resp[0] = NIO_DISK_VERSION;
    resp[1] = NIO_DISK_INFO_INSERTED | NIO_DISK_INFO_CHANGED;
    resp[4] = 5;
    resp[5] = 4;
    nio_disk_put_u16le(&resp[6], 512);
    nio_disk_put_u32le(&resp[8], 4001760UL);
    resp[12] = NIO_STATUS_IO_ERROR;

    CHECK(nio_disk_parse_info_response(resp.data(), resp.size(), &info));
    CHECK(info.flags == (NIO_DISK_INFO_INSERTED | NIO_DISK_INFO_CHANGED));
    CHECK(info.slot == 5);
    CHECK(info.image_type == 4);
    CHECK(info.sector_size == 512);
    CHECK(info.sector_count == 4001760UL);
    CHECK(info.last_error == NIO_STATUS_IO_ERROR);
}
