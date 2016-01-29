/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

//
// sandesh_session.h
//
// Sandesh Session
//

#ifndef __SANDESH_SESSION_H__
#define __SANDESH_SESSION_H__

#include <tbb/mutex.h>

#include <boost/system/error_code.hpp>
#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>

#include <base/util.h>
#include <io/tcp_session.h>

#include <sandesh/transport/TBufferTransports.h>
#include <sandesh/sandesh.h>
#include <sandesh/sandesh_uve_types.h>

using contrail::sandesh::transport::TMemoryBuffer;
class SandeshSession;
class Sandesh;

class SandeshWriter {
public:
    static const uint32_t kEncodeBufferSize = 2048;
    static const unsigned int kDefaultSendSize = 16384;

    SandeshWriter(SandeshSession *session);
    ~SandeshWriter();
    void SendMsg(Sandesh *sandesh, bool more);
    void SendBuffer(boost::shared_ptr<TMemoryBuffer> sbuffer,
            bool more = false) {
        SendInternal(sbuffer);
    }
    void WriteReady(const boost::system::error_code &ec);
    bool SendReady() {
        tbb::mutex::scoped_lock lock(send_mutex_);
        return ready_to_send_;
    }

    static const std::string sandesh_open_;
    static const std::string sandesh_open_attr_length_;
    static const std::string sandesh_close_;

protected:
    friend class SandeshSessionTest;

    // Inline routines invoked by SendMsg()
    void SendMsgMore(boost::shared_ptr<TMemoryBuffer>);
    void SendMsgAll(boost::shared_ptr<TMemoryBuffer>);

private:
    friend class SandeshSendMsgUnitTest;

    SandeshSession *session_;

    void SendInternal(boost::shared_ptr<TMemoryBuffer>);
    void ConnectTimerExpired(const boost::system::error_code &error);
    size_t send_buf_offset() { return send_buf_offset_; }
    uint8_t* send_buf() const { return send_buf_; }
    void set_send_buf(uint8_t *buf, size_t len) {
        assert(len && (len < kDefaultSendSize));
        memcpy(send_buf(), buf, len);
        send_buf_offset_ = len;
    }
    void append_send_buf(uint8_t *buf, size_t len) {
        assert(len && ((len + send_buf_offset_) < kDefaultSendSize));
        memcpy(send_buf() + send_buf_offset_, buf, len);
        send_buf_offset_ += len;
    }
    void reset_send_buf() {
        send_buf_offset_ = 0;
    }

    tbb::mutex send_mutex_;
    bool ready_to_send_;
    // send_buf_ is used to store unsent data
    uint8_t *send_buf_;
    size_t send_buf_offset_;

#define sXML_SANDESH_OPEN_ATTR_LENGTH  "<sandesh length=\""
#define sXML_SANDESH_OPEN              "<sandesh length=\"0000000000\">"
#define sXML_SANDESH_CLOSE             "</sandesh>"

    DISALLOW_COPY_AND_ASSIGN(SandeshWriter);
};

typedef boost::function<bool(const std::string&, SandeshSession *)>
    SandeshReceiveMsgCb;

class SandeshReader {
public:
    typedef boost::asio::const_buffer Buffer;

    SandeshReader(SandeshSession *session);
    virtual ~SandeshReader();
    virtual void OnRead(Buffer buffer);
    void SetReceiveMsgCb(SandeshReceiveMsgCb cb);
    static int ExtractMsgHeader(const std::string& msg, SandeshHeader& header,
            std::string& msg_type, uint32_t& header_offset);

private:
    bool MsgLengthKnown() { return msg_length_ != (size_t)-1; }

    size_t msg_length() { return msg_length_; }

    void set_msg_length(size_t length) { msg_length_ = length; }

    void reset_msg_length() { set_msg_length(-1); }

    void SetBuf(const std::string &str);
    void ReplaceBuf(const std::string &str);
    bool LeftOver() const;
    int MatchString(const std::string& match, size_t &m_offset);
    bool ExtractMsgLength(size_t &msg_length, int *result);
    bool ExtractMsg(Buffer buffer, int *result, bool NewBuf);

    std::string buf_;
    size_t offset_;
    size_t msg_length_;
    SandeshSession *session_;
    tbb::mutex cb_mutex_;
    SandeshReceiveMsgCb cb_;

