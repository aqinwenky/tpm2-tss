/*******************************************************************************
 * Copyright 2017-2018, Fraunhofer SIT sponsored by Infineon Technologies AG
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
 ******************************************************************************/

#include "tss2_mu.h"
#include "tss2_sys.h"
#include "tss2_esys.h"

#include "esys_types.h"
#include "esys_iutil.h"
#include "esys_mu.h"
#define LOGMODULE esys
#include "util/log.h"

/** Store command parameters inside the ESYS_CONTEXT for use during _Finish */
static void store_input_parameters (
    ESYS_CONTEXT *esysContext,
    ESYS_TR authObject,
    ESYS_TR policySession,
    const TPM2B_NONCE *nonceTPM,
    const TPM2B_DIGEST *cpHashA,
    const TPM2B_NONCE *policyRef,
    INT32 expiration,
    const TPMT_SIGNATURE *auth)
{
    esysContext->in.PolicySigned.authObject = authObject;
    esysContext->in.PolicySigned.policySession = policySession;
    esysContext->in.PolicySigned.expiration = expiration;
    if (nonceTPM == NULL) {
        esysContext->in.PolicySigned.nonceTPM = NULL;
    } else {
        esysContext->in.PolicySigned.nonceTPMData = *nonceTPM;
        esysContext->in.PolicySigned.nonceTPM =
            &esysContext->in.PolicySigned.nonceTPMData;
    }
    if (cpHashA == NULL) {
        esysContext->in.PolicySigned.cpHashA = NULL;
    } else {
        esysContext->in.PolicySigned.cpHashAData = *cpHashA;
        esysContext->in.PolicySigned.cpHashA =
            &esysContext->in.PolicySigned.cpHashAData;
    }
    if (policyRef == NULL) {
        esysContext->in.PolicySigned.policyRef = NULL;
    } else {
        esysContext->in.PolicySigned.policyRefData = *policyRef;
        esysContext->in.PolicySigned.policyRef =
            &esysContext->in.PolicySigned.policyRefData;
    }
    if (auth == NULL) {
        esysContext->in.PolicySigned.auth = NULL;
    } else {
        esysContext->in.PolicySigned.authData = *auth;
        esysContext->in.PolicySigned.auth =
            &esysContext->in.PolicySigned.authData;
    }
}

/** One-Call function for TPM2_PolicySigned
 *
 * This function invokes the TPM2_PolicySigned command in a one-call
 * variant. This means the function will block until the TPM response is
 * available. All input parameters are const. The memory for non-simple output
 * parameters is allocated by the function implementation.
 *
 * @param[in,out] esysContext The ESYS_CONTEXT.
 * @param[in] authObject Input handle of type ESYS_TR for
 *     object with handle type TPMI_DH_OBJECT.
 * @param[in] policySession Input handle of type ESYS_TR for
 *     object with handle type TPMI_SH_POLICY.
 * @param[in] shandle1 First session handle.
 * @param[in] shandle2 Second session handle.
 * @param[in] shandle3 Third session handle.
 * @param[in] nonceTPM Input parameter of type TPM2B_NONCE.
 * @param[in] cpHashA Input parameter of type TPM2B_DIGEST.
 * @param[in] policyRef Input parameter of type TPM2B_NONCE.
 * @param[in] expiration Input parameter of type INT32.
 * @param[in] auth Input parameter of type TPMT_SIGNATURE.
 * @param[out] timeout (callee-allocated) Output parameter
 *    of type TPM2B_TIMEOUT. May be NULL if this value is not required.
 * @param[out] policyTicket (callee-allocated) Output parameter
 *    of type TPMT_TK_AUTH. May be NULL if this value is not required.
 * @retval TSS2_RC_SUCCESS on success
 * @retval TSS2_RC_BAD_SEQUENCE if context is not ready for this function
 * \todo add further error RCs to documentation
 */
