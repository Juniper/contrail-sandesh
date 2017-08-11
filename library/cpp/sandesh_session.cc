/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_session.cc
//
// Sandesh session
//

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#endif

#include <boost/bind.hpp>
#include <boost/assign.hpp>
#include <boost/algorithm/string.hpp>

#include <base/parse_object.h>

#include <sandesh/common/vns_types.h>
#include <sandesh/common/vns_constants.h>
#include <sandesh/transport/TBufferTransports.h>
#include <sandesh/protocol/TXMLProtocol.h>
#include "sandesh/sandesh_types.h"
#include "sandesh/sandesh.h"

#include "sandesh_connection.h"
#include "sandesh_session.h"


using namespace std;
using namespace contrail::sandesh::protocol;
using namespace contrail::sandesh::transport;

using boost::asio::mutable_buffer;
using boost::asio::buffer_cast;

const std::string SandeshWriter::sandesh_open_ = sXML_SANDESH_OPEN;
const std::string SandeshWriter::sandesh_open_attr_length_ =
        sXML_SANDESH_OPEN_ATTR_LENGTH;
const std::string SandeshWriter::sandesh_close_ = sXML_SANDESH_CLOSE;

//
// SandeshWriter
//
SandeshWriter::SandeshWriter(SandeshSession *session)
    : session_(session),
    ready_to_send_(true),
    send_buf_(new uint8_t[kDefaultSendSize]),
    send_buf_offset_(0) {
}

SandeshWriter::~SandeshWriter() {
    delete [] send_buf_;
}

void SandeshWriter::WriteReady(const boost::system::error_code &ec) {
    if (ec) {
        SANDESH_LOG(ERROR, "SandeshSession Write error value: " << ec.value()
            << " category: " << ec.category().name()
            << " message: " << ec.message());
        session_->increment_write_ready_cb_error();
        return;
    }

    {
        tbb::mutex::scoped_lock lock(send_mutex_);
        ready_to_send_ = true;
    }

    // We may want to start the Runner for the send_queue
    session_->send_queue()->MayBeStartRunner();
}

