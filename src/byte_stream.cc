#include "byte_stream.hh"
#include <iostream>

using namespace std;

ByteStream::ByteStream(uint64_t capacity)
    : capacity_(capacity), buffer(capacity + 1, '\0'), w_nbytes(0), r_nbytes(0) {}

bool Writer::is_closed() const { return this->w_close; }

void Writer::push(string data) {
    if (this->is_closed()) {
        return;
    }

    uint64_t n = data.size();
    // If the rest bytes is not enough, move str to the start of buffer.
    if (this->end_ + n > this->capacity_) {
        buffer.replace(0, this->end_ - this->st_, buffer, this->st_, this->end_ - this->st_);
        this->end_ = this->end_ - this->st_;
        this->st_ = 0;
    }

    if (this->end_ + n <= this->capacity_) {
        buffer.replace(this->end_, n, data);
        this->w_nbytes += n;
        this->end_ += n;
    } else {
        buffer.replace(this->end_, this->capacity_ - this->end_, data, 0, this->capacity_ - this->end_);
        this->w_nbytes += this->capacity_ - this->end_;
        this->end_ = this->capacity_;
    }
}

void Writer::close() { this->w_close = true; }

uint64_t Writer::available_capacity() const { return this->capacity_ - (this->end_ - this->st_); }

uint64_t Writer::bytes_pushed() const { return this->w_nbytes; }

bool Reader::is_finished() const { return this->w_close && (this->st_ == this->end_); }

uint64_t Reader::bytes_popped() const { return this->r_nbytes; }

string_view Reader::peek() const {
    return std::string_view(this->buffer.data() + this->st_, this->end_ - this->st_);
}

void Reader::pop(uint64_t len) {
    if (this->st_ + len <= this->end_) {
        this->st_ += len;
        r_nbytes += len;
    } else {
        std::cerr << "len out of range!\n";
        this->st_ = this->end_;
        r_nbytes += (this->end_ - this->st_);
    }
}

uint64_t Reader::bytes_buffered() const { return this->end_ - this->st_; }
