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

#include "sandesh.h"

static inline u_int32_t
available_read (ThriftMemoryBuffer *t)
{
  return t->buf_woffset - t->buf_roffset;
}

static inline u_int32_t
available_write (ThriftMemoryBuffer *t)
{
  return t->buf_size - t->buf_woffset;
}

static int
ensure_can_write (ThriftMemoryBuffer *t, u_int32_t len)
{
  u_int32_t avail, new_size;
  void *new_buf;
  int error;

  avail = available_write(t);
  if (!t->owner)
  {
#ifdef __KERNEL__
    if (vrouter_dbg)
    {
      os_log(OS_LOG_DEBUG, "Insufficient space in external memory buffer of size %d "
             "bytes for writing %d bytes", t->buf_size, len);
    }
#elif (!defined(SANDESH_QUIET))
    os_log(OS_LOG_ERR, "Insufficient space in external memory buffer of size %d "
           "bytes for writing %d bytes", t->buf_size, len);
#endif
    return ENOMEM;
  }

  /* Grow the buffer as necessary. */
  new_size = t->buf_size;
  while (len > avail)
  {
    new_size = new_size > 0 ? new_size * 2 : 1;
    avail = available_write(t) + (new_size - t->buf_size);
  }

  /* Allocate into a new pointer so we don't bork ours if it fails. */
  new_buf = os_realloc(t->buf, new_size);
  if (new_buf == NULL) {
    error = ENOMEM;
    os_log(OS_LOG_ERR, "Reallocation of memory buffer of size %d bytes failed "
           "[%d]", new_size, error);
    return error;
  }

  t->buf_size = new_size;
  t->buf = new_buf;
  return 0;
}

int
thrift_memory_buffer_wrote_bytes (ThriftMemoryBuffer *t, u_int32_t len)
{
  u_int32_t avail = available_write(t);
  if (len > avail)
  {
    os_log(OS_LOG_ERR, "Client wrote more bytes (%d) than size of buffer (%d).",
           len, t->buf_size);
    return ENOMEM;
  }
  t->buf_woffset += len;
  return 0;
}

/* implements thrift_transport_is_open */
u_int8_t
thrift_memory_buffer_is_open (ThriftTransport *transport)
{
  THRIFT_UNUSED_VAR (transport);
  return 1;
}

/* implements thrift_transport_open */
u_int8_t
thrift_memory_buffer_open (ThriftTransport *transport ,
                           int *error)
{
  THRIFT_UNUSED_VAR (transport);
  THRIFT_UNUSED_VAR (error);
  return 1;
}

/* implements thrift_transport_close */
u_int8_t
thrift_memory_buffer_close (ThriftTransport *transport ,
                            int *error)
{
  THRIFT_UNUSED_VAR (transport);
  THRIFT_UNUSED_VAR (error);
  return 1;
}

/* implements thrift_transport_read */
int32_t
thrift_memory_buffer_read (ThriftTransport *transport, void *buf,
                           u_int32_t len, int *error)
{
  ThriftMemoryBuffer *t = (ThriftMemoryBuffer *) (transport);

  if (available_read(t) < len)
  {
    *error = THRIFT_TRANSPORT_ERROR_RECEIVE;
    os_log(OS_LOG_ERR, "Unable to read %d bytes from buffer of length %d"
           " at read offset %d, write offset %d", len, t->buf_size,
           t->buf_roffset, t->buf_woffset);
    return -1;
  }

  memcpy (buf, t->buf + t->buf_roffset, len);
  t->buf_roffset += len;

  if (t->buf_roffset == t->buf_woffset)
  {
    t->buf_roffset = t->buf_woffset = 0;
  }

  return len;
}

/* implements thrift_transport_read_end
 * called when read is complete.  nothing to do on our end. */
u_int8_t
thrift_memory_buffer_read_end (ThriftTransport *transport ,
                               int *error)
{
  THRIFT_UNUSED_VAR (transport);
  THRIFT_UNUSED_VAR (error);
  return 1;
}

/* implements thrift_transport_write */
u_int8_t
thrift_memory_buffer_write (ThriftTransport *transport,
                            const void *buf,
                            const u_int32_t len, int *error)
{
  ThriftMemoryBuffer *t = (ThriftMemoryBuffer *) (transport);

  if (len > available_write(t))
  {
    /* does the buffer have enough space. ? */
    if (ensure_can_write(t, len))
    {
      *error = THRIFT_TRANSPORT_ERROR_SEND;
#ifdef __KERNEL__
      if (vrouter_dbg)
      {
        os_log(OS_LOG_DEBUG, "Unable to write %d bytes to buffer of length %d"
               " at write offset %d", len, t->buf_size, t->buf_woffset);
      }
#elif (!defined(SANDESH_QUIET))
        os_log(OS_LOG_ERR, "Unable to write %d bytes to buffer of length %d"
               " at write offset %d", len, t->buf_size, t->buf_woffset);
#endif
        return 0;
      }
  }

  memcpy(t->buf + t->buf_woffset, buf, len);
  t->buf_woffset += len;
  return 1;
}

/* implements thrift_transport_write_end
 * called when write is complete.  nothing to do on our end. */
u_int8_t
thrift_memory_buffer_write_end (ThriftTransport *transport ,
                                int *error)
{
  THRIFT_UNUSED_VAR (transport);
  THRIFT_UNUSED_VAR (error);
  return 1;
}

/* implements thrift_transport_flush */
u_int8_t
thrift_memory_buffer_flush (ThriftTransport *transport ,
                            int *error)
{
  THRIFT_UNUSED_VAR (transport);
  THRIFT_UNUSED_VAR (error);
  return 1;
}

static void
thrift_memory_buffer_transport_init (ThriftTransport *transport)
{
  transport->ttype = T_TRANSPORT_MEMORY_BUFFER;
  transport->is_open = thrift_memory_buffer_is_open;
  transport->open = thrift_memory_buffer_open;
  transport->close = thrift_memory_buffer_close;
  transport->read = thrift_memory_buffer_read;
  transport->read_end = thrift_memory_buffer_read_end;
  transport->write = thrift_memory_buffer_write;
  transport->write_end = thrift_memory_buffer_write_end;
  transport->flush = thrift_memory_buffer_flush;
}

/* initializes the instance */
void
thrift_memory_buffer_init (ThriftMemoryBuffer *transport, void *buf, u_int32_t size)
{
  thrift_memory_buffer_transport_init(&transport->tt);
  if (buf == NULL) {
    transport->buf = os_malloc(size);
    transport->owner = 1;
  } else {
    transport->buf = buf;
    transport->owner = 0;
  }
  transport->buf_size = size;
  transport->buf_roffset = 0;
  transport->buf_woffset = 0;
}

/* destructor */
void
thrift_memory_buffer_finalize (ThriftMemoryBuffer *transport)
{
  if (transport->buf != NULL && transport->owner)
  {
    os_free (transport->buf);
  }
  transport->buf = NULL;
  transport->buf_size = 0;
  transport->buf_roffset = 0;
  transport->buf_woffset = 0;
}