void SandeshWriter::SendMsg(Sandesh *sandesh, bool more) {
    SandeshHeader header;
    std::stringstream ss;
    uint8_t *buffer;
    int32_t xfer = 0, ret;
    uint32_t offset;
    boost::shared_ptr<TMemoryBuffer> btrans(
                    new TMemoryBuffer(kEncodeBufferSize));
    boost::shared_ptr<TXMLProtocol> prot(
                    new TXMLProtocol(btrans));
    // Populate the header
    header.set_Namespace(sandesh->scope());
    header.set_Timestamp(sandesh->timestamp());
    header.set_Module(sandesh->module());
    header.set_Source(sandesh->source());
    header.set_Context(sandesh->context());
    header.set_SequenceNum(sandesh->seqnum());
    header.set_VersionSig(sandesh->versionsig());
    header.set_Type(sandesh->type());
    header.set_Hints(sandesh->hints());
    header.set_Level(sandesh->level());
    header.set_Category(sandesh->category());
    header.set_NodeType(sandesh->node_type());
    header.set_InstanceId(sandesh->instance_id());

    // Write the sandesh open envelope.
    buffer = btrans->getWritePtr(sandesh_open_.length());
    memcpy(buffer, sandesh_open_.c_str(), sandesh_open_.length());
    btrans->wroteBytes(sandesh_open_.length());
    // Write the sandesh header
    if ((ret = header.write(prot)) < 0) {
        SANDESH_LOG(ERROR, __func__ << ": Sandesh header write FAILED: " <<
            sandesh->Name() << " : " << sandesh->source() << ":" <<
            sandesh->module() << ":" << sandesh->instance_id() <<
            " Sequence Number:" << sandesh->seqnum());
        session_->increment_send_msg_fail();
        Sandesh::UpdateTxMsgFailStats(sandesh->Name(), 0,
            SandeshTxDropReason::HeaderWriteFailed);
        sandesh->Release();
        return;
    }
    xfer += ret;
    // Write the sandesh
    if ((ret = sandesh->Write(prot)) < 0) {
        SANDESH_LOG(ERROR, __func__ << ": Sandesh write FAILED: "<<
            sandesh->Name() << " : " << sandesh->source() << ":" <<
            sandesh->module() << ":" << sandesh->instance_id() <<
            " Sequence Number:" << sandesh->seqnum());
        session_->increment_send_msg_fail();
        Sandesh::UpdateTxMsgFailStats(sandesh->Name(), 0,
            SandeshTxDropReason::WriteFailed);
        sandesh->Release();
        return;
    }
    xfer += ret;
    // Write the sandesh close envelope
    buffer = btrans->getWritePtr(sandesh_close_.length());
    memcpy(buffer, sandesh_close_.c_str(), sandesh_close_.length());
    btrans->wroteBytes(sandesh_close_.length());
    // Get the buffer
    btrans->getBuffer(&buffer, &offset);
    // Sanity
    assert(sandesh_open_.length() + xfer + sandesh_close_.length() ==
            offset);
    // Update the sandesh open envelope length;
    char prev = ss.fill('0');
    // Adjust for '">'
    ss.width(sandesh_open_.length() - sandesh_open_attr_length_.length() - 2);
    ss << offset;
    ss.fill(prev);
    memcpy(buffer + sandesh_open_attr_length_.length(), ss.str().c_str(),
            ss.str().length());

    // Update sandesh stats
    Sandesh::UpdateTxMsgStats(sandesh->Name(), offset);
    session_->increment_send_msg();

    if (send_buf()) {
        if (more) {
            // There are more messages in the send_queue_. 
            // Try to package as many sandesh messages as possible 
            // (== kEncodeBufferSize) before transporting to the
            // receiver.
            SendMsgMore(btrans);
        } else {
            // send_queue_ is empty. Flush sandesh->send_buf_ and this message.
            SendMsgAll(btrans);
        }
    } else {
        // Send the message
        SendInternal(btrans);
    }
    sandesh->Release();
}

// Package as many sandesh messages as possible [not more than 
// kEncodeBufferSize] before transporting it to the receiver. 
// 
// send_buf_ => unsent data (partial/complete message).
// buf => new message
// buf_len => buf's len
void SandeshWriter::SendMsgMore(boost::shared_ptr<TMemoryBuffer>
                                            send_buffer) {
    uint8_t *buf;
    uint32_t buf_len;

    send_buffer->getBuffer(&buf, &buf_len);

    if (send_buf_offset()) {
        // We have some unsent data.
        size_t bulk_msg_len = send_buf_offset() + buf_len;
        if (bulk_msg_len < kDefaultSendSize) {
            // We still have space for more data. Don't send the message yet.
            // Add the message to the existing data.
            append_send_buf(buf, buf_len);
        } else {
            uint8_t *buffer;
            // (send_buf_offset() + buf_len) >= kDefaultSendSize
            boost::shared_ptr<TMemoryBuffer> bulk_msg(
                    new TMemoryBuffer(send_buf_offset()));
            buffer = bulk_msg->getWritePtr(send_buf_offset());
            // Copy unsent data
            memcpy(buffer, send_buf(), send_buf_offset());
            bulk_msg->wroteBytes(send_buf_offset());
            // Send it
            SendInternal(bulk_msg);
            // Cleanup send_buf
            reset_send_buf();
            // Next send the new message
            SendInternal(send_buffer);
        }
    } else {
        // We don't have any unsent data.
        if (buf_len >= kDefaultSendSize) {
            // We don't have room to accommodate anything more.
            // Send the message now.
            SendInternal(send_buffer);
        } else {
            // We have room to accommodate more data.
            // Save this message.
            // Note: The memcpy here can be avoided by passing send_buf_ 
            // to TMemoryBuffer so that the message is encoded in 
            // send_buf_ itself. 
            set_send_buf(buf, buf_len);
        }
    }
}