TSS2_RC
Esys_PolicySigned(
    ESYS_CONTEXT *esysContext,
    ESYS_TR authObject,
    ESYS_TR policySession,
    ESYS_TR shandle1,
    ESYS_TR shandle2,
    ESYS_TR shandle3,
    const TPM2B_NONCE *nonceTPM,
    const TPM2B_DIGEST *cpHashA,
    const TPM2B_NONCE *policyRef,
    INT32 expiration,
    const TPMT_SIGNATURE *auth,
    TPM2B_TIMEOUT **timeout,
    TPMT_TK_AUTH **policyTicket)
{
    TSS2_RC r;

    r = Esys_PolicySigned_Async(esysContext,
                authObject,
                policySession,
                shandle1,
                shandle2,
                shandle3,
                nonceTPM,
                cpHashA,
                policyRef,
                expiration,
                auth);
    return_if_error(r, "Error in async function");

    /* Set the timeout to indefinite for now, since we want _Finish to block */
    int32_t timeouttmp = esysContext->timeout;
    esysContext->timeout = -1;
    /*
     * Now we call the finish function, until return code is not equal to
     * from TSS2_BASE_RC_TRY_AGAIN.
     * Note that the finish function may return TSS2_RC_TRY_AGAIN, even if we
     * have set the timeout to -1. This occurs for example if the TPM requests
     * a retransmission of the command via TPM2_RC_YIELDED.
     */
    do {
        r = Esys_PolicySigned_Finish(esysContext,
                timeout,
                policyTicket);
        /* This is just debug information about the reattempt to finish the
           command */
        if ((r & ~TSS2_RC_LAYER_MASK) == TSS2_BASE_RC_TRY_AGAIN)
            LOG_DEBUG("A layer below returned TRY_AGAIN: %" PRIx32
                      " => resubmitting command", r);
    } while ((r & ~TSS2_RC_LAYER_MASK) == TSS2_BASE_RC_TRY_AGAIN);

    /* Restore the timeout value to the original value */
    esysContext->timeout = timeouttmp;
    return_if_error(r, "Esys Finish");

    return TSS2_RC_SUCCESS;
}

/** Asynchronous function for TPM2_PolicySigned
 *
 * This function invokes the TPM2_PolicySigned command in a asynchronous
 * variant. This means the function will return as soon as the command has been
 * sent downwards the stack to the TPM. All input parameters are const.
 * In order to retrieve the TPM's response call Esys_PolicySigned_Finish.
 *
 * @param[in,out] esysContext The ESYS_CONTEXT.
 * @param[in] authObject Input handle of type ESYS_TR for
 *     object with handle type TPMI_DH_OBJECT.
 * @param[in] policySession Input handle of type ESYS_TR for
 *     object with handle type TPMI_SH_POLICY.
 * @param[in] shandle1 First session handle.
 * @param[in] shandle2 Second session handle.
 * @param[in] shandle3 Third session handle.
 * @param[in] nonceTPM Input parameter of type TPM2B_NONCE.
 * @param[in] cpHashA Input parameter of type TPM2B_DIGEST.
 * @param[in] policyRef Input parameter of type TPM2B_NONCE.
 * @param[in] expiration Input parameter of type INT32.
 * @param[in] auth Input parameter of type TPMT_SIGNATURE.
 * @retval TSS2_RC_SUCCESS on success
 * @retval TSS2_RC_BAD_SEQUENCE if context is not ready for this function
 * \todo add further error RCs to documentation
 */
