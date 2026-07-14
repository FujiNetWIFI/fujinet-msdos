#include "doctest.h"

extern "C" {
#include "../sys/nio_protocol.h"
}

#include <array>
#include <cstddef>
#include <cstdint>

namespace {

constexpr std::uint8_t DEV_NETWORK = 0xFD;
constexpr std::uint8_t CMD_READ = 0x02;
constexpr std::uint8_t CMD_WRITE = 0x03;
constexpr std::uint8_t STATUS_OK = 0x00;

struct Frame {
    nio_frame_header_t header{};
    std::array<std::uint8_t, 16> payload{};
    std::uint16_t rx_len{};
};

Frame make_response(std::uint8_t device, std::uint8_t command,
                    std::initializer_list<std::uint8_t> payload)
{
    Frame frame;
    std::size_t idx = 0;

    frame.header.device = device;
    frame.header.command = command;
    frame.header.length = static_cast<std::uint16_t>(NIO_FRAME_HEADER_SIZE + payload.size());
    frame.header.checksum = 0;
    frame.header.fields = NIO_PROTO_FIELD_A1;

    for (auto byte : payload)
        frame.payload[idx++] = byte;

    frame.header.checksum = static_cast<std::uint8_t>(
        nio_protocol_checksum(frame.payload.data(), static_cast<std::uint16_t>(payload.size()),
            nio_protocol_checksum(&frame.header, sizeof(frame.header), 0)));
    frame.rx_len = frame.header.length;
    return frame;
}

std::uint8_t validate(const Frame& frame, std::uint8_t expected_command,
                      std::uint16_t capacity, nio_parsed_response_t& parsed)
{
    return nio_protocol_validate_response(&frame.header, frame.payload.data(),
                                          frame.rx_len, DEV_NETWORK,
                                          expected_command, capacity, &parsed);
}

} // namespace

TEST_CASE("NIO protocol accepts a matching valid response")
{
    auto frame = make_response(DEV_NETWORK, CMD_READ, {STATUS_OK, 0x10, 0x20});
    nio_parsed_response_t parsed{};

    CHECK(sizeof(nio_frame_header_t) == static_cast<std::size_t>(NIO_FRAME_HEADER_SIZE));
    CHECK(validate(frame, CMD_READ, 8, parsed) == NIO_PROTO_OK);
    CHECK(parsed.status == STATUS_OK);
    CHECK(parsed.expected_len == frame.rx_len);
    CHECK(parsed.payload_offset == 1);
    CHECK(parsed.payload_length == 2);
}

TEST_CASE("NIO protocol rejects a stale response for a different command")
{
    auto stale = make_response(DEV_NETWORK, CMD_WRITE, {STATUS_OK, 0x01});
    nio_parsed_response_t parsed{};

    CHECK(validate(stale, CMD_READ, 8, parsed) == NIO_PROTO_ERR_DEVICE_COMMAND);
    CHECK(parsed.expected_len == stale.rx_len);
}

TEST_CASE("NIO protocol can discard stale frames until the expected response arrives")
{
    auto stale = make_response(DEV_NETWORK, CMD_WRITE, {STATUS_OK, 0x01});
    auto expected = make_response(DEV_NETWORK, CMD_READ, {STATUS_OK, 0xaa, 0xbb});
    nio_parsed_response_t parsed{};

    CHECK(validate(stale, CMD_READ, 8, parsed) == NIO_PROTO_ERR_DEVICE_COMMAND);
    CHECK(validate(expected, CMD_READ, 8, parsed) == NIO_PROTO_OK);
    CHECK(parsed.payload_length == 2);
}

TEST_CASE("NIO protocol rejects bad checksum")
{
    auto frame = make_response(DEV_NETWORK, CMD_READ, {STATUS_OK, 0x10});
    nio_parsed_response_t parsed{};

    frame.payload[1] ^= 0x55;

    CHECK(validate(frame, CMD_READ, 8, parsed) == NIO_PROTO_ERR_CHECKSUM);
}

TEST_CASE("NIO protocol rejects short and length-mismatched frames")
{
    auto frame = make_response(DEV_NETWORK, CMD_READ, {STATUS_OK});
    nio_parsed_response_t parsed{};

    frame.rx_len = NIO_FRAME_HEADER_SIZE - 1;
    CHECK(validate(frame, CMD_READ, 8, parsed) == NIO_PROTO_ERR_SHORT_FRAME);
    CHECK(parsed.expected_len == NIO_FRAME_HEADER_SIZE);

    frame = make_response(DEV_NETWORK, CMD_READ, {STATUS_OK});
    frame.rx_len = frame.header.length - 1;
    CHECK(validate(frame, CMD_READ, 8, parsed) == NIO_PROTO_ERR_LENGTH_MISMATCH);
    CHECK(parsed.expected_len == frame.header.length);
}

TEST_CASE("NIO protocol rejects invalid fields and oversized replies")
{
    auto frame = make_response(DEV_NETWORK, CMD_READ, {STATUS_OK, 0x10, 0x20});
    nio_parsed_response_t parsed{};

    frame.header.fields = 0;
    frame.header.checksum = 0;
    frame.header.checksum = static_cast<std::uint8_t>(
        nio_protocol_checksum(frame.payload.data(), 3,
            nio_protocol_checksum(&frame.header, sizeof(frame.header), 0)));
    CHECK(validate(frame, CMD_READ, 8, parsed) == NIO_PROTO_ERR_FIELDS);

    frame = make_response(DEV_NETWORK, CMD_READ, {STATUS_OK, 0x10, 0x20});
    CHECK(validate(frame, CMD_READ, 1, parsed) == NIO_PROTO_ERR_REPLY_TOO_LARGE);
}
