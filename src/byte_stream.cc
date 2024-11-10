#include "byte_stream.hh"
#include <iostream>

using namespace std;

ByteStream::ByteStream(uint64_t capacity)
    : capacity_(capacity), buffer(capacity + 1, '\0'), w_nbytes(0), r_nbytes(0) {}

bool Writer::is_closed() const { return w_close; }

void Writer::push(string data) {
    if (is_closed()) {
        return;
    }

    uint64_t n = data.size();
    // If the rest bytes is not enough, move str to the start of buffer.
    if (end_ + n > capacity_) {
        buffer.replace(0, end_ - st_, buffer, st_, end_ - st_);
        end_ = end_ - st_;
        st_ = 0;
    }

    if (end_ + n <= capacity_) {
        buffer.replace(end_, n, data);
        w_nbytes += n;
        end_ += n;
    } else {
        buffer.replace(end_, capacity_ - end_, data, 0, capacity_ - end_);
        w_nbytes += capacity_ - end_;
        end_ = capacity_;
    }
}

void Writer::close() { w_close = true; }

uint64_t Writer::available_capacity() const { return capacity_ - (end_ - st_); }

uint64_t Writer::bytes_pushed() const { return w_nbytes; }

bool Reader::is_finished() const { return w_close && (st_ == end_); }

uint64_t Reader::bytes_popped() const { return r_nbytes; }

string_view Reader::peek() const { return std::string_view(buffer.data() + st_, end_ - st_); }

void Reader::pop(uint64_t len) {
    if (st_ + len <= end_) {
        st_ += len;
        r_nbytes += len;
    } else {
        std::cerr << "len out of range!\n";
        st_ = end_;
        r_nbytes += (end_ - st_);
    }
}

uint64_t Reader::bytes_buffered() const { return end_ - st_; }
