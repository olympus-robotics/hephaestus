//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#include "hephaestus/utils/stack_trace.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>

#include <cxxabi.h>
#include <execinfo.h>

namespace heph::utils {
// NOLINTBEGIN(misc-include-cleaner) // Lots of false positives
class StackTrace::Impl {
public:
  Impl();

private:
  static constexpr size_t MAX_FRAMES = 64;
  static constexpr size_t MAX_FUNC_NAME_LEN = 1024;
  static void print();
  [[noreturn]] static void abort(int signum, siginfo_t* si, void* unused);
};

StackTrace::Impl::Impl() {
  struct sigaction sa {};
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = StackTrace::Impl::abort;  // NOLINT(cppcoreguidelines-pro-type-union-access)
  sigemptyset(&sa.sa_mask);

  sigaction(SIGABRT, &sa, nullptr);
  sigaction(SIGSEGV, &sa, nullptr);
  sigaction(SIGBUS, &sa, nullptr);
  sigaction(SIGILL, &sa, nullptr);
  sigaction(SIGFPE, &sa, nullptr);
  sigaction(SIGPIPE, &sa, nullptr);
}

void StackTrace::Impl::print() {
  /// reference: https://oroboro.com/stack-trace-on-crash/

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,cert-err33-c)
  fprintf(stderr, "stack trace:\n");
  void* addrlist[MAX_FRAMES];  // NOLINT(cppcoreguidelines-avoid-c-arrays)

  // retrieve current stack addresses
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  auto addrlen = backtrace(addrlist, MAX_FRAMES);
  if (addrlen == 0) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,cert-err33-c)
    fprintf(stderr, "  \n");
  }

  // resolve addresses into strings containing "filename(function+address)",
  // Actually it will be ## program address function + offset
  // this array must be free()-ed
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  char** symbol_list = backtrace_symbols(addrlist, addrlen);

  char funcname[MAX_FUNC_NAME_LEN];  // NOLINT(cppcoreguidelines-avoid-c-arrays)

  // iterate over the returned symbol lines. skip the first, it is the
  // address of this function.
  for (auto i = 3; i < addrlen; i++) {
    char* begin_name = nullptr;
    char* begin_offset = nullptr;
    char* end_offset = nullptr;

    // find parentheses and +address offset surrounding the mangled name
    // ./module(function+0x15c) [0x8048a6d]
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (char* p = symbol_list[i]; *p != 0; ++p) {
      if (*p == '(') {
        begin_name = p;
      } else if (*p == '+') {
        begin_offset = p;
      } else if (*p == ')' && ((nullptr != begin_offset) || (nullptr != begin_name))) {
        end_offset = p;
      }
    }

    if ((begin_name != nullptr) && (end_offset != nullptr) && (begin_name < end_offset)) {
      *begin_name++ = '\0';  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      *end_offset++ = '\0';  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      if (begin_offset != nullptr) {
        *begin_offset++ = '\0';  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
      }

      // mangled name is now in [begin_name, begin_offset) and caller
      // offset in [begin_offset, end_offset). now apply
      // __cxa_demangle():

      int status = 0;
      size_t fun_name_len = MAX_FUNC_NAME_LEN;
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      char* ret = abi::__cxa_demangle(begin_name, funcname, &fun_name_len, &status);
      char* fname = begin_name;
      if (status == 0) {
        fname = ret;
      }

      if (begin_offset != nullptr) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,cppcoreguidelines-pro-bounds-pointer-arithmetic,cert-err33-c)
        fprintf(stderr, "  %-30s ( %-40s  + %-6s) %s\n", symbol_list[i], fname, begin_offset, end_offset);
      } else {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,cppcoreguidelines-pro-bounds-pointer-arithmetic,cert-err33-c)
        fprintf(stderr, "  %-30s ( %-40s    %-6s) %s\n", symbol_list[i], fname, "", end_offset);
      }
    } else {
      // couldn't parse the line? print the whole line.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,cppcoreguidelines-pro-bounds-pointer-arithmetic,cert-err33-c)
      fprintf(stderr, "  %-40s\n", symbol_list[i]);
    }
  }

  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory,hicpp-no-malloc,bugprone-multi-level-implicit-pointer-conversion)
  free(static_cast<void*>(symbol_list));
}

void StackTrace::Impl::abort(int signum, siginfo_t* signal_info, void* unused) {
  (void)signal_info;
  (void)unused;

  std::cerr << "Caught signal " << strsignal(signum) << "\n";
  StackTrace::Impl::print();
  exit(signum);
}

// ----------------------------------------------------------------------------------------------------------

StackTrace::StackTrace() : impl_{ std::make_unique<Impl>() } {
}

StackTrace::~StackTrace() = default;

// NOLINTEND(misc-include-cleaner)
}  // namespace heph::utils
