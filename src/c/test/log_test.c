#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "../helpers/log.h"

/* Simple assert macro */
#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        fprintf(stderr, "ASSERT FAILED: %s (line %d): %s\n", __FILE__, __LINE__, msg); \
        exit(1); \
    } \
} while(0)

/* Capture structure for stdout/stderr */
typedef struct Capture {
    FILE *original;
    FILE *pipeWrite; /* writing end for redirect */
    FILE *pipeRead;  /* reading end to collect */
    char *buffer;    /* collected output */
    size_t size;
    int isStderr;    /* flag for stderr */
} Capture;

#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#define pipe _pipe
#else
#include <unistd.h>
#endif

static int make_pipe(int fds[2]) {
#if defined(_WIN32)
    return _pipe(fds, 4096, _O_BINARY);
#else
    return pipe(fds);
#endif
}

static void start_capture(Capture *cap, int isStderr) {
    memset(cap, 0, sizeof(*cap));
    cap->isStderr = isStderr;
    int fds[2];
    if (make_pipe(fds) != 0) {
        fprintf(stderr, "Failed to create pipe: %s\n", strerror(errno));
        exit(1);
    }
#if defined(_WIN32)
    int readFd = fds[0];
    int writeFd = fds[1];
    FILE *readFile = _fdopen(readFd, "rb");
    FILE *writeFile = _fdopen(writeFd, "wb");
#else
    FILE *readFile = fdopen(fds[0], "r");
    FILE *writeFile = fdopen(fds[1], "w");
#endif
    if (!readFile || !writeFile) {
        fprintf(stderr, "fdopen failure\n");
        exit(1);
    }
    cap->pipeRead = readFile;
    cap->pipeWrite = writeFile;
    if (isStderr) {
        fflush(stderr);
        cap->original = stderr;
        stderr = writeFile;
    } else {
        fflush(stdout);
        cap->original = stdout;
        stdout = writeFile;
    }
}

static void finish_capture(Capture *cap) {
    fflush(cap->pipeWrite);
    /* Restore original */
    if (cap->isStderr) {
        stderr = cap->original;
    } else {
        stdout = cap->original;
    }
    /* Close writing end to signal EOF */
    fclose(cap->pipeWrite);
    cap->pipeWrite = NULL;
    /* Read all */
    size_t alloc = 1024;
    cap->buffer = (char*)malloc(alloc);
    size_t len = 0;
    for (;;) {
        if (len + 512 >= alloc) {
            alloc *= 2;
            cap->buffer = (char*)realloc(cap->buffer, alloc);
        }
        size_t n = fread(cap->buffer + len, 1, 512, cap->pipeRead);
        len += n;
        if (n < 512) {
            break; /* EOF */
        }
    }
    cap->buffer[len] = '\0';
    cap->size = len;
    fclose(cap->pipeRead);
    cap->pipeRead = NULL;
}

static void free_capture(Capture *cap) {
    free(cap->buffer);
    cap->buffer = NULL;
}

static int contains(const char *haystack, const char *needle) {
    return strstr(haystack, needle) != NULL;
}

