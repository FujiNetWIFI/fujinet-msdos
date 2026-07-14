#include "doctest.h"

extern "C" {
#include "../sys/nio_transaction.h"
}

#include <array>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>

namespace {

constexpr std::uint8_t DEV_NETWORK = NIO_DEVICEID_NETWORK;
constexpr std::uint8_t CMD_READ = 0x02;
constexpr std::uint8_t CMD_WRITE = 0x03;
constexpr std::uint8_t STATUS_OK = NIO_STATUS_OK;

struct Frame {
    nio_frame_header_t header{};
    std::array<std::uint8_t, 32> payload{};
    std::uint16_t rx_len{};
};

struct Event {
    Frame frame{};
    std::uint8_t reason = NIO_TRANSACTION_PORT_REASON_NONE;
    std::uint8_t lsr = 0;
};

struct FakePort {
    std::vector<Event> events;
    std::vector<std::uint8_t> writes;
    std::size_t read_index = 0;
    std::uint8_t last_reason = NIO_TRANSACTION_PORT_REASON_NONE;
    std::uint8_t last_lsr = 0;
    unsigned flush_count = 0;
    unsigned wait_count = 0;
    unsigned read_count = 0;
};

struct Fixture {
    std::array<std::uint8_t, 32> tx_prefix{};
    nio_frame_header_t rx_header{};
    std::array<std::uint8_t, 64> rx_payload{};
    std::array<std::uint8_t, 64> reply{};
    nio_response_t response{};
    nio_transaction_diag_t diag{};
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

void fake_flush_rx(void *ctx)
{
    auto *fake = static_cast<FakePort *>(ctx);
    fake->flush_count++;
}

int fake_putc(void *ctx, std::uint8_t c)
{
    auto *fake = static_cast<FakePort *>(ctx);
    fake->writes.push_back(c);
    return c;
}

std::uint16_t fake_putbuf_slip(void *ctx, const void *buf, std::uint16_t len)
{
    auto *fake = static_cast<FakePort *>(ctx);
    const auto *bytes = static_cast<const std::uint8_t *>(buf);
    fake->writes.insert(fake->writes.end(), bytes, bytes + len);
    return len;
}

void fake_wait_tx_empty(void *ctx)
{
    auto *fake = static_cast<FakePort *>(ctx);
    fake->wait_count++;
}

std::uint16_t fake_getbuf_slip_dual(void *ctx, void *hdr_buf, std::uint16_t hdr_len,
                                    void *data_buf, std::uint16_t data_len,
                                    std::uint16_t timeout)
{
    auto *fake = static_cast<FakePort *>(ctx);
    fake->read_count++;
    CHECK(timeout == 2000);

    if (fake->read_index >= fake->events.size()) {
        fake->last_reason = NIO_TRANSACTION_PORT_REASON_TIMEOUT;
        fake->last_lsr = 0;
        return 0;
    }

    const auto& event = fake->events[fake->read_index++];
    fake->last_reason = event.reason;
    fake->last_lsr = event.lsr;
    std::memcpy(hdr_buf, &event.frame.header, hdr_len);
    std::memcpy(data_buf, event.frame.payload.data(),
                std::min<std::uint16_t>(data_len, event.frame.rx_len - NIO_FRAME_HEADER_SIZE));
    return event.frame.rx_len;
}

std::uint8_t fake_last_reason(void *ctx)
{
    return static_cast<FakePort *>(ctx)->last_reason;
}

std::uint8_t fake_last_lsr(void *ctx)
{
    return static_cast<FakePort *>(ctx)->last_lsr;
}

nio_transaction_port_t make_port(FakePort& fake)
{
    nio_transaction_port_t port = {};
    port.ctx = &fake;
    port.flush_rx = fake_flush_rx;
    port.putc = fake_putc;
    port.putbuf_slip = fake_putbuf_slip;
    port.wait_tx_empty = fake_wait_tx_empty;
    port.getbuf_slip_dual = fake_getbuf_slip_dual;
    port.last_slip_reason = fake_last_reason;
    port.last_lsr = fake_last_lsr;
    return port;
}

nio_transaction_buffers_t make_buffers(Fixture& fixture)
{
    nio_transaction_buffers_t buffers = {};
    buffers.tx_prefix = fixture.tx_prefix.data();
    buffers.tx_prefix_capacity = fixture.tx_prefix.size();
    buffers.rx_header = &fixture.rx_header;
    buffers.rx_payload = fixture.rx_payload.data();
    buffers.rx_payload_capacity = fixture.rx_payload.size();
    return buffers;
}

bool call_transaction(FakePort& fake, Fixture& fixture, std::uint8_t retries)
{
    auto port = make_port(fake);
    auto buffers = make_buffers(fixture);
    const std::array<std::uint8_t, 2> request = {0x41, 0x42};

    return nio_transaction_call(&port, &buffers, DEV_NETWORK, CMD_READ,
                                request.data(), request.size(),
                                fixture.reply.data(), fixture.reply.size(),
                                &fixture.response, retries,
                                2000, 15000, 2, &fixture.diag);
}

} // namespace

TEST_CASE("NIO transaction builds and sends a checked request frame")
{
    FakePort fake;
    Fixture fixture;
    fake.events.push_back({make_response(DEV_NETWORK, CMD_READ, {STATUS_OK, 0x77})});

    CHECK(call_transaction(fake, fixture, 0));

    REQUIRE(fake.writes.size() >= NIO_FRAME_HEADER_SIZE + 4);
    CHECK(fake.writes.front() == NIO_TRANSACTION_SLIP_END);
    CHECK(fake.writes.back() == NIO_TRANSACTION_SLIP_END);

    const auto *header = reinterpret_cast<const nio_frame_header_t *>(&fake.writes[1]);
    CHECK(header->device == DEV_NETWORK);
    CHECK(header->command == CMD_READ);
    CHECK(header->length == NIO_FRAME_HEADER_SIZE + 2);
    CHECK(header->fields == NIO_PROTO_FIELD_NONE);
    CHECK(fake.writes[1 + NIO_FRAME_HEADER_SIZE] == 0x41);
    CHECK(fake.writes[1 + NIO_FRAME_HEADER_SIZE + 1] == 0x42);

    auto checksum_header = *header;
    auto checksum = header->checksum;
    checksum_header.checksum = 0;
    CHECK(checksum == static_cast<std::uint8_t>(
        nio_protocol_checksum(&fake.writes[1 + NIO_FRAME_HEADER_SIZE], 2,
            nio_protocol_checksum(&checksum_header, sizeof(checksum_header), 0))));
}

TEST_CASE("NIO transaction retries after a timeout and preserves final diagnostics")
{
    FakePort fake;
    Fixture fixture;
    Event timeout;
    timeout.frame.rx_len = 0;
    timeout.reason = NIO_TRANSACTION_PORT_REASON_TIMEOUT;
    fake.events.push_back(timeout);
    fake.events.push_back({make_response(DEV_NETWORK, CMD_READ, {STATUS_OK, 0xaa, 0xbb})});

    CHECK(call_transaction(fake, fixture, 1));
    CHECK(fake.flush_count == 2);
    CHECK(fake.wait_count == 2);
    CHECK(fake.read_count == 2);
    CHECK(fixture.diag.error == NIO_ERR_NONE);
    CHECK(fixture.response.status == STATUS_OK);
    CHECK(fixture.response.payload_length == 2);
    CHECK(fixture.reply[0] == 0xaa);
    CHECK(fixture.reply[1] == 0xbb);
}

TEST_CASE("NIO transaction retries after a bad response checksum")
{
    FakePort fake;
    Fixture fixture;
    auto bad = make_response(DEV_NETWORK, CMD_READ, {STATUS_OK, 0x10});
    bad.payload[1] ^= 0x55;
    fake.events.push_back({bad});
    fake.events.push_back({make_response(DEV_NETWORK, CMD_READ, {STATUS_OK, 0xcc})});

    CHECK(call_transaction(fake, fixture, 1));
    CHECK(fake.flush_count == 2);
    CHECK(fake.wait_count == 2);
    CHECK(fake.read_count == 2);
    CHECK(fixture.diag.error == NIO_ERR_NONE);
    CHECK(fixture.response.payload_length == 1);
    CHECK(fixture.reply[0] == 0xcc);
}

TEST_CASE("NIO transaction discards stale response without resending request")
{
    FakePort fake;
    Fixture fixture;
    fake.events.push_back({make_response(DEV_NETWORK, CMD_WRITE, {STATUS_OK, 0x01})});
    fake.events.push_back({make_response(DEV_NETWORK, CMD_READ, {STATUS_OK, 0x02})});

    CHECK(call_transaction(fake, fixture, 0));
    CHECK(fake.flush_count == 1);
    CHECK(fake.wait_count == 1);
    CHECK(fake.read_count == 2);
    CHECK(fixture.diag.error == NIO_ERR_NONE);
    CHECK(fixture.response.payload_length == 1);
    CHECK(fixture.reply[0] == 0x02);
}

TEST_CASE("NIO transaction stops after stale response budget is exhausted")
{
    FakePort fake;
    Fixture fixture;
    fake.events.push_back({make_response(DEV_NETWORK, CMD_WRITE, {STATUS_OK})});
    fake.events.push_back({make_response(DEV_NETWORK, CMD_WRITE, {STATUS_OK})});
    fake.events.push_back({make_response(DEV_NETWORK, CMD_WRITE, {STATUS_OK})});

    CHECK_FALSE(call_transaction(fake, fixture, 0));
    CHECK(fake.flush_count == 1);
    CHECK(fake.read_count == 3);
    CHECK(fixture.diag.error == NIO_ERR_DEVICE_COMMAND);
}

TEST_CASE("NIO transaction does not retry non-retry protocol errors")
{
    FakePort fake;
    Fixture fixture;
    auto frame = make_response(DEV_NETWORK, CMD_READ, {STATUS_OK, 0x10});
    frame.header.fields = 0;
    frame.header.checksum = 0;
    frame.header.checksum = static_cast<std::uint8_t>(
        nio_protocol_checksum(frame.payload.data(), 2,
            nio_protocol_checksum(&frame.header, sizeof(frame.header), 0)));
    fake.events.push_back({frame});
    fake.events.push_back({make_response(DEV_NETWORK, CMD_READ, {STATUS_OK})});

    CHECK_FALSE(call_transaction(fake, fixture, 2));
    CHECK(fake.flush_count == 1);
    CHECK(fake.read_count == 1);
    CHECK(fixture.diag.error == NIO_ERR_FIELDS);
}
