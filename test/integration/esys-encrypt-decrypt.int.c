/*******************************************************************************
 * Copyright 2017-2018, Fraunhofer SIT sponsored by Infineon Technologies AG All
 * rights reserved.
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
 *******************************************************************************/

#include "tss2_esys.h"

#include "esys_iutil.h"
#define LOGMODULE test
#include "util/log.h"

/*
 * This test is intended to test the ESAPI function Esys_EncryptDecrypt.
 * First a primary key is generated. This key will be uses as parent fo a
 * symmetric key, which will be used to encrypt and decrypt a tpm2b. The
 * result will be compared.
 */

int
test_invoke_esapi(ESYS_CONTEXT * esys_context)
{
    uint32_t r = 0;

   TPM2B_AUTH authValuePrimary = {
        .size = 5,
        .buffer = {1, 2, 3, 4, 5}
    };

   TPM2B_SENSITIVE_CREATE inSensitivePrimary = {
       .size = 4,
       .sensitive = {
           .userAuth = {
                .size = 0,
                .buffer = {0 },
            },
           .data = {
                .size = 0,
                .buffer = {0},
            },
       },
   };

    inSensitivePrimary.sensitive.userAuth = authValuePrimary;

    TPM2B_PUBLIC inPublic = {
        .size = 0,
        .publicArea = {
            .type = TPM2_ALG_RSA,
            .nameAlg = TPM2_ALG_SHA256,
            .objectAttributes = (TPMA_OBJECT_USERWITHAUTH |
                                 TPMA_OBJECT_RESTRICTED |
                                 TPMA_OBJECT_DECRYPT |
                                 TPMA_OBJECT_FIXEDTPM |
                                 TPMA_OBJECT_FIXEDPARENT |
                                 TPMA_OBJECT_SENSITIVEDATAORIGIN),
            .authPolicy = {
                 .size = 0,
             },
            .parameters.rsaDetail = {
                 .symmetric = {
                     .algorithm = TPM2_ALG_AES,
                     .keyBits.aes = 128,
                     .mode.aes = TPM2_ALG_CFB},
                 .scheme = {
                      .scheme = TPM2_ALG_NULL
                  },
                 .keyBits = 2048,
                 .exponent = 65537,
             },
            .unique.rsa = {
                 .size = 0,
                 .buffer = {},
             },
        },
    };

    TPM2B_DATA outsideInfo = {
        .size = 0,
        .buffer = {},
    };

    TPML_PCR_SELECTION creationPCR = {
        .count = 0,
    };

    TPM2B_AUTH authValue = {
        .size = 0,
        .buffer = {}
    };

    r = Esys_TR_SetAuth(esys_context, ESYS_TR_RH_OWNER, &authValue);
    goto_if_error(r, "Error: TR_SetAuth", error);

    ESYS_TR primaryHandle_handle;
    TPM2B_PUBLIC *outPublic;
    TPM2B_CREATION_DATA *creationData;
    TPM2B_DIGEST *creationHash;
    TPMT_TK_CREATION *creationTicket;

    r = Esys_CreatePrimary(esys_context, ESYS_TR_RH_OWNER, ESYS_TR_PASSWORD,
                           ESYS_TR_NONE, ESYS_TR_NONE,
                           &inSensitivePrimary, &inPublic,
                           &outsideInfo, &creationPCR, &primaryHandle_handle,
                           &outPublic, &creationData, &creationHash,
                           &creationTicket);
    goto_if_error(r, "Error esys create primary", error);

    r = Esys_TR_SetAuth(esys_context, primaryHandle_handle, &authValuePrimary);
    goto_if_error(r, "Error: TR_SetAuth", error);

    TPM2B_AUTH authKey2 = {
        .size = 6,
        .buffer = {6, 7, 8, 9, 10, 11}
    };

    TPM2B_SENSITIVE_CREATE inSensitive2 = {
        .size = 1,
        .sensitive = {
            .userAuth = {
                 .size = 0,
                 .buffer = {0}
             },
            .data = {
                 .size = 16,
                 .buffer = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 16}
             }
        }
    };

    inSensitive2.sensitive.userAuth = authKey2;

    TPM2B_PUBLIC inPublic2 = {
        .size = 0,
        .publicArea = {
            .type = TPM2_ALG_SYMCIPHER,
            .nameAlg = TPM2_ALG_SHA256,
            .objectAttributes = (TPMA_OBJECT_USERWITHAUTH |
                                 TPMA_OBJECT_SIGN_ENCRYPT |
                                 TPMA_OBJECT_DECRYPT),

            .authPolicy = {
                 .size = 0,
             },
            .parameters.symDetail = {
                 .sym = {
                     .algorithm = TPM2_ALG_AES,
                     .keyBits = {.aes = 128},
                     .mode = {.aes = TPM2_ALG_CFB}}
             },
            .unique.sym = {
                 .size = 0,
                 .buffer = {}
             }
        }
    };

    TPM2B_DATA outsideInfo2 = {
        .size = 0,
        .buffer = {}
        ,
    };

    TPML_PCR_SELECTION creationPCR2 = {
        .count = 0,
    };

    TPM2B_PUBLIC *outPublic2;
    TPM2B_PRIVATE *outPrivate2;
    TPM2B_CREATION_DATA *creationData2;
    TPM2B_DIGEST *creationHash2;
    TPMT_TK_CREATION *creationTicket2;

    r = Esys_Create(esys_context,
                    primaryHandle_handle,
                    ESYS_TR_PASSWORD, ESYS_TR_NONE, ESYS_TR_NONE,
                    &inSensitive2,
                    &inPublic2,
                    &outsideInfo2,
                    &creationPCR2,
                    &outPrivate2,
                    &outPublic2,
                    &creationData2, &creationHash2, &creationTicket2);
    goto_if_error(r, "Error esys create ", error);

    LOG_INFO("AES key created.");

    ESYS_TR loadedKeyHandle;

    r = Esys_Load(esys_context,
                  primaryHandle_handle,
                  ESYS_TR_PASSWORD,
                  ESYS_TR_NONE,
                  ESYS_TR_NONE, outPrivate2, outPublic2, &loadedKeyHandle);
    goto_if_error(r, "Error esys load ", error);

    LOG_INFO("AES key loaded.");

    r = Esys_TR_SetAuth(esys_context, loadedKeyHandle, &authKey2);
    goto_if_error(r, "Error esys TR_SetAuth ", error);

    ESYS_TR keyHandle_handle = loadedKeyHandle;
    TPMI_YES_NO decrypt = TPM2_YES;
    TPMI_YES_NO encrypt = TPM2_NO;
    TPMI_ALG_SYM_MODE mode = TPM2_ALG_NULL;
    TPM2B_IV ivIn = {
        .size = 16,
        .buffer = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 16}
    };
    TPM2B_MAX_BUFFER inData = {
        .size = 16,
        .buffer = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 16}
    };
    TPM2B_MAX_BUFFER *outData;
    TPM2B_IV *ivOut;

    r = Esys_EncryptDecrypt(
        esys_context,
        keyHandle_handle,
        ESYS_TR_PASSWORD,
        ESYS_TR_NONE,
        ESYS_TR_NONE,
        encrypt,
        mode,
        &ivIn,
        &inData,
        &outData,
        &ivOut);
    goto_if_error(r, "Error: EncryptDecrypt", error);

    TPM2B_MAX_BUFFER *outData2;
    TPM2B_IV *ivOut2;

    r = Esys_EncryptDecrypt(
        esys_context,
        keyHandle_handle,
        ESYS_TR_PASSWORD,
        ESYS_TR_NONE,
        ESYS_TR_NONE,
        decrypt,
        mode,
        &ivIn,
        outData,
        &outData2,
        &ivOut2);
    goto_if_error(r, "Error: EncryptDecrypt", error);

    LOGBLOB_DEBUG(&outData2->buffer[0], outData2->size, "** Decrypted data **");

    if (outData2->size != inData.size ||
        memcmp(&outData2->buffer, &inData.buffer[0], outData2->size) != 0) {
        LOG_ERROR("Error: decrypted text not  equal to origin");
        goto error;
    }

    r = Esys_FlushContext(esys_context, primaryHandle_handle);
    goto_if_error(r, "Error during FlushContext", error);

    r = Esys_FlushContext(esys_context, loadedKeyHandle);
    goto_if_error(r, "Error during FlushContext", error);

    return 0;

 error:
    return 1;
}
