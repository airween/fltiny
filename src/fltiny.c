#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>	// need for va_list...

#include <unistd.h>
#include <signal.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>
#include <xmlrpc-c/client_global.h>

#include "serialmodem.h"

#define NAME "xmlrpcclient"
#define XMLRPCVERSION "1.0"

#define TX   1
#define RX   2

typedef struct xmlrpc_res_s {
    int			intval;
    const char		*stringval;
    const unsigned char	*byteval;
} xmlrpc_res;

static volatile int keeprun = 1;

const char fldigi_url[50] = "http://localhost:7362/RPC2";
/*
 * Used XML RPC methods, and its formats of arguments
 * ==================================================
   tx.get_data       6:n   (bytes:) - get TX data since last query
 main.get_trx_state  s:n  - get RX/TX state, 's' could be "RX" | "TX"
*/

xmlrpc_env env;
xmlrpc_server_info * serverInfoP;
int ptt;

int fldigi_xmlrpc_init() {
    xmlrpc_client_init2(&env, XMLRPC_CLIENT_NO_FLAGS, NAME, XMLRPCVERSION, NULL, 0);
    serverInfoP = xmlrpc_server_info_new(&env, fldigi_url);
    if (env.fault_occurred != 0) {
        return -1;
    }
    return 0;
}

int fldigi_xmlrpc_cleanup() {
    if (serverInfoP != NULL) {
        xmlrpc_server_info_free(serverInfoP);
    }
    return 0;
}

int fldigi_xmlrpc_query(xmlrpc_res * local_result, xmlrpc_env * local_env, char * methodname, char * format, ...) {

    static int connerr = 0;
    static unsigned int connerrcnt = 0;
    xmlrpc_value * callresult;
    xmlrpc_value * pcall_array = NULL;
    xmlrpc_value * va_param = NULL;
    va_list argptr;
    int restype;
    size_t bytesize = 0;

    /*
    if connerr had been set up to 1, that means an error
    occured at last xmlrpc_call() method
    if that true, then we count the number of calling this
    function (xmlrpc()), if counter reaches 100, then clear
    it, and try again
    this handles the xmlrpc_call() errors, eg. Fldigi is
    unreacheable, but it will check again and again, not
    need to restart Tlf
    */
    if (connerr == 1) {
        if (connerrcnt == 10) {
            connerr = 0;
            connerrcnt = 0;
        }
        else {
            connerrcnt++;
        }
    }

    if (connerr == 0) {
        va_start(argptr, format);

        xmlrpc_env_init(local_env);
        pcall_array = xmlrpc_array_new(local_env);
        while(*format != '\0') {
            if(*format == 's') {
                char* s = va_arg(argptr, char *);
                va_param = xmlrpc_string_new(local_env, s);
                if (local_env->fault_occurred) {
                    return -1;
                }
                xmlrpc_array_append_item(local_env, pcall_array, va_param);
                if (local_env->fault_occurred) {
                    return -1;
                }
                xmlrpc_DECREF(va_param);
            }
            else if(*format == 'd') {
                int d = va_arg(argptr, int);
                va_param = xmlrpc_int_new(local_env, d);
                xmlrpc_array_append_item(local_env, pcall_array, va_param);
                if (local_env->fault_occurred) {
                    return -1;
                }
                xmlrpc_DECREF(va_param);
            }
            format++;
        }
        va_end(argptr);

        callresult = xmlrpc_client_call_server_params(local_env, serverInfoP, methodname, pcall_array);
        if (local_env->fault_occurred) {
            // error till xmlrpc_call
            connerr = 1;
            xmlrpc_DECREF(pcall_array);
            xmlrpc_env_clean(local_env);
            return -1;
        }
        else {
            restype = xmlrpc_value_type(callresult);
            if (restype == 0xDEAD) {
                xmlrpc_DECREF(callresult);
                xmlrpc_DECREF(pcall_array);
                xmlrpc_env_clean(local_env);
                return -1;
            }
            else {
                local_result->intval = 0;
            }
            switch(restype) {
                // int
                case XMLRPC_TYPE_INT:    xmlrpc_read_int(local_env, callresult, &local_result->intval);
                                         break;
                // string
                case XMLRPC_TYPE_STRING: xmlrpc_read_string(local_env, callresult, &local_result->stringval);
                                         break;
                // byte stream
                case XMLRPC_TYPE_BASE64: xmlrpc_read_base64(local_env, callresult, &bytesize, &local_result->byteval);
                                         local_result->intval = (int)bytesize;
                                         break;
            }
            xmlrpc_DECREF(callresult);
        }

        xmlrpc_DECREF(pcall_array);
    }
    if (connerr == 0) {
        return 0;
    }
    else {
        return -1;
    }
}

int fldigi_get_tx_text(char * line) {

    int rc;
    xmlrpc_res result;
    xmlrpc_env env;
    line[0] = '\0';

    rc = fldigi_xmlrpc_query(&result, &env, "tx.get_data", "");
    if (rc != 0) {
        return 0;
    }
    else {
        if (result.intval > 0 && result.byteval != NULL) {
            line[0] = '\0';
            memcpy(line, result.byteval, result.intval);
            line[result.intval] = '\0';
            free((void *)result.byteval);
        }
        if (result.byteval != NULL) {
            free((void *)result.byteval);
        }
    }
    return 0;
}

int fldigi_get_rxtx_state() {
    int rc;
    xmlrpc_res result;
    xmlrpc_env env;

    rc = fldigi_xmlrpc_query(&result, &env, "main.get_trx_state", "");
    if (rc != 0) {
        return 0;
    }
    else {
        if (strcmp(result.stringval, "TX") == 0) {
            ptt = TX;
        }
        else {
            ptt = RX;
        }
        free((void *)result.stringval);
        if (result.byteval != NULL) {
            free((void *)result.byteval);
        }
    }
    return 0;
}

void exit_handler() {
    keeprun = 0;
}

int main() {
    int i, rc;
    char line[100];
    static int ptt_last = RX;

    line[0] = '\0';

    signal(SIGINT, exit_handler);

    fldigi_xmlrpc_init();
    rc = serial_init();
    if (rc != 0) {
        perror("Can't open port!");
        return -1;
    }

    ptt = RX;

    while(keeprun == 1) {
        rc = fldigi_get_rxtx_state();
        if (rc != 0) {
            perror("Fldigi error.");
            return -1;
        }
        if (ptt == TX) {
            if (ptt_last == RX) {
                rc = serial_write('[');
                if (rc <= 0) {
                    perror("Serial port write error.");
                    return -1;
                }
                ptt_last = TX;
            }
            rc = fldigi_get_tx_text(line);
            if (rc != 0) {
                perror("Fldigi error");
                return -1;
            }
            if (strlen(line) > 0) {
                for(i=0; i<strlen(line); i++) {
                    rc = serial_write(line[i]);
                    if (rc <= 0) {
                        perror("Serial port write error.");
                        return -1;
                    }
                }
            }
        }
        rc = fldigi_get_rxtx_state();
        if (rc != 0) {
            perror("Fldigi error.");
            return -1;
        }
        if (ptt == RX) {
            if (ptt_last == TX) {
                rc = serial_write(']');
                if (rc <= 0) {
                    perror("Serial port write error.");
                    return -1;
                }
                ptt_last = RX;
            }
        }
        usleep(1/45.45);
    }

    fldigi_xmlrpc_cleanup();
    serial_close();
    return 0;
}
