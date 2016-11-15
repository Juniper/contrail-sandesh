//
// Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
//

#ifndef SANDESH_LIBRARY_CPP_TEST_SANDESH_TEST_COMMON_H_
#define SANDESH_LIBRARY_CPP_TEST_SANDESH_TEST_COMMON_H_

#include <sandesh/sandesh_server.h>

class SandeshSession;
class SandeshMessage;

namespace contrail {
namespace sandesh {
namespace test {

class SandeshServerTest : public SandeshServer {
public:
    typedef boost::function<bool(SandeshSession *session,
        const SandeshMessage *msg)> ReceiveMsgCb;

    SandeshServerTest(EventManager *evm, ReceiveMsgCb cb) :
        SandeshServer(evm),
        cb_(cb) {
    }

    virtual ~SandeshServerTest() {
    }

    virtual bool ReceiveSandeshMsg(SandeshSession *session,
        const SandeshMessage *msg, bool rsc) {
        return cb_(session, msg);
    }

private:
    ReceiveMsgCb cb_;
};

void CreateFakeMessage(uint8_t *data, size_t length);
void CreateFakeCtrlMessage(std::string& data);

} // end namespace test
} // end namespace sandesh
} // end namespace contrail

#endif // SANDESH_LIBRARY_CPP_TEST_SANDESH_TEST_COMMON_H_
