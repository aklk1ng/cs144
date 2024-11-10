#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive(TCPSenderMessage message) {
    if (this->writer().has_error()) {
        return;
    }
    if (message.RST) {
        this->reader().set_error();
        return;
    }
    if (!this->zero_point.has_value()) {
        if (!message.SYN) {
            return;
        }
        zero_point.emplace(message.seqno);
    }

    const uint64_t checkpoint = this->writer().bytes_pushed() + 1;
    const uint64_t abs_seqno = message.seqno.unwrap(this->zero_point.value(), checkpoint);
    const uint64_t stream_index = abs_seqno + static_cast<uint64_t>(message.SYN) - 1;
    this->reassembler_.insert(stream_index, std::move(message.payload), message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const {
    const uint16_t window_size = this->writer().available_capacity() > UINT16_MAX
                                     ? static_cast<uint16_t>(UINT16_MAX)
                                     : static_cast<uint16_t>(this->writer().available_capacity());
    if (this->zero_point.has_value()) {
        const uint64_t ack = this->writer().bytes_pushed() + 1 + static_cast<uint64_t>(this->writer().is_closed());
        return {Wrap32::wrap(ack, this->zero_point.value()), window_size, this->writer().has_error()};
    }
    return {nullopt, window_size, this->writer().has_error()};
}
