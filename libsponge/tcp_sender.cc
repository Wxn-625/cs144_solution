#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include "iostream"

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    { _now_retransmission_timeout = _initial_retransmission_timeout;}

uint64_t TCPSender::bytes_in_flight() const {
    return _next_seqno - _ack_seqno; }

void TCPSender::fill_window() {
    while(_sender_window_size){
        // Get the send size.
//        size_t  send_size = std::min(_sender_window_size,TCPConfig::MAX_PAYLOAD_SIZE);
        size_t  send_size  = std::max(_sender_window_size,1ul);

        // If it is the first byte,SYN will cost 1 byte.
        if(_next_seqno == 0)
            --send_size;


        Buffer buf = Buffer(stream_in().read(std::min(TCPConfig::MAX_PAYLOAD_SIZE,send_size)));

        // Get the tcpSegment
        TCPSegment tcpSegment = TCPSegment();
        tcpSegment.payload() = buf;
        tcpSegment.header().seqno = wrap(_next_seqno,_isn);

        // Add the SYN flag
        if(_next_seqno == 0){
            tcpSegment.header().syn = true;
        }

        // Add the FIN flag,when the stram_in is read out.
        if(tcpSegment.length_in_sequence_space() < send_size && stream_in().eof()){
            tcpSegment.header().fin = true;
        }

        if(tcpSegment.length_in_sequence_space() == 0 || (hasFIN && tcpSegment.length_in_sequence_space() == 1 && tcpSegment.header().fin))
            return;

        // start the timer
        if(!_isRunning){
            _isRunning = true;
            _timer = 0;
        }


        if(tcpSegment.header().fin)
            hasFIN = true;

        _segments_out.push(tcpSegment);
        _segments_storage.push(tcpSegment);


        if(_sender_window_size >= tcpSegment.length_in_sequence_space())
            _sender_window_size -=  tcpSegment.length_in_sequence_space();
        _next_seqno += tcpSegment.length_in_sequence_space();
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    const uint64_t old_ack_seqno = _ack_seqno;
    const uint64_t tmp = unwrap(ackno,_isn,_ack_seqno);

    // If ack > _next_seq,discard it.
    if(tmp > _next_seqno)
        return;
    // If ack < _ack_seqno,discard it.
    if(tmp >= _ack_seqno)
        _ack_seqno = tmp;
    else
        return;

    // Modify the _window_size and _ack_seqno.
    _window_size = window_size;
    if(_next_seqno - tmp <= _window_size)
        _sender_window_size = _window_size - (_next_seqno - tmp);

    //If the _window_size == 0,let _sender_window_size be 1.
    if(_window_size == 0)
        _sender_window_size = 1;

    // Delete the ack segment storage.
    while(!_segments_storage.empty()){
        auto seg = _segments_storage.front();
        uint64_t index = unwrap(seg.header().seqno,_isn,_ack_seqno);
        if(_ack_seqno >= index + seg.length_in_sequence_space())
            _segments_storage.pop();
        else
            break;
    }

    if(_ack_seqno > old_ack_seqno){
        _now_retransmission_timeout = _initial_retransmission_timeout;
        _num_consecutive_retransmissions = 0;
        if(!_segments_storage.empty())
            _timer = 0;
    }

    // If no storage,stop the timer
    if(_segments_storage.empty()){
        _timer = 0;
        _isRunning = false;
    }



}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // If the timer is not running,return;
    if(!_isRunning)
        return;
    _timer += ms_since_last_tick;
    if(_timer >= _now_retransmission_timeout){
        _segments_out.push(_segments_storage.front());
        _timer = 0;

        if(_window_size != 0){
            _num_consecutive_retransmissions++;
            _now_retransmission_timeout *= 2;
        }

    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _num_consecutive_retransmissions; }

void TCPSender::send_empty_segment(bool rst) {
    TCPSegment tcpSegment;
    tcpSegment.header().rst = rst;
    tcpSegment.header().seqno = wrap(next_seqno_absolute(),_isn);
    _segments_out.push(tcpSegment);
}