// sandesh->send_queue_ is empty. 
// Flush unsent data (if any) and this message. 
void SandeshWriter::SendMsgAll(boost::shared_ptr<TMemoryBuffer> send_buffer) {
    uint8_t *buf;
    uint32_t buf_len;

    send_buffer->getBuffer(&buf, &buf_len);

    if (send_buf_offset()) {
        // We have some unsent data.
        size_t bulk_msg_len = send_buf_offset() + buf_len;
        if (bulk_msg_len <= kDefaultSendSize) {
            uint8_t *buffer;
            // We have enough room to send all the pending data in one message.
            boost::shared_ptr<TMemoryBuffer> bulk_msg(new 
                    TMemoryBuffer(bulk_msg_len));
            buffer = bulk_msg->getWritePtr(bulk_msg_len);
            memcpy(buffer, send_buf(), send_buf_offset());
            memcpy(buffer + send_buf_offset(), buf, buf_len);
            bulk_msg->wroteBytes(bulk_msg_len);
            // send the message
            SendInternal(bulk_msg);
            // reset send_buf_
            reset_send_buf();
        } else {
            uint8_t *buffer;
            // We don't have enough space to accommodate all the
            // pending data in one message. 
            boost::shared_ptr<TMemoryBuffer> old_buf(new
                    TMemoryBuffer(send_buf_offset()));
            buffer = old_buf->getWritePtr(send_buf_offset());
            memcpy(buffer, send_buf(),
                   send_buf_offset());
            old_buf->wroteBytes(send_buf_offset());
            // Take care of the unsent data in send_buf_ first.
            // Note that we could have accomodated part of buf [last message] 
            // here. But, not doing so to avoid additional memcpy() :)
            SendInternal(old_buf);
            // Cleanup send_buf_
            reset_send_buf();
            // Well, send the last message now.
            SendInternal(send_buffer);
        }
    } else {
        // No unsent data. Send the message now.
        SendInternal(send_buffer);
    }
}

void SandeshWriter::SendInternal(boost::shared_ptr<TMemoryBuffer> buf) {
    uint8_t  *buffer;
    uint32_t len;
    buf->getBuffer(&buffer, &len);
    tbb::mutex::scoped_lock lock(send_mutex_);
    ready_to_send_ = session_->Send((const uint8_t *)buffer, len, NULL);
}

//
// SandeshSession
//
SandeshSession::SandeshSession(SslServer *client, SslSocket *socket,
        int task_instance, int writer_task_id, int reader_task_id) :
    SslSession(client, socket),
    instance_(task_instance),
    writer_(new SandeshWriter(this)), 
    reader_(new SandeshReader(this)),
    send_queue_(new Sandesh::SandeshQueue(writer_task_id,
            task_instance,
            boost::bind(&SandeshSession::SendMsg, this, _1),
            kQueueSize)),
    keepalive_idle_time_(kSessionKeepaliveIdleTime),
    keepalive_interval_(kSessionKeepaliveInterval),
    keepalive_probes_(kSessionKeepaliveProbes),
    tcp_user_timeout_(kSessionTcpUserTimeout),
    reader_task_id_(reader_task_id) {
    if (Sandesh::role() == Sandesh::SandeshRole::Collector) {
        send_buffer_queue_.reset(new Sandesh::SandeshBufferQueue(writer_task_id,
                task_instance,
                boost::bind(&SandeshSession::SendBuffer, this, _1)));
        send_buffer_queue_->SetStartRunnerFunc(boost::bind(&SandeshSession::SessionSendReady, this));
    }
    send_queue_->SetStartRunnerFunc(boost::bind(&SandeshSession::SessionSendReady, this));
}

