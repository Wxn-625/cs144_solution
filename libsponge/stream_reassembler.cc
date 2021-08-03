#include "stream_reassembler.hh"
#include "iostream"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {
    _nowIndex = 0;
    _sizeOfUnassembledBytes = 0;
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // If the data has been writen in,just discard it,
    // and we will accept index + data.size() == _nowIndex,
    // because it may carry the info about eof.
    if(index + data.size() < _nowIndex)
        return;

    std::string newStr,ttp;
    size_t newIndex;
    ttp = data;

    // If the index < _nowIndex,truncate the str
    if(index < _nowIndex){
        newStr = data.substr(_nowIndex-index);
        newIndex = _nowIndex;
    }
    else{
        newStr = data;
        newIndex = index;
    }


//    if(newIndex == _nowIndex){
//        _output.write(newStr);
//        if(eof){
//            _output.end_input();
//        }
//
//        _nowIndex = newIndex + newStr.size();
//    }
//    else
        merge_inorderedStr(newStr,newIndex,eof);

    // If the first str can be write in.
    // Because the eof may cause the break part.
    while(!_inorderedStr.empty() && _inorderedStr.begin()->_index == _nowIndex){
        auto it = _inorderedStr.begin();

        _output.write(it -> _data);
        if(it -> _eof){
            _output.end_input();
        }

        _nowIndex = it -> _index + it -> _data.size();
        _sizeOfUnassembledBytes -= it -> _data.size();
        _inorderedStr.erase(it);
    }

}

size_t StreamReassembler::unassembled_bytes() const { return _sizeOfUnassembledBytes; }

bool StreamReassembler::empty() const { return _sizeOfUnassembledBytes == 0; }

size_t StreamReassembler::remaining_capacity() const{
    return _output.remaining_capacity() - _sizeOfUnassembledBytes;
}