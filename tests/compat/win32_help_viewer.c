/* Manual check of the packaged guide through the real desktop URL launcher. */
#include <windows.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    UINT uCommand = HELP_CONTEXT;
    ULONG_PTR idh = 4501;
    unsigned long long ullContext;
    char *pszEnd;

    if (argc == 2 && !strcmp(argv[1], "--contents")) {
        uCommand = HELP_INDEX;
        idh = 0;
    } else if (argc == 2 && argv[1][0] >= '0' && argv[1][0] <= '9') {
        errno = 0;
        ullContext = strtoull(argv[1], &pszEnd, 10);
        if (errno || *pszEnd || ullContext > UINT32_MAX) return 2;
        idh = (ULONG_PTR)ullContext;
    } else if (argc != 1) {
        fprintf(stderr, "Usage: %s [--contents|context-id]\n", argv[0]);
        return 2;
    }
    if (!WinHelpA(NULL, "stars!.hlp", uCommand, idh)) {
        fprintf(stderr, "Help launch failed: error %lu\n", (unsigned long)GetLastError());
        return 1;
    }
    puts("Desktop accepted the guide launch; verify the page in your browser.");
    return 0;
}