SandeshSession::~SandeshSession() {
}

bool SandeshSession::SessionSendReady() {
    return (IsEstablished() && writer_->SendReady() &&
            Sandesh::IsSendQueueEnabled());
}

void SandeshSession::SetSendQueueWaterMark(
    Sandesh::QueueWaterMarkInfo &swmi) {
    WaterMarkInfo wm(boost::get<0>(swmi),
        boost::bind(&Sandesh::SetSendingLevel, _1, boost::get<1>(swmi)));
    if (boost::get<2>(swmi)) {
        send_queue_->SetHighWaterMark(wm);
    } else {
        send_queue_->SetLowWaterMark(wm);
    }
}

void SandeshSession::ResetSendQueueWaterMark() {
    send_queue_->ResetHighWaterMark();
    send_queue_->ResetLowWaterMark();
}

void SandeshSession::Shutdown() {
    if (Sandesh::role() == Sandesh::SandeshRole::Collector) {
        send_buffer_queue_->Shutdown();
    }
    send_queue_->Shutdown();
}

std::string SandeshSession::ToString() const {
    std::stringstream out;
    out << TcpSession::ToString() << "(" << instance_ << ")";
    return out.str();
}

boost::system::error_code SandeshSession::SetSocketOptions() {
    boost::system::error_code ec = TcpSession::SetSocketOptions();
    if (ec) {
        return ec;
    }
    return SetSocketKeepaliveOptions(keepalive_idle_time_, keepalive_interval_,
            keepalive_probes_, tcp_user_timeout_);
}

void SandeshSession::OnRead(Buffer buffer) {
    reader_->OnRead(buffer);
}

bool SandeshSession::SendMsg(SandeshElement element) {
    Sandesh *sandesh = element.snh_;
    tbb::mutex::scoped_lock lock(send_mutex_);
    if (!IsEstablished()) {
        if (Sandesh::IsLoggingDroppedAllowed(sandesh->type())) {
            SANDESH_LOG(ERROR, __func__ << " Not Connected : Dropping Message: " <<
                sandesh->ToString());
        }
        increment_send_msg_fail();
        Sandesh::UpdateTxMsgFailStats(sandesh->Name(), 0,
            SandeshTxDropReason::SessionNotConnected);
        sandesh->Release();
        return true;
    }
    if (sandesh->IsLoggingAllowed()) {
        sandesh->Log();
    }
    bool more = !send_queue_->IsQueueEmpty();
    writer_->SendMsg(sandesh, more);
    return true;
}

bool SandeshSession::SendBuffer(boost::shared_ptr<TMemoryBuffer> sbuffer) {
    tbb::mutex::scoped_lock lock(send_mutex_);
    if (!IsEstablished()) {
        increment_send_buffer_fail();
        return true;
    }
    // No buffer packing supported currently
    writer_->SendBuffer(sbuffer);
    return true;
}

bool SandeshSession::EnqueueBuffer(u_int8_t *buf, u_int32_t buf_len) {
    boost::shared_ptr<TMemoryBuffer> sbuffer(new TMemoryBuffer(buf_len));
    u_int8_t *write_buf = sbuffer->getWritePtr(buf_len);
    memcpy(write_buf, buf, buf_len);
    sbuffer->wroteBytes(buf_len);
    return send_buffer_queue()->Enqueue(sbuffer);
}

