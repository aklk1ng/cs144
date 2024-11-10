#include "reassembler.hh"
#include <algorithm>
#include <ranges>

using namespace std;

auto Reassembler::split(uint64_t pos) noexcept {
    auto it = buffer.lower_bound(pos);
    if (it != buffer.end() && it->first == pos) {
        return it;
    }

    if (it == buffer.begin()) {
        return it;
    }

    const auto pre = prev(it);
    if (pre->first + pre->second.size() > pos) {
        const auto res = buffer.emplace_hint(it, pos, pre->second.substr(pos - pre->first));
        pre->second.resize(pos - pre->first);
        return res;
    }
    return it;
}

void Reassembler::insert(uint64_t first_index, string data, bool is_last_substring) {
    const auto try_close = [&]() noexcept -> void {
        if (next_index.has_value() && next_index.value() == writer().bytes_pushed()) {
            output_.writer().close();
        }
    };

    if (data.empty()) {
        if (!next_index.has_value() && is_last_substring) {
            next_index.emplace(first_index);
        }
        try_close();
    }

    if (writer().is_closed() || !writer().available_capacity()) {
        return;
    }

    const uint64_t unassembled_index = writer().bytes_pushed();
    const uint64_t unacceptable_index = unassembled_index + writer().available_capacity();
    if (first_index + data.size() <= unassembled_index || first_index >= unacceptable_index) {
        return;
    }

    // Remove unacceptable bytes
    if (first_index + data.size() > unacceptable_index) {
        data.resize(unacceptable_index - first_index);
        is_last_substring = false;
    }
    // Remove poped/buffered bytes
    if (first_index < unassembled_index) {
        data.erase(0, unassembled_index - first_index);
        first_index = unassembled_index;
    }

    if (!next_index.has_value() && is_last_substring) {
        next_index.emplace(first_index + data.size());
    }

    const auto upper = split(first_index + data.size());
    const auto lower = split(first_index);
    ranges::for_each(ranges::subrange(lower, upper) | views::values,
                     [&](const auto &str) { bytes_pending_ -= str.size(); });
    bytes_pending_ += data.size();
    buffer.emplace_hint(buffer.erase(lower, upper), first_index, move(data));
    while (!buffer.empty()) {
        auto &&[index, payload] = *buffer.begin();
        if (index != writer().bytes_pushed()) {
            break;
        }

        bytes_pending_ -= payload.size();
        output_.writer().push(move(payload));
        buffer.erase(buffer.begin());
    }
    try_close();
}

uint64_t Reassembler::bytes_pending() const { return bytes_pending_; }
