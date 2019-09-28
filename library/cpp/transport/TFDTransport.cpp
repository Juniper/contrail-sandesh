/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdint.h>
#include <cerrno>

#include <transport/TFDTransport.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <base/logging.h>

using namespace std;

namespace contrail { namespace sandesh { namespace transport {

void TFDTransport::close() {
  if (!isOpen()) {
    return;
  }

  int rv = ::close(fd_);
  int errno_copy = errno;
  fd_ = -1;
  if (rv < 0) {
    LOG(ERROR, __func__ << ": TFDTransport::close() " << errno_copy);
  }
}

int32_t TFDTransport::read(uint8_t* buf, uint32_t len) {
  unsigned int maxRetries = 5; // same as the TSocket default
  unsigned int retries = 0;
  while (true) {
    ssize_t rv = ::read(fd_, buf, len);
    if (rv < 0) {
      if (errno == EINTR && retries < maxRetries) {
        // If interrupted, try again
        ++retries;
        continue;
      }
      int errno_copy = errno;
      LOG(ERROR, __func__ << ": TFDTransport::read() " << errno_copy);
    }
    return rv;
  }
}

int32_t TFDTransport::write(const uint8_t* buf, uint32_t len) {
  while (len > 0) {
    ssize_t rv = ::write(fd_, buf, len);

    if (rv < 0) {
      int errno_copy = errno;
      LOG(ERROR, __func__ << ": TFDTransport::write() " << errno_copy);
      return rv;
    } else if (rv == 0) {
      LOG(ERROR, __func__ << ": TFDTransport::write() EOF");
      return -1;
    }

    buf += rv;
    len -= rv;
  }
  return 0;
}

}}} // contrail::sandesh::transport
