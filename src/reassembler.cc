#include "reassembler.hh"
#include <algorithm>
#include <ranges>

using namespace std;

auto Reassembler::split(uint64_t pos) noexcept {
    auto it = this->buffer.lower_bound(pos);
    if (it != this->buffer.end() && it->first == pos) {
        return it;
    }

    if (it == this->buffer.begin()) {
        return it;
    }

    const auto pre = prev(it);
    if (pre->first + pre->second.size() > pos) {
        const auto res = this->buffer.emplace_hint(it, pos, pre->second.substr(pos - pre->first));
        pre->second.resize(pos - pre->first);
        return res;
    }
    return it;
}

void Reassembler::insert(uint64_t first_index, string data, bool is_last_substring) {
    const auto try_close = [&]() noexcept -> void {
        if (this->next_index.has_value() && this->next_index.value() == this->writer().bytes_pushed()) {
            this->output_.writer().close();
        }
    };

    if (data.empty()) {
        if (!this->next_index.has_value() && is_last_substring) {
            this->next_index.emplace(first_index);
        }
        try_close();
    }

    if (this->writer().is_closed() || !this->writer().available_capacity()) {
        return;
    }

    const uint64_t unassembled_index = this->writer().bytes_pushed();
    const uint64_t unacceptable_index = unassembled_index + this->writer().available_capacity();
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

    if (!this->next_index.has_value() && is_last_substring) {
        this->next_index.emplace(first_index + data.size());
    }

    const auto upper = split(first_index + data.size());
    const auto lower = split(first_index);
    ranges::for_each(ranges::subrange(lower, upper) | views::values,
                     [&](const auto &str) { this->bytes_pending_ -= str.size(); });
    this->bytes_pending_ += data.size();
    this->buffer.emplace_hint(this->buffer.erase(lower, upper), first_index, move(data));
    while (!this->buffer.empty()) {
        auto &&[index, payload] = *this->buffer.begin();
        if (index != this->writer().bytes_pushed()) {
            break;
        }

        this->bytes_pending_ -= payload.size();
        this->output_.writer().push(move(payload));
        this->buffer.erase(this->buffer.begin());
    }
    try_close();
}

uint64_t Reassembler::bytes_pending() const { return this->bytes_pending_; }
