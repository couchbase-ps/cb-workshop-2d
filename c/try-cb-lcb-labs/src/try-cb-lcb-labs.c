/**
 * Copyright (C) 2021 Couchbase, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALING
 * IN THE SOFTWARE.
 */

#include "try-cb-lcb-labs.h"
#include "util.h"

#if defined(__linux__)
#include <kore/seccomp.h>

// syscalls required by libcouchbase
// See https://docs.kore.io/4.1.0/api/seccomp.html
// List of syscalls retrieved by running with `seccomp_tracing yes` in config.
KORE_SECCOMP_FILTER("try-cb-lcb-labs",
    KORE_SYSCALL_ALLOW(getgid),
    KORE_SYSCALL_ALLOW(getegid),
    KORE_SYSCALL_ALLOW(epoll_create1),
    KORE_SYSCALL_ALLOW(pipe2),
    KORE_SYSCALL_ALLOW(uname),
    KORE_SYSCALL_ALLOW(socket),
    KORE_SYSCALL_ALLOW(connect),
    KORE_SYSCALL_ALLOW(getsockname),
    KORE_SYSCALL_ALLOW(getpeername),
    KORE_SYSCALL_ALLOW(sendmsg),
    KORE_SYSCALL_ALLOW(recvmsg),
    KORE_SYSCALL_ALLOW(gettimeofday),
)
#endif /* linux */

_Thread_local lcb_INSTANCE *_tcblcb_lcb_instance = NULL;

// See `docker-compose.yml` for the `db` alias that resolves to the couchbase-server docker hostname.
static const char   DEFAULT_SCHEME_STRING[] = "couchbase://";
static const size_t DEFAULT_SCHEME_STRLEN   = sizeof(DEFAULT_SCHEME_STRING) - 1;
static const char   DEFAULT_HOST_STRING[]   = "db";
static const size_t DEFAULT_HOST_STRLEN     = sizeof(DEFAULT_HOST_STRING) - 1;
static const char   DEFAULT_USER_STRING[]   = "Administrator";
static const size_t DEFAULT_USER_STRLEN     = sizeof(DEFAULT_USER_STRING) - 1;
static const char   DEFAULT_PSWD_STRING[]   = "password";
static const size_t DEFAULT_PSWD_STRLEN     = sizeof(DEFAULT_PSWD_STRING) - 1;
static const char   TRAVEL_BUCKET_STRING[]  = "travel-sample";
static const size_t TRAVEL_BUCKET_STRLEN    = sizeof(TRAVEL_BUCKET_STRING) - 1;
static const char   APPLICATION_USER_STRING[]   = "application";
static const size_t APPLICATION_USER_STRLEN     = sizeof(APPLICATION_USER_STRING) - 1;

static const char   ENV_CB_SCHEME[] = "CB_SCHEME";
static const char   ENV_CB_HOST[]   = "CB_HOST";
static const char   ENV_CB_USER[]   = "CB_USER";
static const char   ENV_CB_PSWD[]   = "CB_PSWD";

static const char  *_cb_scheme_string = DEFAULT_SCHEME_STRING;
static size_t       _cb_scheme_strlen = DEFAULT_SCHEME_STRLEN;
static const char  *_cb_host_string   = DEFAULT_HOST_STRING;
static size_t       _cb_host_strlen   = DEFAULT_HOST_STRLEN;
static const char  *_cb_user_string   = APPLICATION_USER_STRING;
static size_t       _cb_user_strlen   = APPLICATION_USER_STRLEN;
static const char  *_cb_pswd_string   = DEFAULT_PSWD_STRING;
static size_t       _cb_pswd_strlen   = DEFAULT_PSWD_STRLEN;

static const char * _cb_conn_string = NULL;
static size_t       _cb_conn_strlen = 0;

// for testing get
static const char   RSPMSG_REQ_ERROR_STRING[] = "{\"message\":"
                    " \"Error Calling Test Get\"}";
static const size_t RSPMSG_REQ_ERROR_STRLEN = sizeof(RSPMSG_REQ_ERROR_STRING) - 1;

typedef struct tcblcb_TestGetResult {
    lcb_STATUS status;
    char *response;
} tcblcb_TestGetResult;

static void open_callback(__unused lcb_INSTANCE *instance, lcb_STATUS rc)
{
    kore_log(LOG_NOTICE, "Open bucket callback result was: %s", lcb_strerror_short(rc));
}

