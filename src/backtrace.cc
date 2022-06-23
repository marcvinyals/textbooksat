#if defined BACKTRACE

#include <iostream>

#define BOOST_STACKTRACE_USE_BACKTRACE
#define BOOST_STACKTRACE_USE_ADDR2LINE
#include <boost/stacktrace.hpp>

void backtrace_handler(int signum) {
  std::cerr << boost::stacktrace::stacktrace();
}

#else

void backtrace_handler(int signum) {}

#endif
