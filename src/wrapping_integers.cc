#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap(uint64_t n, Wrap32 zero_point) { return zero_point + static_cast<uint32_t>(n); }

uint64_t Wrap32::unwrap(Wrap32 zero_point, uint64_t checkpoint) const {
    const uint64_t seqno_offset = raw_value_ - zero_point.raw_value_;
    if (checkpoint > seqno_offset) {
        const uint64_t abs_check_seq = checkpoint - seqno_offset + (BASE / 2);
        const uint64_t times = abs_check_seq / BASE;
        return times * BASE + seqno_offset;
    }
    return seqno_offset;
}
