/*
 * Copyright (c) 2018 Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TCTI_MSSIM_H
#define TCTI_MSSIM_H

#include <limits.h>

#include "tcti-common.h"
#include "util/io.h"

/*
 * longest possible conf string:
 * HOST_NAME_MAX + max char uint16 (5) + strlen ("host=,port=") (11)
 */
#define TCTI_MSSIM_CONF_MAX (HOST_NAME_MAX + 16)
#define TCTI_MSSIM_DEFAULT_HOST "localhost"
#define TCTI_MSSIM_DEFAULT_PORT 2321
#define MSSIM_CONF_DEFAULT_INIT { \
    .host = TCTI_MSSIM_DEFAULT_HOST, \
    .port = TCTI_MSSIM_DEFAULT_PORT, \
}

#define TCTI_MSSIM_MAGIC 0xf05b04cd9f02728dULL

typedef struct {
    char *host;
    uint16_t port;
} mssim_conf_t;

typedef struct {
    TSS2_TCTI_COMMON_CONTEXT common;
    SOCKET platform_sock;
    SOCKET tpm_sock;
/* Flag indicating if a command has been cancelled.
 * This is a temporary flag, which will be changed into
 * a tcti state when support for asynch operation will be added */
    bool cancel;
} TSS2_TCTI_MSSIM_CONTEXT;

#endif /* TCTI_MSSIM_H */
