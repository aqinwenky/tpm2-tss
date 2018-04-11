/***********************************************************************;
 * Copyright (c) 2015 - 2018, Intel Corporation
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
 ***********************************************************************/
#include <string.h>

#include "tss2_tpm2_types.h"
#include "tss2_mu.h"
#include "sysapi_util.h"
#include "util/tss2_endian.h"

TSS2_RC Tss2_Sys_SetEncryptParam(
    TSS2_SYS_CONTEXT *sysContext,
    size_t encryptParamSize,
    const uint8_t *encryptParamBuffer)
{
    _TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    size_t currEncryptParamSize;
    const uint8_t *currEncryptParamBuffer;
    TSS2_RC rval;

    if (!encryptParamBuffer || !ctx)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = Tss2_Sys_GetEncryptParam(sysContext,
                                    &currEncryptParamSize,
                                    &currEncryptParamBuffer);
    if (rval)
        return rval;

    if (encryptParamSize != currEncryptParamSize)
        return TSS2_SYS_RC_BAD_SIZE;

    if (currEncryptParamBuffer + encryptParamSize >
            ctx->cmdBuffer + ctx->maxCmdSize)
        return TSS2_SYS_RC_INSUFFICIENT_CONTEXT;

    memmove((void *)currEncryptParamBuffer,
            encryptParamBuffer, encryptParamSize);

    return TSS2_RC_SUCCESS;
}