static void get_callback(lcb_INSTANCE *instance, __unused int cbtype, const lcb_RESPGET *resp)
{
    tcblcb_RESPDELEGATE *resp_delegate = NULL;

    IfLCBFailGotoDone(
        lcb_respget_cookie(resp, (void**)&resp_delegate),
        "Failed to get response delegate cookie"
    );
    IfNULLGotoDone(
        resp_delegate,
        "Response delegate is NULL"
    );
    IfNULLGotoDone(
        resp_delegate->callback,
        "Response delegate callback is NULL"
    );

    resp_delegate->callback(instance, resp_delegate->cookie, (lcb_RESPBASE *)resp);

done:
    // receiver is responsible for freeing this memory if command is scheduled
    if (resp_delegate != NULL) {
        free(resp_delegate);
    }
}

static void store_callback(lcb_INSTANCE *instance, __unused int cbtype, const lcb_RESPSTORE *resp)
{
    tcblcb_RESPDELEGATE *resp_delegate = NULL;

    IfLCBFailGotoDone(
        lcb_respstore_cookie(resp, (void**)&resp_delegate),
        "Failed to get response delegate cookie"
    );
    IfNULLGotoDone(
        resp_delegate,
        "Response delegate is NULL"
    );
    IfNULLGotoDone(
        resp_delegate->callback,
        "Response delegate callback is NULL"
    );

    resp_delegate->callback(instance, resp_delegate->cookie, (lcb_RESPBASE *)resp);

done:
    // receiver is responsible for freeing this memory if command is scheduled
    if (resp_delegate != NULL) {
        free(resp_delegate);
    }
}

static void subdoc_callback(lcb_INSTANCE *instance, __unused int cbtype, const lcb_RESPSUBDOC *resp)
{
    tcblcb_RESPDELEGATE *resp_delegate = NULL;

    IfLCBFailGotoDone(
        lcb_respsubdoc_cookie(resp, (void**)&resp_delegate),
        "Failed to get response delegate cookie"
    );
    IfNULLGotoDone(
        resp_delegate,
        "Response delegate is NULL"
    );
    IfNULLGotoDone(
        resp_delegate->callback,
        "Response delegate callback is NULL"
    );

    resp_delegate->callback(instance, resp_delegate->cookie, (lcb_RESPBASE *)resp);

done:
    // receiver is responsible for freeing this memory if command is scheduled
    if (resp_delegate != NULL) {
        free(resp_delegate);
    }
}

static void destroy_cb_instance()
{
    // LAB - Couchbase shutdown - Destroy connection/instance
    // if (_tcblcb_lcb_instance != NULL) {
    //     lcb_destroy(_tcblcb_lcb_instance);
    //     _tcblcb_lcb_instance = NULL;
    // }
}

void kore_parent_configure(__unused int argc, __unused char *argv[])
{
    // use current time as the random number generator seed
    srand(time(NULL));

    // kore has it's own command line options processing so we'll use env variables instead
    char *cb_scheme = getenv(ENV_CB_SCHEME);
    if (cb_scheme != NULL && cb_scheme[0] != '\0') {
        _cb_scheme_string = cb_scheme;
        _cb_scheme_strlen = strlen(cb_scheme);
    }

    char *cb_host = getenv(ENV_CB_HOST);
    if (cb_host != NULL && cb_host[0] != '\0') {
        _cb_host_string = cb_host;
        _cb_host_strlen = strlen(cb_host);
    }

    char *cb_user = getenv(ENV_CB_USER);
    if (cb_user != NULL && cb_user[0] != '\0') {
        _cb_user_string = cb_user;
        _cb_user_strlen = strlen(cb_user);
    }

    char *cb_pswd = getenv(ENV_CB_PSWD);
    if (cb_pswd != NULL && cb_pswd[0] != '\0') {
        _cb_pswd_string = cb_pswd;
        _cb_pswd_strlen = strlen(cb_pswd);
    }

    struct kore_buf *conn_buf = kore_buf_alloc(BUFSIZ);
    kore_buf_appendf(conn_buf, "%.*s%.*s",
        _cb_scheme_strlen, _cb_scheme_string,
        _cb_host_strlen, _cb_host_string);
    _cb_conn_string = kore_buf_stringify(conn_buf, NULL);
    _cb_conn_strlen = strlen(_cb_conn_string);
    
    kore_log(LOG_INFO, "Couchbase Connection: %s", _cb_conn_string);
    kore_log(LOG_INFO, "Couchbase Username: %s", _cb_user_string);
}

