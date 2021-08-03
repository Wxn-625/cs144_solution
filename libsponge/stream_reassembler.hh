#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <set>
#include <string>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.
    class StoreSubstr{
      public:
        std::string _data{};
        size_t _index{};
        bool _eof{};

        StoreSubstr(std::string data,const size_t index,const bool eof){
            _data = std::move(data);
            _index = index;
            _eof = eof;
        }
        bool operator < (const StoreSubstr &b) const{
            return _index == b._index ? _index + _data.size() < b._index + b._data.size() : _index < b._index;
        }
    };

    size_t _nowIndex{};    //!< The index needed now
    std::set<StoreSubstr> _inorderedStr{};    //!< Store the inordered substr
    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity{};    //!< The maximum number of bytes
    size_t _sizeOfUnassembledBytes{};

    size_t remaining_capacity() const;

    //!< Insert the new Str in the storage
    void merge_inorderedStr(const std::string &data, const uint64_t index, const bool eof){
        StoreSubstr tmp = StoreSubstr(data,index,eof);
        _inorderedStr.insert(tmp);
        _sizeOfUnassembledBytes += data.size();

        auto it = _inorderedStr.find(tmp);
        size_t la = 0;
        std::string str;
        uint64_t newIndex;
        bool newEof = false;

        // merge front one
        if(it != _inorderedStr.begin()){
            it--;
            if(it->_index + it->_data.size() <= index)
                it++;
        }
        newIndex = it->_index;
        // for erase
        auto beg = it;

        while(true){
            // copy to new str
            la = std::max(it->_index + it->_data.size(),la);
            for(size_t i = newIndex + str.size() - it->_index;i < it->_data.size();i++)
                str.push_back(it->_data[i]);

            _sizeOfUnassembledBytes -= it->_data.size();
//        if(it->_eof){
//            // [begin,end)
//            it++;
//            newEof = true;
//            break;
//        }
            newEof = it->_eof;

            it++;
            // If not continuous,break;
            if(it->_index > la || (it -> _index == la && it ->_data.size() == 0))
                break;
        }
        // erase the merged ones
        _inorderedStr.erase(beg,it);

        _inorderedStr.insert(StoreSubstr(str,newIndex,newEof));
        _sizeOfUnassembledBytes += str.size();

        // delete the oversize part
        while(_output.remaining_capacity() < _sizeOfUnassembledBytes){
            size_t oversize = _sizeOfUnassembledBytes - _output.remaining_capacity();
            it = _inorderedStr.cbegin();

            // If it is enough,delete part of it
            if(it -> _data.size() > oversize){
                size_t oriSize = it->_data.size();
                std::string tmp_val = it -> _data.substr(0,oriSize - oversize);
                _inorderedStr.insert(StoreSubstr(tmp_val,it->_index,false));
                _sizeOfUnassembledBytes -= oversize;
                _inorderedStr.erase(it);
            }
                // else delete it;
            else{
                _sizeOfUnassembledBytes -= it->_data.size();
                _inorderedStr.erase(it);
            }
        }
    }
  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;

    uint64_t getIndex() const{return _nowIndex;};
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