    static const int kDefaultRecvSize = SandeshWriter::kDefaultSendSize;

    DISALLOW_COPY_AND_ASSIGN(SandeshReader);
};

class SandeshConnection;

class SandeshSession : public TcpSession {
public:
    SandeshSession(TcpServer *client, Socket *socket, int task_instance, 
        int writer_task_id, int reader_task_id);
    virtual ~SandeshSession();
    virtual void Shutdown();
    virtual void OnRead(Buffer buffer);
    virtual void WriteReady(const boost::system::error_code &ec) {
        writer_->WriteReady(ec);
    }
    virtual bool EnqueueBuffer(u_int8_t *buf, u_int32_t buf_len);
    Sandesh::SandeshQueue *send_queue() {
        return send_queue_.get();
    }
    Sandesh::SandeshBufferQueue *send_buffer_queue() {
        return send_buffer_queue_.get();
    }
    SandeshWriter* writer() {
        return writer_.get();
    }
    void SetConnection(SandeshConnection *connection) {
        tbb::mutex::scoped_lock lock(conn_mutex_);
        connection_ = connection;
    }
    SandeshConnection *connection() {
        tbb::mutex::scoped_lock lock(conn_mutex_);
        return connection_;
    }
    void SetReceiveMsgCb(SandeshReceiveMsgCb cb) {
        reader_->SetReceiveMsgCb(cb);
    }
    virtual int GetSessionInstance() const {
        return instance_;
    }
    virtual void EnqueueClose();
    virtual boost::system::error_code SetSocketOptions();
    virtual std::string ToString() const;
    static Sandesh * DecodeCtrlSandesh(const std::string& msg, const SandeshHeader& header,
        const std::string& sandesh_name, const uint32_t& header_offset);
    // Session statistics
    inline void increment_recv_msg() {
        sstats_.num_recv_msg++;
    }
    inline void increment_recv_msg_fail() {
        sstats_.num_recv_msg_fail++;
    }
    inline void increment_recv_fail() {
        sstats_.num_recv_fail++;
    }
    inline void increment_send_msg() {
        sstats_.num_send_msg++;
    }
    inline void increment_send_msg_fail() {
        sstats_.num_send_msg_fail++;
    }
    inline void increment_send_buffer_fail() {
        sstats_.num_send_buffer_fail++;
    }
    inline void increment_wait_msgq_enqueue() {
        sstats_.num_wait_msgq_enqueue++;
    }
    inline void increment_wait_msgq_dequeue() {
        sstats_.num_wait_msgq_dequeue++;
    }
    inline void increment_write_ready_cb_error() {
        sstats_.num_write_ready_cb_error++;
    }
    const SandeshSessionStats& GetStats() const {
        return sstats_;
    }
    void SetSendQueueWaterMark(Sandesh::QueueWaterMarkInfo &wm_info);
    void ResetSendQueueWaterMark();

protected:
    virtual int reader_task_id() const {
        return reader_task_id_;
    }

private:
    friend class SandeshSessionTest;

    // 60 seconds - 45s + (3*5)s
    static const int kSessionKeepaliveIdleTime = 15; // in seconds
    static const int kSessionKeepaliveInterval = 3; // in seconds
    static const int kSessionKeepaliveProbes = 5; // count
    static const int kSessionTcpUserTimeout = 30000; // ms
    static const int kQueueSize = 200 * 1024 * 1024; // 200 MB

    bool SendMsg(SandeshElement element);
    bool SendBuffer(boost::shared_ptr<TMemoryBuffer> sbuffer);
    bool SessionSendReady();

    int instance_;
    boost::scoped_ptr<SandeshWriter> writer_;
    boost::scoped_ptr<SandeshReader> reader_;
    boost::scoped_ptr<Sandesh::SandeshQueue> send_queue_;
    boost::scoped_ptr<Sandesh::SandeshBufferQueue> send_buffer_queue_;
    SandeshConnection *connection_;
    tbb::mutex conn_mutex_;
    tbb::mutex send_mutex_;
    int keepalive_idle_time_;
    int keepalive_interval_;
    int keepalive_probes_;
    int tcp_user_timeout_;
    int reader_task_id_;

    // Session statistics
    SandeshSessionStats sstats_;
 
    DISALLOW_COPY_AND_ASSIGN(SandeshSession);
};

#endif // __SANDESH_SESSION_H__