TSS2_RC
Esys_PolicySigned_Async(
    ESYS_CONTEXT *esysContext,
    ESYS_TR authObject,
    ESYS_TR policySession,
    ESYS_TR shandle1,
    ESYS_TR shandle2,
    ESYS_TR shandle3,
    const TPM2B_NONCE *nonceTPM,
    const TPM2B_DIGEST *cpHashA,
    const TPM2B_NONCE *policyRef,
    INT32 expiration,
    const TPMT_SIGNATURE *auth)
{
    TSS2_RC r;
    LOG_TRACE("context=%p, authObject=%"PRIx32 ", policySession=%"PRIx32 ","
              "nonceTPM=%p, cpHashA=%p, policyRef=%p,"
              "expiration=%"PRIi32 ", auth=%p",
              esysContext, authObject, policySession, nonceTPM, cpHashA,
              policyRef, expiration, auth);
    TSS2L_SYS_AUTH_COMMAND auths;
    RSRC_NODE_T *authObjectNode;
    RSRC_NODE_T *policySessionNode;

    /* Check context, sequence correctness and set state to error for now */
    if (esysContext == NULL) {
        LOG_ERROR("esyscontext is NULL.");
        return TSS2_ESYS_RC_BAD_REFERENCE;
    }
    r = iesys_check_sequence_async(esysContext);
    if (r != TSS2_RC_SUCCESS)
        return r;
    esysContext->state = _ESYS_STATE_INTERNALERROR;

    /* Check and store input parameters */
    r = check_session_feasibility(shandle1, shandle2, shandle3, 0);
    return_state_if_error(r, _ESYS_STATE_INIT, "Check session usage");
    store_input_parameters(esysContext, authObject, policySession,
                nonceTPM,
                cpHashA,
                policyRef,
                expiration,
                auth);

    /* Retrieve the metadata objects for provided handles */
    r = esys_GetResourceObject(esysContext, authObject, &authObjectNode);
    return_state_if_error(r, _ESYS_STATE_INIT, "authObject unknown.");
    r = esys_GetResourceObject(esysContext, policySession, &policySessionNode);
    return_state_if_error(r, _ESYS_STATE_INIT, "policySession unknown.");

    /* Initial invocation of SAPI to prepare the command buffer with parameters */
    r = Tss2_Sys_PolicySigned_Prepare(esysContext->sys,
                (authObjectNode == NULL) ? TPM2_RH_NULL : authObjectNode->rsrc.handle,
                (policySessionNode == NULL) ? TPM2_RH_NULL : policySessionNode->rsrc.handle,
                nonceTPM,
                cpHashA,
                policyRef,
                expiration,
                auth);
    return_state_if_error(r, _ESYS_STATE_INIT, "SAPI Prepare returned error.");

    /* Calculate the cpHash Values */
    r = init_session_tab(esysContext, shandle1, shandle2, shandle3);
    return_state_if_error(r, _ESYS_STATE_INIT, "Initialize session resources");
    iesys_compute_session_value(esysContext->session_tab[0], NULL, NULL);
    iesys_compute_session_value(esysContext->session_tab[1], NULL, NULL);
    iesys_compute_session_value(esysContext->session_tab[2], NULL, NULL);

    /* Generate the auth values and set them in the SAPI command buffer */
    r = iesys_gen_auths(esysContext, authObjectNode, policySessionNode, NULL, &auths);
    return_state_if_error(r, _ESYS_STATE_INIT, "Error in computation of auth values");
    esysContext->authsCount = auths.count;
    r = Tss2_Sys_SetCmdAuths(esysContext->sys, &auths);
    return_state_if_error(r, _ESYS_STATE_INIT, "SAPI error on SetCmdAuths");

    /* Trigger execution and finish the async invocation */
    r = Tss2_Sys_ExecuteAsync(esysContext->sys);
    return_state_if_error(r, _ESYS_STATE_INTERNALERROR, "Finish (Execute Async)");

    esysContext->state = _ESYS_STATE_SENT;

    return r;
}

/** Asynchronous finish function for TPM2_PolicySigned
 *
 * This function returns the results of a TPM2_PolicySigned command
 * invoked via Esys_PolicySigned_Finish. All non-simple output parameters
 * are allocated by the function's implementation. NULL can be passed for every
 * output parameter if the value is not required.
 *
 * @param[in,out] esysContext The ESYS_CONTEXT.
 * @param[out] timeout (callee-allocated) Output parameter
 *    of type TPM2B_TIMEOUT. May be NULL if this value is not required.
 * @param[out] policyTicket (callee-allocated) Output parameter
 *    of type TPMT_TK_AUTH. May be NULL if this value is not required.
 * @retval TSS2_RC_SUCCESS on success
 * @retval TSS2_RC_BAD_SEQUENCE if context is not ready for this function.
 * \todo add further error RCs to documentation
 */
