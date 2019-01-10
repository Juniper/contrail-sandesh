/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

/*
 * thrift_fake_transport.h
 *
 * Thrift FAKE transport implementation
 */

#include "sandesh.h"

/* implements thrift_transport_read */
int32_t
thrift_fake_transport_read (ThriftTransport *transport, void *buf,
                            u_int32_t len, int *error)
{
  /* NOT IMPLEMENTED */
  THRIFT_UNUSED_VAR (transport);
  THRIFT_UNUSED_VAR (buf);
  THRIFT_UNUSED_VAR (len);
  THRIFT_UNUSED_VAR (error);
  return -1;
}

/* implements thrift_transport_write */
u_int8_t
thrift_fake_transport_write (ThriftTransport *transport,
                             const void *buf,
                             const u_int32_t len, int *error)
{
  THRIFT_UNUSED_VAR (transport);
  THRIFT_UNUSED_VAR (buf);
  THRIFT_UNUSED_VAR (len);
  THRIFT_UNUSED_VAR (error);
  /* Always success */
  return 1;
}

/* initializes the instance */
void
thrift_fake_transport_init (ThriftFakeTransport *transport)
{
  transport->tt.ttype = T_TRANSPORT_FAKE;
  transport->tt.read = thrift_fake_transport_read;
  transport->tt.write = thrift_fake_transport_write;
}
