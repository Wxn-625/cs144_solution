#include "tcp_receiver.hh"
#include "iostream"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    uint64_t seq_tmp = 0;
    if(_isInit && seg.header().syn)
        return;
    if(!_isInit){
        if(!seg.header().syn){
            return;
        }
        _isn = seg.header().seqno;
        _isInit = true;
        seq_tmp = 1;
        checkpoint = 1;
        if(seg.payload().size() == 0){
            expt_seq = 1;

            if(seg.header().fin){
                _reassembler.push_substring("",0, true);
                expt_seq++;
            }
            return;
        }
    }
    else{
        seq_tmp = unwrap(seg.header().seqno,_isn,checkpoint);
        // If out of the window.
        if(seq_tmp + seg.length_in_sequence_space() <= getExpt_seq() || seq_tmp >= getExpt_seq() + window_size())
            return;
    }



    checkpoint = seq_tmp;
    const std::string data = seg.payload().copy();
    const bool eof = seg.header().fin;

    _reassembler.push_substring(data,checkpoint - 1,eof);
    expt_seq = _reassembler.getIndex() + 1;
    if(stream_out().get_end_input_flag()){
        expt_seq ++;
    }


}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(!_isInit)
        return nullopt;
    return  wrap(expt_seq,_isn) ;
}

size_t TCPReceiver::window_size() const { return stream_out().remaining_capacity(); }