Sandesh * SandeshSession::DecodeCtrlSandesh(const string& msg,
        const SandeshHeader& header,
        const string& sandesh_name, const uint32_t& header_offset) {
    namespace sandesh_prot = contrail::sandesh::protocol;
    namespace sandesh_trans = contrail::sandesh::transport;

    assert(header.get_Hints() & g_sandesh_constants.SANDESH_CONTROL_HINT);

    // Create and process the sandesh
    Sandesh *sandesh = SandeshBaseFactory::CreateInstance(sandesh_name);
    if (sandesh == NULL) {
        SANDESH_LOG(ERROR, __func__ << ": Unknown sandesh ctrl message: " << sandesh_name);
        return NULL;
    }
    boost::shared_ptr<sandesh_trans::TMemoryBuffer> btrans =
            boost::shared_ptr<sandesh_trans::TMemoryBuffer>(
                    new sandesh_trans::TMemoryBuffer((uint8_t *)msg.c_str() + header_offset,
                            msg.size() - header_offset));
    boost::shared_ptr<sandesh_prot::TXMLProtocol> prot =
            boost::shared_ptr<sandesh_prot::TXMLProtocol>(new sandesh_prot::TXMLProtocol(btrans));
    int32_t xfer = sandesh->Read(prot);
    if (xfer < 0) {
        SANDESH_LOG(ERROR, __func__ << ": Decoding " << sandesh_name << " for ctrl FAILED");
        sandesh->Release();
        return NULL;
    } else {
        return sandesh;
    }
}

void SandeshSession::EnqueueClose() {
    if (IsClosed()) {
        return;
    }
    tbb::mutex::scoped_lock lock(conn_mutex_);
    if (connection_) {
        connection_->state_machine()->OnSessionEvent(this,
            TcpSession::CLOSE);
    }
}

//
// SandeshReader
//
SandeshReader::SandeshReader(SandeshSession *session) :
        buf_(""),
        offset_(0),
        msg_length_(-1),
        session_(session) {
    buf_.reserve(kDefaultRecvSize);
}

SandeshReader::~SandeshReader() {
}

int SandeshReader::ExtractMsgHeader(const std::string& msg,
        SandeshHeader& header, std::string& msg_type, uint32_t& header_offset) {
    int32_t xfer = 0, ret;
    boost::shared_ptr<TMemoryBuffer> btrans =
            boost::shared_ptr<TMemoryBuffer>(
                    new TMemoryBuffer((uint8_t *)msg.c_str(), msg.size()));
    boost::shared_ptr<TXMLProtocol> prot =
            boost::shared_ptr<TXMLProtocol>(new TXMLProtocol(btrans));
    // Read the sandesh header and note the offset
    if ((ret = header.read(prot)) <= 0) {
        SANDESH_LOG(ERROR, __func__ << ": Sandesh header read FAILED: " << msg);
        return EINVAL;
    }
    xfer += ret;
    header_offset = xfer;
    // Extract the message name
    if ((ret = prot->readSandeshBegin(msg_type)) <= 0) {
        SANDESH_LOG(ERROR, __func__ << ": Sandesh begin read FAILED: " << msg);
        return EINVAL;
    }
    xfer += ret;
    return 0;
}

void SandeshReader::SetBuf(const std::string &str) {
    if (buf_.empty()) {
        ReplaceBuf(str);
    } else {
        buf_ += str;
    }
    // TODO handle buf_ > kMaxMessageSize
}

void SandeshReader::ReplaceBuf(const std::string &str) {
    buf_ = str;
    buf_.reserve(SandeshReader::kDefaultRecvSize);
    offset_ = 0;
}

bool SandeshReader::LeftOver() const {
    if (buf_.empty()) {
        return false;
    }
    return (buf_.size() != offset_);
}

// Returns false if not able to extract the message length, true otherwise
bool SandeshReader::ExtractMsgLength(size_t &msg_length, int *result) {
    // Have we read enough to extract the message length?
    if (buf_.size() - offset_ < SandeshWriter::sandesh_open_.size()) {
        return false;
    }
    // Some sanity check
    if (!boost::algorithm::starts_with(buf_.c_str() + offset_,
            SandeshWriter::sandesh_open_attr_length_)) {
        *result = -1;
        return false;
    }

    std::string::const_iterator end = buf_.begin() + offset_ +
            SandeshWriter::sandesh_open_.size() - 1;
    if (*end != '>') {
        *result = -2;
        return false;
    }

    std::string::const_iterator st = buf_.begin() + offset_ +
            SandeshWriter::sandesh_open_attr_length_.size();
    // Adjust for double quote
    --end;
    string length = string(st, end);

    stringToInteger(length.c_str(), msg_length);
    if (msg_length == 0) {
	*result = -3;
	return false;
    }
    return true;
}