void kore_worker_configure()
{
    bool connected = false;
    lcb_STATUS connection_status;
    // LAB - Couchbase bootstrap - Configure the connection
    // lcb_CREATEOPTS *create_options = NULL;
    // lcb_createopts_create(&create_options, LCB_TYPE_CLUSTER);
    // lcb_createopts_connstr(create_options, _cb_conn_string, _cb_conn_strlen);
    // lcb_createopts_credentials(
    //     create_options,
    //     _cb_user_string, _cb_user_strlen,
    //     _cb_pswd_string, _cb_pswd_strlen
    // );

    // Note that we're creating the instance as a thread local in the worker threads
    
    // LAB - Couchbase bootstrap - Create the instance
    // IfLCBFailGotoDone(
    //     lcb_create(&_tcblcb_lcb_instance, create_options),
    //     "Failed to create a libcouchbase instance"
    // );
    // IfLCBFailLogWarningMsg(
    //     lcb_createopts_destroy(create_options),
    //     "Failed to destroy libcouchbase create options"
    // );
    IfNULLGotoDone(
        _tcblcb_lcb_instance,
        "libcouchbase instance is NULL"
    );

    // LAB - Couchbase bootstrap - Connect to the cluster
    // schedule the initial connect operation
    // IfLCBFailGotoDone(
    //     lcb_connect(_tcblcb_lcb_instance),
    //     "Failed to schedule the Couchbase connect operation"
    // );

    // wait for the initial connect operation to complete
    IfLCBFailGotoDone(
        lcb_wait(_tcblcb_lcb_instance, LCB_WAIT_DEFAULT),
        "Failed to establish initial connection to Couchbase"
    );

    // LAB - Couchbase bootstrap - Check the bootstrap status
    // confirm the resulting bootstrap status
    // IfLCBFailGotoDone(
    //     lcb_get_bootstrap_status(_tcblcb_lcb_instance),
    //     "Couchbase bootstrap failed"
    // );

    // LAB - Couchbase bootstrap - Install callbacks
    // install any global callbacks that are needed to delegate to other logic
    // lcb_set_open_callback(_tcblcb_lcb_instance, open_callback);
    // lcb_install_callback(_tcblcb_lcb_instance, LCB_CALLBACK_GET, (lcb_RESPCALLBACK)get_callback);
    // lcb_install_callback(_tcblcb_lcb_instance, LCB_CALLBACK_STORE, (lcb_RESPCALLBACK)store_callback);
    // lcb_install_callback(_tcblcb_lcb_instance, LCB_CALLBACK_SDLOOKUP, (lcb_RESPCALLBACK)subdoc_callback);
    // lcb_install_callback(_tcblcb_lcb_instance, LCB_CALLBACK_SDMUTATE, (lcb_RESPCALLBACK)subdoc_callback);

    // LAB - Couchbase bootstrap - Open bucket
    // schedule an open bucket operation
    // IfLCBFailGotoDone(
    //     connection_status = lcb_open(_tcblcb_lcb_instance, TRAVEL_BUCKET_STRING, TRAVEL_BUCKET_STRLEN),
    //     "Failed to schedule the open bucket operation"
    // );

    // wait for the open bucket operation to complete
    IfLCBFailGotoDone(
        lcb_wait(_tcblcb_lcb_instance, LCB_WAIT_DEFAULT),
        "Open bucket operation failed"
    );
    
    connected = true;

done:
    if (!connected) {
        destroy_cb_instance();
    }
    kore_log(LOG_NOTICE, "LCB connection attempt complete with status: (%d) %s", connection_status, lcb_strerror_long(connection_status));
}

void kore_worker_teardown()
{
    destroy_cb_instance();
}

int tcblcb_page_index(struct http_request *req)
{
    const char body[] = "<h1> Kore.io Travel Sample API </h1>"
    "A sample API for getting started with Couchbase Server that demonstrates using the <a href=\"https://docs.couchbase.com/c-sdk/current/hello-world/start-using-sdk.html\">C SDK</a> with the <a href=\"https://kore.io/\">Kore.io</a> framework to create a RESTful service API."
    "<ul>"
    "<li> <a href = \"/apidocs\"> Learn the API with Swagger, interactively </a>"
    "<li> <a href = \"https://github.com/couchbaselabs/try-cb-lcb\"> GitHub </a>"
    "</ul>";

    http_response(req, 200, body, sizeof(body));
    return (KORE_RESULT_OK);
}


// Test Connection - get airline_10
// called from a global callback and should not reference any other locals
static void test_get_airline_callback(__unused lcb_INSTANCE *instance, void *cookie, const lcb_RESPGET *resp)
{
    tcblcb_TestGetResult *result = (tcblcb_TestGetResult *)cookie;
    size_t result_len;
    const char *value;
    IfNULLGotoDone(
        cookie,
        "Test get airline result cookie was NULL"
    );

    IfLCBFailGotoDone(
        (result->status = lcb_respget_status(resp)),
        "Test get airline operation failed"
    );
    lcb_respget_value(resp, &value, &result_len);
    result->response =  (char*)malloc(result_len);
    strncpy(result->response, value,result_len);
    result->response[result_len-1] = '\0';
    kore_log(LOG_NOTICE,"Got test value from Server for airline_10: %s\n", result->response);

done:
    // no clean up to do in this block
    return;
}

