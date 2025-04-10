#include <stdarg.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <execinfo.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "iron.h"

void fe_runtime_crash(const char* error, ...) {
    fflush(stdout);

    printf("\niron runtime crash: ");
    fflush(stdout);

    va_list args;
    va_start(args, error);
    vfprintf(stderr, error, args);
    va_end(args);
    printf("\n");

#ifndef __WIN32__
    void* array[256]; // hold 256 stack traces
    char** strings;
    int size;

    size = backtrace(array, 256);
    strings = backtrace_symbols(array, size);

    if (strings != NULL) {
        printf("obtained %d stack frames\n", size);
        for (int i = 0; i < size; i++) {
            char* output_str = strings[i];
            // if (output_str.raw[0] == '.') {
            //     // we need to trim
            //     for (; output_str.raw[0] != '('; output_str.raw++)
            //         ;
            //     output_str.raw++;
            //     int close_bracket = 0;
            //     for (; output_str.raw[close_bracket] != ')'; close_bracket++)
            //         ;
            //     output_str.len = close_bracket;
            // }
            printf("| %s\n", output_str);
        }
    }
#else
    // figure out windows stack traces!
#endif
    exit(-1);
}

#ifndef __WIN32__
static void signal_handler(int sig, siginfo_t* info, void* ucontext) {
    // ucontext = ucontext;
    switch (sig) {
    case SIGSEGV:
        fe_runtime_crash("segfault at addr 0x%lx\n", (long)info->si_addr);
        break;
    case SIGINT:
        printf("debug interrupt caught");
        break;
    case SIGFPE:
        fe_runtime_crash("fatal arithmetic exception");
        break;
    default:
        fe_runtime_crash("unhandled signal %s caught\n", strsignal(sig));
        break;
    }
}

void fe_init_signal_handler() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_handler;
    sigfillset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        printf("sigaction to catch SIGSEGV failed.");
        exit(-1);
    }
    if (sigaction(SIGFPE, &sa, NULL) == -1) {
        printf("sigaction to catch SIGFPE failed.");
        exit(-1);
    }
}
#endif