static void test_log_levels() {
    /* At DEBUG level, all outputs should appear */
    log_set_level(LOG_LEVEL_DEBUG);

    Capture capOut; start_capture(&capOut, 0);
    Capture capErr; start_capture(&capErr, 1);

    LOG_MSG("Message %d", 1);
    LOG_DEBUG("Debug %d", 2);
    LOG_WARN("Warn %d", 3);
    LOG_ERROR("Error %d", 4);
    log_flush();

    finish_capture(&capOut);
    finish_capture(&capErr);

    ASSERT(contains(capOut.buffer, "Message 1"), "MSG missing at debug level");
    ASSERT(contains(capOut.buffer, "Debug 2"), "DEBUG missing at debug level");
    ASSERT(contains(capErr.buffer, "WARN:"), "WARN missing at debug level");
    ASSERT(contains(capErr.buffer, "ERROR:"), "ERROR missing at debug level");

    free_capture(&capOut); free_capture(&capErr);

    /* At WARN level, debug suppressed but warn/error visible */
    log_set_level(LOG_LEVEL_WARN);
    start_capture(&capOut, 0); start_capture(&capErr, 1);

    LOG_MSG("Message %d", 10); /* uses error threshold */
    LOG_DEBUG("Debug %d", 20); /* should NOT appear */
    LOG_WARN("Warn %d", 30);   /* appear */
    LOG_ERROR("Error %d", 40); /* appear */
    log_flush();

    finish_capture(&capOut); finish_capture(&capErr);

    ASSERT(!contains(capOut.buffer, "Debug 20"), "DEBUG should be suppressed at WARN level");
    ASSERT(contains(capErr.buffer, "Warn 30"), "WARN should appear at WARN level");
    ASSERT(contains(capErr.buffer, "Error 40"), "ERROR should appear at WARN level");

    free_capture(&capOut); free_capture(&capErr);

    /* At ERROR level, only msg/error appear (warn should appear? Original logic prints warn when level <= WARN (3). ERROR is 4 so warn should be suppressed) */
    log_set_level(LOG_LEVEL_ERROR);
    start_capture(&capOut, 0); start_capture(&capErr, 1);

    LOG_MSG("Message %d", 100);
    LOG_DEBUG("Debug %d", 200);
    LOG_WARN("Warn %d", 300); /* should NOT appear */
    LOG_ERROR("Error %d", 400);
    log_flush();

    finish_capture(&capOut); finish_capture(&capErr);

    ASSERT(!contains(capOut.buffer, "Debug 200"), "DEBUG suppressed at ERROR level");
    ASSERT(!contains(capErr.buffer, "Warn 300"), "WARN suppressed at ERROR level");
    ASSERT(contains(capErr.buffer, "Error 400"), "ERROR appears at ERROR level");
    ASSERT(contains(capOut.buffer, "Message 100"), "MSG appears at ERROR level");

    free_capture(&capOut); free_capture(&capErr);

    /* At OFF level, nothing should appear */
    log_set_level(LOG_LEVEL_OFF);
    start_capture(&capOut, 0); start_capture(&capErr, 1);

    LOG_MSG("Message %d", 500);
    LOG_DEBUG("Debug %d", 600);
    LOG_WARN("Warn %d", 700);
    LOG_ERROR("Error %d", 800);
    log_flush();

    finish_capture(&capOut); finish_capture(&capErr);

    ASSERT(capOut.size == 0, "STDOUT should be empty at OFF level");
    ASSERT(capErr.size == 0, "STDERR should be empty at OFF level");

    free_capture(&capOut); free_capture(&capErr);
}


static char sink_last_prefix[64];
static char sink_last_msg[4096];
static int sink_calls = 0;
static size_t sink_countA = 0;
static void custom_sink(const char *prefix, const char *msg) {
    strncpy(sink_last_prefix, prefix ? prefix : "", sizeof(sink_last_prefix)-1);
    sink_last_prefix[sizeof(sink_last_prefix)-1] = '\0';
    strncpy(sink_last_msg, msg ? msg : "", sizeof(sink_last_msg)-1);
    sink_last_msg[sizeof(sink_last_msg)-1] = '\0';
    sink_countA = 0; for (const char *cp = msg; cp && *cp; ++cp) if (*cp == 'A') sink_countA++;
    sink_calls++;
}

static void test_custom_sink_and_large_msg() {
    log_set_level(LOG_LEVEL_DEBUG);
    log_set_sink(custom_sink);
    sink_calls = 0;
    /* Large message > stack buffer (assumes default 1024) */
    size_t bigSize = 1500;
    char *big = (char*)malloc(bigSize + 1);
    ASSERT(big != NULL, "Allocation failed for big test message");
    memset(big, 'A', bigSize);
    big[bigSize] = '\0';

    LOG_DEBUG("%s", big);
    ASSERT(sink_calls == 1, "Custom sink should be called exactly once for large message");
    ASSERT(strstr(sink_last_prefix, "DEBUG") != NULL, "Prefix should contain DEBUG");
    /* Ensure large content passed fully: count 'A' occurrences */
    ASSERT(sink_countA == bigSize, "Large message truncated in custom sink");

    free(big);
    log_set_sink(NULL); /* restore */
}

static void test_unsafe_format() {
    log_set_level(LOG_LEVEL_DEBUG);
    Capture capOut; start_capture(&capOut, 0);
    Capture capErr; start_capture(&capErr, 1);

    /* This format contains %n and should be blocked */
    LOG_ERROR("Blocked %n format test");
    log_flush();

    finish_capture(&capOut);
    finish_capture(&capErr);

    ASSERT(!contains(capOut.buffer, "Blocked"), "Unsafe message should not appear on stdout");
    ASSERT(contains(capErr.buffer, "Unsafe format string"), "Blocking notice missing on stderr");

    free_capture(&capOut); free_capture(&capErr);
}

int main(void) {
    test_log_levels();
    test_custom_sink_and_large_msg();
    test_unsafe_format();
    printf("All log tests passed.\n");
    return 0;
}
