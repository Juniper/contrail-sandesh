/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

/*
 * thrift_fake_transport.h
 *
 * Thrift FAKE transport
 */

#ifndef _THRIFT_FAKE_TRANSPORT_H
#define _THRIFT_FAKE_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/*! \file thrift_fake_transport.h
 *  \brief Implementation of a Thrift FAKE transport.
 */

/*!
 * ThriftFakeTransport instance.
 */
struct _ThriftFakeTransport
{
  /* Needs to be the first element */
  ThriftTransport tt;
};
typedef struct _ThriftFakeTransport ThriftFakeTransport;

void thrift_fake_transport_init (ThriftFakeTransport *transport);

#ifdef __cplusplus
}
#endif

#endif
