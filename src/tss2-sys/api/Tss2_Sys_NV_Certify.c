/***********************************************************************;
 * Copyright (c) 2015 - 2017, Intel Corporation
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

#include "tss2_tpm2_types.h"
#include "tss2_mu.h"
#include "sysapi_util.h"

TSS2_RC Tss2_Sys_NV_Certify_Prepare(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_DH_OBJECT signHandle,
    TPMI_RH_NV_AUTH authHandle,
    TPMI_RH_NV_INDEX nvIndex,
    const TPM2B_DATA *qualifyingData,
    const TPMT_SIG_SCHEME *inScheme,
    UINT16 size,
    UINT16 offset)
{
    _TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    if (!ctx || !inScheme)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonPreparePrologue(ctx, TPM2_CC_NV_Certify);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT32_Marshal(signHandle, ctx->cmdBuffer,
                                  ctx->maxCmdSize,
                                  &ctx->nextData);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT32_Marshal(authHandle, ctx->cmdBuffer,
                                  ctx->maxCmdSize,
                                  &ctx->nextData);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT32_Marshal(nvIndex, ctx->cmdBuffer,
                                  ctx->maxCmdSize,
                                  &ctx->nextData);
    if (rval)
        return rval;

    if (!qualifyingData) {
        ctx->decryptNull = 1;

        rval = Tss2_MU_UINT16_Marshal(0, ctx->cmdBuffer,
                                      ctx->maxCmdSize,
                                      &ctx->nextData);
    } else {

        rval = Tss2_MU_TPM2B_DATA_Marshal(qualifyingData, ctx->cmdBuffer,
                                          ctx->maxCmdSize,
                                          &ctx->nextData);
    }

    if (rval)
        return rval;

    rval = Tss2_MU_TPMT_SIG_SCHEME_Marshal(inScheme, ctx->cmdBuffer,
                                           ctx->maxCmdSize,
                                           &ctx->nextData);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT16_Marshal(size, ctx->cmdBuffer,
                                  ctx->maxCmdSize,
                                  &ctx->nextData);
    if (rval)
        return rval;

    rval = Tss2_MU_UINT16_Marshal(offset, ctx->cmdBuffer,
                                  ctx->maxCmdSize,
                                  &ctx->nextData);
    if (rval)
        return rval;

    ctx->decryptAllowed = 1;
    ctx->encryptAllowed = 1;
    ctx->authAllowed = 1;

    return CommonPrepareEpilogue(ctx);
}

TSS2_RC Tss2_Sys_NV_Certify_Complete(
    TSS2_SYS_CONTEXT *sysContext,
    TPM2B_ATTEST *certifyInfo,
    TPMT_SIGNATURE *signature)
{
    _TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    if (!ctx)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = CommonComplete(ctx);
    if (rval)
        return rval;

    rval = Tss2_MU_TPM2B_ATTEST_Unmarshal(ctx->cmdBuffer,
                                          ctx->maxCmdSize,
                                          &ctx->nextData,
                                          certifyInfo);
    if (rval)
        return rval;

    return Tss2_MU_TPMT_SIGNATURE_Unmarshal(ctx->cmdBuffer,
                                            ctx->maxCmdSize,
                                            &ctx->nextData,
                                            signature);
}

TSS2_RC Tss2_Sys_NV_Certify(
    TSS2_SYS_CONTEXT *sysContext,
    TPMI_DH_OBJECT signHandle,
    TPMI_RH_NV_AUTH authHandle,
    TPMI_RH_NV_INDEX nvIndex,
    TSS2L_SYS_AUTH_COMMAND const *cmdAuthsArray,
    const TPM2B_DATA *qualifyingData,
    const TPMT_SIG_SCHEME *inScheme,
    UINT16 size,
    UINT16 offset,
    TPM2B_ATTEST *certifyInfo,
    TPMT_SIGNATURE *signature,
    TSS2L_SYS_AUTH_RESPONSE *rspAuthsArray)
{
    _TSS2_SYS_CONTEXT_BLOB *ctx = syscontext_cast(sysContext);
    TSS2_RC rval;

    if (!inScheme)
        return TSS2_SYS_RC_BAD_REFERENCE;

    rval = Tss2_Sys_NV_Certify_Prepare(sysContext, signHandle, authHandle,
                                       nvIndex, qualifyingData, inScheme,
                                       size, offset);
    if (rval)
        return rval;

    rval = CommonOneCall(ctx, cmdAuthsArray, rspAuthsArray);
    if (rval)
        return rval;

    return Tss2_Sys_NV_Certify_Complete(sysContext, certifyInfo, signature);
}
