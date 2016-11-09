/*
 * Copyright (c) 2013 Juniper Networks, Inc. All rights reserved.
 */

/*
 * sandesh.c
 *
 * Sandesh C Library
 */

#include "sandesh.h"

/**
 * Extract sandesh information corresponding to sandesh name
 *
 * Returns sandesh info on success, NULL otherwise
 */
sandesh_info_t *
sandesh_find_info (sandesh_info_t *infos, const char *name) {
    int sidx = 0;
    while (infos[sidx].name != NULL) {
        if (strcmp(infos[sidx].name, name) == 0) {
            return &infos[sidx];
        }
        sidx++;
    }
    return NULL;
}

/**
 * Decode a sandesh from the buffer and call the sandesh handler function
 *
 * Returns the number of bytes processed, -1 on error and sets error
 */
static int32_t
sandesh_decode_one (u_int8_t *buf, u_int32_t buf_len, sandesh_find_info_fn sinfo_find_fn,
                    int *error) {
    int32_t rxfer = 0, ret, sread_len = 0;
    char *sname = NULL;
    sandesh_info_t *sinfo;
    void *sandesh;
    union {
        void * b[4];
        int32_t all;
    } bytes4;

    /* Read the sandesh name */
    if (buf_len < rxfer + 4) {
        os_log(OS_LOG_ERR, "Read sandesh begin FAILED");
        ret = -1;
        goto exit;
    }
    memcpy(bytes4.b, buf + rxfer, 4);
    rxfer += 4;
    sread_len = ntohl(bytes4.all);
    if (sread_len > 0) {
        if (buf_len < rxfer + sread_len) {
            os_log(OS_LOG_ERR, "Read sandesh begin: Length(%d) FAILED",
                sread_len);
            ret = -1;
            goto exit;
        }
        sname = os_malloc(sread_len + 1);
        memcpy(sname, buf + rxfer, sread_len);
        sname[sread_len] = 0;
        rxfer += sread_len;
    } else {
        os_log(OS_LOG_ERR, "Read sandesh begin: Length(%d) INVALID",
            sread_len);
        ret = -1;
        goto exit;
    }

    /* Find the corresponding sandesh information */
    sinfo = sinfo_find_fn(sname);
    if (sinfo == NULL) {
        os_log(OS_LOG_ERR, "No information for sandesh %s", sname);
        *error = EINVAL;
        ret = -1;
        goto exit;
    }

    sandesh = os_zalloc(sinfo->size);
    if (sandesh == NULL) {
        os_log(OS_LOG_ERR, "Allocation of sandesh %s : %d bytes FAILED",
               sname, sinfo->size);
        *error = ENOMEM;
        ret = -1;
        goto exit;
    }

    /* Now, read the sandesh */
    rxfer = sinfo->read_binary_from_buffer(sandesh, buf, buf_len, error);
    if (rxfer < 0) {
        os_log(OS_LOG_ERR, "%s read FAILED (%d)", sname, *error);
        sinfo->free(sandesh);
        os_free(sandesh);
        sandesh = NULL;
        ret = -1;
        goto exit;
    }
    ret = rxfer;

    /* Process the sandesh */
    sinfo->process(sandesh);

    /* Release the sandesh */
    sinfo->free(sandesh);
    os_free(sandesh);
    sandesh = NULL;

exit:
    if (sname) {
        os_free(sname);
        sname = NULL;
    }
    return ret;
}

/**
 * Decode sandeshs from the buffer and calls the sandesh handler function
 * for each decoded sandesh
 *
 * Returns the number of bytes processed, -1 on error and sets error
 */
int32_t
sandesh_decode (u_int8_t *buf, u_int32_t buf_len, sandesh_find_info_fn sinfo_find_fn,
                int *error) {
    u_int32_t xfer = 0;
    int32_t ret;

    while (xfer < buf_len) {
        ret = sandesh_decode_one(buf + xfer, buf_len - xfer, sinfo_find_fn, error);
        if (ret < 0) {
            os_log(OS_LOG_ERR, "Sandesh read from %d bytes, at offset %d "
                    "FAILED (%d)", buf_len, xfer, ret);
            return ret;
        }
        xfer += ret;
    }
    return xfer;
}

/*
 * Encode a sandesh into the buffer
 *
 * Returns the length of the encoded sandesh, -1 on error and set error
 */
int32_t
sandesh_encode (void *sandesh, const char *sname,
                sandesh_find_info_fn sinfo_find_fn, u_int8_t *buf,
                u_int32_t buf_len, int *error) {
    sandesh_info_t *sinfo;
    int32_t wxfer;

    /* First find the corresponding sandesh information */
    sinfo = sinfo_find_fn(sname);
    if (sinfo == NULL) {
        os_log(OS_LOG_ERR, "Write: No information for sandesh %s", sname);
        *error = EINVAL;
        return -1;
    }

    /* Now, write the sandesh */
    wxfer = sinfo->write_binary_to_buffer(sandesh, buf, buf_len, error);
    if (wxfer < 0) {
#ifdef __KERNEL__
        if (vrouter_dbg) {
            os_log(OS_LOG_DEBUG, "Write: Encode sandesh %s FAILED(%d)",
                   sname, *error);
        }
#elif (!defined(SANDESH_QUIET))
        os_log(OS_LOG_ERR, "Write: Encode sandesh %s FAILED(%d)", sname, *error);
#endif
        return -1;
    }

    return wxfer;
}

/*
 * Get the encoded length of a sandesh
 *
 * Returns the encoded length of a sandesh, -1 on error and set error
 */
int32_t
sandesh_get_encoded_length (void *sandesh, const char *sname,
                            sandesh_find_info_fn sinfo_find_fn, int *error) {
    ThriftBinaryProtocol protocol;
    ThriftFakeTransport transport;
    sandesh_info_t *sinfo;
    int32_t wxfer;

    /* First find the corresponding sandesh information */
    sinfo = sinfo_find_fn(sname);
    if (sinfo == NULL) {
        os_log(OS_LOG_ERR, "Encode: No information for sandesh %s", sname);
        *error = EINVAL;
        return -1;
    }

    /* Initialize the transport and protocol */
    thrift_fake_transport_init(&transport);
    thrift_protocol_init(&protocol, T_PROTOCOL_BINARY, (ThriftTransport *)&transport);

    /* Now, write the sandesh into the fake transport to get the encoded length */
    wxfer = sinfo->write(sandesh, &protocol, error);
    if (wxfer < 0) {
        os_log(OS_LOG_ERR, "Encode: Encode sandesh %s FAILED(%d)", sname, *error);
        return -1;
    }

    return wxfer;
}