static tcblcb_TestGetResult test_get_airline(lcb_INSTANCE *instance )
{
    tcblcb_TestGetResult results;
    results.status = LCB_ERR_GENERIC;
    results.response = NULL;
    lcb_CMDGET *cmd = NULL;
    tcblcb_RESPDELEGATE *get_delegate = NULL;
    bool cmd_scheduled = false;

    // LAB - Get - Create Command
    IfLCBFailGotoDone(
         lcb_cmdget_create(&cmd),
         "Failed to create get command"
     );

    // LAB - Get document - Specify the Document ID
    IfLCBFailGotoDone(
         lcb_cmdget_key(cmd, "airline_10", 10),
         "Failed to set document key"
     );

    LogDebug("%s","Get airline_10 via get command.");

    // receiver is responsible for freeing this memory if command is scheduled
    get_delegate = malloc(sizeof(tcblcb_RESPDELEGATE));
    get_delegate->cookie = (void*)&results;
    get_delegate->callback = (tcblcb_RESPDELEGATE_CALLBACK)test_get_airline_callback;
    // LAB - Get document - Run get operation
    IfLCBFailGotoDone(
        lcb_get(instance, get_delegate, cmd),
        "Failed to schedule get command"
    )

    cmd_scheduled = true;

done:

    if (cmd != NULL) {
        IfLCBFailLogWarningMsg(
            lcb_cmdget_destroy(cmd),
            "Failed to destroy get command"
        );
    }

    if (cmd_scheduled) {
        IfLCBFailLogWarningMsg(
            lcb_wait(instance, LCB_WAIT_DEFAULT),
            "Failed to complete get command"
        );
    } else {
        // free memory if command was not scheduled
        if (get_delegate != NULL) {
            free(get_delegate);
        }
    }

    return results;
}

int tcblcb_api_test_get(struct http_request *req)
{
    ProcessCORSAndExitIfPreflight(req);

    tcblcb_HTTPResponse hresp;
    hresp.status = 400;
    hresp.string = RSPMSG_REQ_ERROR_STRING;
    hresp.strlen = RSPMSG_REQ_ERROR_STRLEN;

    struct kore_buf *context_buf = NULL;

    cJSON *response_json = NULL;
    char *response_string = NULL;
    size_t response_strlen = 0;

    tcblcb_TestGetResult test_get_result = test_get_airline(_tcblcb_lcb_instance);
    kore_log(LOG_NOTICE,"Got value from function: %s\n", test_get_result.response);
    lcb_STATUS pword_status = test_get_result.status;

    LogDebug("Test Get Status: (%d) %s", pword_status, lcb_strerror_long(pword_status));
    if (pword_status == LCB_SUCCESS) {
        
        // note that if we fail to prepare a JSON response we could have an edge case where
        // the insert actually succeeded but an error will still be returned 
        hresp.status = 500;
        hresp.string = RSPMSG_REQ_ERROR_STRING;
        hresp.strlen = RSPMSG_REQ_ERROR_STRLEN;

        context_buf = kore_buf_alloc(BUFSIZ);
        kore_buf_appendf(context_buf, "%s", test_get_result.response);

        size_t context_strlen;
        char *context_string = kore_buf_stringify(context_buf, &context_strlen);

        // create main response object
        response_json = cJSON_CreateObject();
        // add data to response object
        // add context to response object
        cJSON *context_array = cJSON_AddArrayToObject(response_json, "context");
        IfNULLGotoDone(context_array, "Failed to create response context array");
        IfFalseGotoDone(
            cJSON_AddItemToArray(context_array, cJSON_CreateStringReference(context_string)),
            "Failed to add response context string to array"
        );

        response_string = cJSON_PrintBuffered(response_json, BUFSIZ, FMT_RESPONSE);
        response_strlen = strlen(response_string);
        IfTrueGotoDone(
            (response_string == NULL || response_strlen == 0),
            "Unable to create response JSON string"
        );

        hresp.status = 200;
        hresp.string = response_string;
        hresp.strlen = response_strlen;
    } else if (pword_status == LCB_ERR_DOCUMENT_NOT_FOUND) {
        hresp.status = 401;
        hresp.string = RSPMSG_REQ_ERROR_STRING;
        hresp.strlen = RSPMSG_REQ_ERROR_STRLEN;
    } else {
        hresp.status = 500;
        hresp.string = RSPMSG_REQ_ERROR_STRING;
        hresp.strlen = RSPMSG_REQ_ERROR_STRLEN;
    }

done:
    http_response(req, hresp.status, hresp.string, hresp.strlen);

    if (context_buf != NULL) {
        kore_buf_free(context_buf);
    }

    if (response_string != NULL) {
        free(response_string);
    }
    
    if (response_json != NULL) {
        cJSON_Delete(response_json);
    }

    return (KORE_RESULT_OK);
}