TSS2_RC
Esys_PolicySigned_Finish(
    ESYS_CONTEXT *esysContext,
    TPM2B_TIMEOUT **timeout,
    TPMT_TK_AUTH **policyTicket)
{
    TSS2_RC r;
    LOG_TRACE("context=%p, timeout=%p, policyTicket=%p",
              esysContext, timeout, policyTicket);

    if (esysContext == NULL) {
        LOG_ERROR("esyscontext is NULL.");
        return TSS2_ESYS_RC_BAD_REFERENCE;
    }

    /* Check for correct sequence and set sequence to irregular for now */
    if (esysContext->state != _ESYS_STATE_SENT) {
        LOG_ERROR("Esys called in bad sequence.");
        return TSS2_ESYS_RC_BAD_SEQUENCE;
    }
    esysContext->state = _ESYS_STATE_INTERNALERROR;

    /* Allocate memory for response parameters */
    if (timeout != NULL) {
        *timeout = calloc(sizeof(TPM2B_TIMEOUT), 1);
        if (*timeout == NULL) {
            return_error(TSS2_ESYS_RC_MEMORY, "Out of memory");
        }
    }
    if (policyTicket != NULL) {
        *policyTicket = calloc(sizeof(TPMT_TK_AUTH), 1);
        if (*policyTicket == NULL) {
            goto_error(r, TSS2_ESYS_RC_MEMORY, "Out of memory", error_cleanup);
        }
    }

    /*Receive the TPM response and handle resubmissions if necessary. */
    r = Tss2_Sys_ExecuteFinish(esysContext->sys, esysContext->timeout);
    if ((r & ~TSS2_RC_LAYER_MASK) == TSS2_BASE_RC_TRY_AGAIN) {
        LOG_DEBUG("A layer below returned TRY_AGAIN: %" PRIx32, r);
        esysContext->state = _ESYS_STATE_SENT;
        goto error_cleanup;
    }
    /* This block handle the resubmission of TPM commands given a certain set of
     * TPM response codes. */
    if (r == TPM2_RC_RETRY || r == TPM2_RC_TESTING || r == TPM2_RC_YIELDED) {
        LOG_DEBUG("TPM returned RETRY, TESTING or YIELDED, which triggers a "
            "resubmission: %" PRIx32, r);
        if (esysContext->submissionCount >= _ESYS_MAX_SUBMISSIONS) {
            LOG_WARNING("Maximum number of (re)submissions has been reached.");
            esysContext->state = _ESYS_STATE_INIT;
            goto error_cleanup;
        }
        esysContext->state = _ESYS_STATE_RESUBMISSION;
        r = Esys_PolicySigned_Async(esysContext,
                esysContext->in.PolicySigned.authObject,
                esysContext->in.PolicySigned.policySession,
                esysContext->session_type[0],
                esysContext->session_type[1],
                esysContext->session_type[2],
                esysContext->in.PolicySigned.nonceTPM,
                esysContext->in.PolicySigned.cpHashA,
                esysContext->in.PolicySigned.policyRef,
                esysContext->in.PolicySigned.expiration,
                esysContext->in.PolicySigned.auth);
        if (r != TSS2_RC_SUCCESS) {
            LOG_WARNING("Error attempting to resubmit");
            /* We do not set esysContext->state here but inherit the most recent
             * state of the _async function. */
            goto error_cleanup;
        }
        r = TSS2_ESYS_RC_TRY_AGAIN;
        LOG_DEBUG("Resubmission initiated and returning RC_TRY_AGAIN.");
        goto error_cleanup;
    }
    /* The following is the "regular error" handling. */
    if (r != TSS2_RC_SUCCESS && (r & TSS2_RC_LAYER_MASK) == 0) {
        LOG_WARNING("Received TPM Error");
        esysContext->state = _ESYS_STATE_INIT;
        goto error_cleanup;
    } else if (r != TSS2_RC_SUCCESS) {
        LOG_ERROR("Received a non-TPM Error");
        esysContext->state = _ESYS_STATE_INTERNALERROR;
        goto error_cleanup;
    }

    /*
     * Now the verification of the response (hmac check) and if necessary the
     * parameter decryption have to be done.
     */
    r = iesys_check_response(esysContext);
    goto_state_if_error(r, _ESYS_STATE_INTERNALERROR, "Error: check response",
                      error_cleanup);
    /*
     * After the verification of the response we call the complete function
     * to deliver the result.
     */
    r = Tss2_Sys_PolicySigned_Complete(esysContext->sys,
                (timeout != NULL) ? *timeout : NULL,
                (policyTicket != NULL) ? *policyTicket : NULL);
    goto_state_if_error(r, _ESYS_STATE_INTERNALERROR, "Received error from SAPI"
                        " unmarshaling" ,error_cleanup);
    esysContext->state = _ESYS_STATE_INIT;

    return TSS2_RC_SUCCESS;

error_cleanup:
    if (timeout != NULL)
        SAFE_FREE(*timeout);
    if (policyTicket != NULL)
        SAFE_FREE(*policyTicket);

    return r;
}