// Returns false if not able to extract the full message, true otherwise
bool SandeshReader::ExtractMsg(Buffer buffer, int *result, bool NewBuf) {
    if (NewBuf) {
        const uint8_t *cp = TcpSession::BufferData(buffer);
        // TODO Avoid this copy
        std::string str(cp, cp + TcpSession::BufferSize(buffer));
        SetBuf(str);
    }
    // Extract the message length
    if (!MsgLengthKnown()) {
        size_t msg_length = 0;
        bool done = ExtractMsgLength(msg_length, result);
        if (done == false) {
            return false;
        }
        set_msg_length(msg_length);
    }
    // Check if the entire message is read or not
    if (buf_.size() < msg_length()) {
        return false;
    }
    return true;
}

void SandeshReader::OnRead(Buffer buffer) {
    tbb::mutex::scoped_lock lock(cb_mutex_);
    // Check if session is being deleted, then drop the packet
    if (cb_.empty()) {
        SANDESH_LOG(ERROR, __func__ <<
            " Session being deleted: Dropping Message");
        session_->increment_recv_fail();
        session_->ReleaseBuffer(buffer);
        return;
    }
    int result = 0;
    bool done = ExtractMsg(buffer, &result, true);
    do {
        if (result < 0) {
            // Generate error and close connection
            SANDESH_LOG(ERROR, __func__ << " Message extract failed: " << result);
            const uint8_t *cp = TcpSession::BufferData(buffer);
            size_t cp_size = TcpSession::BufferSize(buffer);
            SANDESH_LOG(ERROR, __func__ << " OnRead Buffer Size: " << cp_size);
            SANDESH_LOG(ERROR, __func__ << " OnRead Buffer: ");
            std::string debug((const char*)cp, cp_size);
            SANDESH_LOG(ERROR, debug);
            SANDESH_LOG(ERROR, __func__ << " Reader Size: " << buf_.size());
            SANDESH_LOG(ERROR, __func__ << " Reader Offset: " << offset_);
            SANDESH_LOG(ERROR, __func__ << " Reader Buffer: " << buf_);
            buf_.clear();
            offset_ = 0;
            // Enqueue a close on the state machine
            session_->increment_recv_fail();
            session_->EnqueueClose();
            break;
        }
        if (done == true) {
            // We got good match. Process the message after extracting out
            // the sandesh open and close envelope
            std::string::const_iterator st = buf_.begin() + offset_ +
                    SandeshWriter::sandesh_open_.size();
            std::string::const_iterator end = buf_.begin() + offset_ +
                    msg_length() - SandeshWriter::sandesh_close_.size();
            std::string xml(st, end);
            offset_ += msg_length();
            reset_msg_length();
            if (!cb_(xml, session_)) {
                // Enqueue a close on the state machine
                session_->increment_recv_fail();
                session_->EnqueueClose();
                break;
            }
        } else {
            // Read more data.
            break;
        }

        if (LeftOver()) {
            ReplaceBuf(string(buf_, offset_, buf_.size() - offset_));
            done = ExtractMsg(buffer, &result, false);
        } else {
            // No more data in the Buffer
            buf_.clear();
            offset_ = 0;
            break;
        }
    } while (true);

    session_->ReleaseBuffer(buffer);
    return;
}

void SandeshReader::SetReceiveMsgCb(SandeshReceiveMsgCb cb) {
    tbb::mutex::scoped_lock lock(cb_mutex_);
    cb_ = cb;
}
