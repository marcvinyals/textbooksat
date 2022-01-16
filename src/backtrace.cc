#include <execinfo.h>
#include <cstdio>

void backtrace_handler(int signum) {
  int nptrs;
  void *buffer[100];
  char **strings;

  nptrs = backtrace(buffer, 100);
  printf("backtrace() returned %d addresses\n", nptrs);

  /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
     would produce similar output to the following: */

  strings = backtrace_symbols(buffer, nptrs);
  if (strings != NULL) {
    for (int j = 0; j < nptrs; j++)
      printf("%s\n", strings[j]);
  }
}
