#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#define LOG_HEADER_IMPLEMENTATION
#include "../helpers/log.h"

#define ASSERT(cond, msg) do { \
	if (!(cond)) { \
		fprintf(stderr, "ASSERT FAILED: %s (line %d): %s\n", __FILE__, __LINE__, msg); \
		exit(1); \
	} \
} while(0)

#include <unistd.h>

typedef struct Capture {
	FILE *original;
	FILE *pipeWrite;
	FILE *pipeRead;
	char *buffer;
	size_t size;
	int isStderr;
} Capture;

static int make_pipe(int fds[2]) { return pipe(fds); }

static void start_capture(Capture *cap, int isStderr) {
	memset(cap, 0, sizeof(*cap));
	cap->isStderr = isStderr;
	int fds[2];
	if (make_pipe(fds) != 0) { fprintf(stderr, "pipe failed\n"); exit(1); }
	FILE *readFile = fdopen(fds[0], "r");
	FILE *writeFile = fdopen(fds[1], "w");
	if (!readFile || !writeFile) { fprintf(stderr, "fdopen failed\n"); exit(1); }
	cap->pipeRead = readFile;
	cap->pipeWrite = writeFile;
	if (isStderr) { fflush(stderr); cap->original = stderr; stderr = writeFile; }
	else { fflush(stdout); cap->original = stdout; stdout = writeFile; }
}

static void finish_capture(Capture *cap) {
	fflush(cap->pipeWrite);
	if (cap->isStderr) stderr = cap->original; else stdout = cap->original;
	fclose(cap->pipeWrite); cap->pipeWrite = NULL;
	size_t alloc = 1024; cap->buffer = (char*)malloc(alloc); size_t len = 0;
	for (;;) {
		if (len + 512 >= alloc) { alloc *= 2; cap->buffer = (char*)realloc(cap->buffer, alloc); }
		size_t n = fread(cap->buffer + len, 1, 512, cap->pipeRead);
		len += n; if (n < 512) break;
	}
	cap->buffer[len] = '\0'; cap->size = len; fclose(cap->pipeRead); cap->pipeRead = NULL;
}

static void free_capture(Capture *cap) { free(cap->buffer); cap->buffer = NULL; }
static int contains(const char *h, const char *n) { return strstr(h, n) != NULL; }

static inline void flush_logs(void) { fflush(stdout); fflush(stderr); }

static void test_log_levels(void) {
	log_set_level(LOG_LEVEL_DEBUG);
	Capture out; start_capture(&out, 0);
	Capture err; start_capture(&err, 1);

	char tmp[128];
	snprintf(tmp, sizeof(tmp), "Message %d", 1); log_error(tmp);
	snprintf(tmp, sizeof(tmp), "Debug %d", 2); log_debug(tmp);
	snprintf(tmp, sizeof(tmp), "Warn %d", 3); log_warn(tmp);
	snprintf(tmp, sizeof(tmp), "Error %d", 4); log_error(tmp);
	flush_logs();

	finish_capture(&out); finish_capture(&err);

	ASSERT(contains(out.buffer, "Debug 2"), "DEBUG missing at debug level");
	ASSERT(contains(out.buffer, "Warn 3"), "WARN missing at debug level");
	ASSERT(contains(err.buffer, "Message 1"), "MSG (error) missing at debug level");
	ASSERT(contains(err.buffer, "Error 4"), "ERROR missing at debug level");

	free_capture(&out); free_capture(&err);

	log_set_level(LOG_LEVEL_WARN);
	start_capture(&out, 0); start_capture(&err, 1);
	snprintf(tmp, sizeof(tmp), "Message %d", 10); log_error(tmp);
	snprintf(tmp, sizeof(tmp), "Debug %d", 20); log_debug(tmp);
	snprintf(tmp, sizeof(tmp), "Warn %d", 30); log_warn(tmp);
	snprintf(tmp, sizeof(tmp), "Error %d", 40); log_error(tmp);
	flush_logs();
	finish_capture(&out); finish_capture(&err);

	ASSERT(!contains(out.buffer, "Debug 20"), "DEBUG should be suppressed at WARN level");
	ASSERT(contains(out.buffer, "Warn 30"), "WARN should appear at WARN level");
	ASSERT(contains(err.buffer, "Error 40"), "ERROR should appear at WARN level");

	free_capture(&out); free_capture(&err);

	log_set_level(LOG_LEVEL_ERROR);
	start_capture(&out, 0); start_capture(&err, 1);
	snprintf(tmp, sizeof(tmp), "Message %d", 100); log_error(tmp);
	snprintf(tmp, sizeof(tmp), "Debug %d", 200); log_debug(tmp);
	snprintf(tmp, sizeof(tmp), "Warn %d", 300); log_warn(tmp);
	snprintf(tmp, sizeof(tmp), "Error %d", 400); log_error(tmp);
	flush_logs();
	finish_capture(&out); finish_capture(&err);

	ASSERT(!contains(out.buffer, "Debug 200"), "DEBUG suppressed at ERROR level");
	ASSERT(!contains(out.buffer, "Warn 300"), "WARN suppressed at ERROR level");
	ASSERT(contains(err.buffer, "Error 400"), "ERROR appears at ERROR level");
	ASSERT(contains(err.buffer, "Message 100"), "MSG (error) appears at ERROR level");

	free_capture(&out); free_capture(&err);

	log_set_level(LOG_LEVEL_OFF);
	start_capture(&out, 0); start_capture(&err, 1);
	snprintf(tmp, sizeof(tmp), "Message %d", 500); log_error(tmp);
	snprintf(tmp, sizeof(tmp), "Debug %d", 600); log_debug(tmp);
	snprintf(tmp, sizeof(tmp), "Warn %d", 700); log_warn(tmp);
	snprintf(tmp, sizeof(tmp), "Error %d", 800); log_error(tmp);
	flush_logs();
	finish_capture(&out); finish_capture(&err);

	ASSERT(out.size == 0, "STDOUT should be empty at OFF level");
	ASSERT(err.size == 0, "STDERR should be empty at OFF level");

	free_capture(&out); free_capture(&err);
}

static void test_large_msg(void) {
	log_set_level(LOG_LEVEL_DEBUG);
	size_t bigSize = 1500;
	char *big = (char*)malloc(bigSize + 1);
	ASSERT(big != NULL, "Allocation failed");
	memset(big, 'A', bigSize); big[bigSize] = '\0';

	Capture out; start_capture(&out, 0);
	log_debug(big);
	flush_logs();
	finish_capture(&out);

	size_t countA = 0; for (const char *p = out.buffer; p && *p; ++p) if (*p == 'A') countA++;
	ASSERT(countA == bigSize, "Large message truncated in output");

	free(big); free_capture(&out);
}

static void test_unsafe_format(void) {
	log_set_level(LOG_LEVEL_DEBUG);
	Capture out; start_capture(&out, 0);
	Capture err; start_capture(&err, 1);

	log_error("Blocked %n format test");
	flush_logs();
	finish_capture(&out); finish_capture(&err);

	ASSERT(!contains(out.buffer, "Blocked"), "Unsafe message should not appear on stdout");
	ASSERT(contains(err.buffer, "%n"), "Expected literal %n in stderr output");

	free_capture(&out); free_capture(&err);
}

int main(void) {
	test_log_levels();
	test_large_msg();
	test_unsafe_format();
	printf("All log tests passed.\n");
	return 0;
}