#include "startup_error.h"
#include "string.h"
#include "terminal.h"

#define STARTUP_ERROR_BUFFER_SIZE 512

static char startupErrors[STARTUP_ERROR_BUFFER_SIZE] = "";

void logStartupError(char* description) {
    if (!description) return;
    if (strlen(startupErrors) + strlen(description) > STARTUP_ERROR_BUFFER_SIZE) {
        strcpy(startupErrors, "FREWOZET SYSTEM ERROR: Too many errors to display.\n    Startup error log buffer overflow.");
        return;
    }
    strcat(startupErrors, description);
    strcat(startupErrors, "\n");
}

char* getStartupErrors(void) {
    if (strlen(startupErrors) > 0) {
        return startupErrors;
    }
    return 0;
}