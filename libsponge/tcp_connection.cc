#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return outbound_stream().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    _time_since_last_segment_received = 0;

    // RST set
    if(seg.header().rst){
        comeRST();
        return;
    }

    // Ignore ACK,when in LISTEN.
    if(!seg.header().syn && seg.header().ack && !_receiver._isInit){
        return;
    }


    //! Test the case: the segment will increment the receiver's expt_seq
    _receiver.segment_received(seg);

    // Set the _linger_after_streams_finish.
    if(inbound_stream().get_end_input_flag() && !outbound_stream().eof())
        _linger_after_streams_finish = false;


    if(seg.header().ack){
        _sender.ack_received(seg.header().ackno,seg.header().win);
    }
    _sender.fill_window();

    // If no segment to be sent,send an empty one.
    if(seg.length_in_sequence_space() && _sender._segments_out.empty()){
        _sender.send_empty_segment();
    }

    // Add the ackno and window_size on the segment to be sent.
    sendAll();

    // Passive close
    if(!_linger_after_streams_finish && inbound_stream().get_end_input_flag() && outbound_stream().eof() && _sender._next_seqno == _sender._ack_seqno){
        _isActive = false;
    }

}

bool TCPConnection::active() const { return _isActive; }

size_t TCPConnection::write(const string &data) {

    size_t ret = outbound_stream().write(data);
    _sender.fill_window();
    sendAll();
    if(!_linger_after_streams_finish && inbound_stream().get_end_input_flag() && outbound_stream().eof() && _sender._next_seqno == _sender._ack_seqno){
        _isActive = false;
    }
    return ret;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _sender.tick(ms_since_last_tick);

    // Exceed the limit,abort the connection.
    if(_sender._num_consecutive_retransmissions > TCPConfig::MAX_RETX_ATTEMPTS){
        while(!_sender._segments_out.empty())
            _sender._segments_out.pop();
        _sender.send_empty_segment(true);
        sendAll();

        // Set the local peer state.
        comeRST();
    }

    // Send the TCPSegments.
    sendAll();

    _time_since_last_segment_received += ms_since_last_tick;


    // Clean shutdown.
    if(_time_since_last_segment_received >= 10 * _cfg.rt_timeout &&  inbound_stream().get_end_input_flag() && outbound_stream().eof() && _sender._next_seqno == _sender._ack_seqno){
        _isActive = false;
    }
}

void TCPConnection::end_input_stream() {
    outbound_stream().end_input();
    _sender.fill_window();
    sendAll();
    if(!_linger_after_streams_finish && inbound_stream().get_end_input_flag() && outbound_stream().eof() && _sender._next_seqno == _sender._ack_seqno){
        _isActive = false;
    }
}

void TCPConnection::connect() {
    _sender.fill_window();
    if(_sender._segments_out.size() != 1){
        return;
    }

    //! There is no need to send window_size and ack?
    _segments_out.push(_sender._segments_out.front());
    _sender._segments_out.pop();
}

void TCPConnection::sendAll(){
    while(!_sender._segments_out.empty()){
        _sender._segments_out.front().header().ackno = wrap(_receiver.getExpt_seq(),_receiver.get_isn());
        _sender._segments_out.front().header().win = _receiver.window_size();
        // Set ACK flag
        if(_receiver.getExpt_seq() != 0)
            _sender._segments_out.front().header().ack = true;
        _segments_out.push(_sender._segments_out.front());
        _sender._segments_out.pop();
    }
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _sender.send_empty_segment(true);
            // Set the local peer state.
            comeRST();

            // Send the TCPSegments.
            sendAll();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
