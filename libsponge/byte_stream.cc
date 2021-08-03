#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) {
    cap = capacity;
    num_of_write = num_of_read = 0;
    end_input_flag = false;
    _error = false;
}

size_t ByteStream::write(const string &data) {
    size_t ret = 0;
    ret = min(remaining_capacity(),data.size());
    for(int i = 0,limit = static_cast<int>(ret);i < limit;i++){
        buf.push_back(data[i]);
    }
    // if write is not end,modify the flag
    num_of_write += ret;
    end_input_flag = false;

    return ret;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t lenth = min(len,buffer_size());
    return string().assign(buf.begin(),buf.begin() + lenth);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t lenth = min(len,buffer_size());
    for(int i = 0,limit = static_cast<int> (lenth);i < limit;i++){
        buf.pop_front();
    }
    num_of_read += lenth;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t sz = min(buffer_size(),len);
    string ret = peek_output(sz);
    pop_output(sz);
    return ret;
}

void ByteStream::end_input() { end_input_flag = true; }

bool ByteStream::input_ended() const { return end_input_flag; }

size_t ByteStream::buffer_size() const { return buf.size(); }

bool ByteStream::buffer_empty() const {
    return buffer_size() == 0; }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return num_of_write; }

size_t ByteStream::bytes_read() const {return num_of_read;}

size_t ByteStream::remaining_capacity() const {
    return cap - buffer_size();
}