/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

/*
 * thrift_fake_transport.h
 *
 * Thrift FAKE transport implementation
 */

#include "sandesh.h"

/* implements thrift_transport_is_open */
u_int8_t
thrift_fake_transport_is_open (ThriftTransport *transport)
{
  THRIFT_UNUSED_VAR (transport);
  return 1;
}

/* implements thrift_transport_open */
u_int8_t
thrift_fake_transport_open (ThriftTransport *transport ,
                            int *error)
{
  THRIFT_UNUSED_VAR (transport);
  THRIFT_UNUSED_VAR (error);
  return 1;
}

/* implements thrift_transport_close */
u_int8_t
thrift_fake_transport_close (ThriftTransport *transport ,
                             int *error)
{
  THRIFT_UNUSED_VAR (transport);
  THRIFT_UNUSED_VAR (error);
  return 1;
}

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

/* implements thrift_transport_read_end
 * called when read is complete.  nothing to do on our end. */
u_int8_t
thrift_fake_transport_read_end (ThriftTransport *transport,
                                int *error)
{
  THRIFT_UNUSED_VAR (transport);
  THRIFT_UNUSED_VAR (error);
  return 1;
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

/* implements thrift_transport_write_end
 * called when write is complete.  nothing to do on our end. */
u_int8_t
thrift_fake_transport_write_end (ThriftTransport *transport ,
                                 int *error)
{
  THRIFT_UNUSED_VAR (transport);
  THRIFT_UNUSED_VAR (error);
  return 1;
}

/* implements thrift_transport_flush */
u_int8_t
thrift_fake_transport_flush (ThriftTransport *transport ,
                             int *error)
{
  THRIFT_UNUSED_VAR (transport);
  THRIFT_UNUSED_VAR (error);
  return 1;
}

/* initializes the instance */
void
thrift_fake_transport_init (ThriftFakeTransport *transport)
{
  transport->tt.ttype = T_TRANSPORT_FAKE;
  transport->tt.is_open = thrift_fake_transport_is_open;
  transport->tt.open = thrift_fake_transport_open;
  transport->tt.close = thrift_fake_transport_close;
  transport->tt.read = thrift_fake_transport_read;
  transport->tt.read_end = thrift_fake_transport_read_end;
  transport->tt.write = thrift_fake_transport_write;
  transport->tt.write_end = thrift_fake_transport_write_end;
  transport->tt.flush = thrift_fake_transport_flush;
}
