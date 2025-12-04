//=================================================================================================
// Copyright (C) 2023-2024 HEPHAESTUS Contributors
// MIT License
//=================================================================================================

#include "hephaestus/utils/stack_trace.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include <bits/signum-generic.h>
#include <bits/types/siginfo_t.h>
#include <cxxabi.h>

#ifndef __WIN32__
#include <execinfo.h>
#endif

namespace heph::utils {

#ifndef __WIN32__

// NOLINTBEGIN(misc-include-cleaner) // Lots of false positives
class StackTrace::Impl {
public:
  Impl();

  static void print(std::ostream& os);

private:
  static constexpr size_t MAX_FRAMES = 64;
  [[noreturn]] static void abort(int signum, siginfo_t* si, void* unused);
};

StackTrace::Impl::Impl() {
  struct sigaction sa{};
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

void StackTrace::Impl::print(std::ostream& os) {
  /// reference: https://oroboro.com/stack-trace-on-crash/

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,cert-err33-c)
  os << "stack trace:\n";
  void* addrlist[MAX_FRAMES];  // NOLINT(cppcoreguidelines-avoid-c-arrays)

  // retrieve current stack addresses
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  auto addrlen = backtrace(addrlist, MAX_FRAMES);
  if (addrlen == 0) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg,cert-err33-c)
    os << "  \n";
  }

  // resolve addresses into strings containing "filename(function+address)",
  // Actually it will be ## program address function + offset
  // this array must be free()-ed
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
  char** symbol_list = backtrace_symbols(addrlist, addrlen);

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

      int status = 0;           // This will be set by __cxa_demangle
      size_t fun_name_len = 0;  // This will be set by __cxa_demangle
      // Nullptr as output buffer will allocate the memory needed for the demangled name on the heap. We need
      // to free manually afterwards.
      // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
      char* ret = abi::__cxa_demangle(begin_name, nullptr, &fun_name_len, &status);
      std::string_view fname = begin_name;
      if (status == 0) {
        fname = std::string_view{ ret, fun_name_len };
      }

      // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,cppcoreguidelines-pro-bounds-pointer-arithmetic)
      if (begin_offset != nullptr) {
        os << "  " << std::left << std::setw(30) << symbol_list[i] << " ( " << std::setw(40) << fname
           << "  + " << std::setw(6) << begin_offset << ") " << end_offset << "\n";
      } else {
        os << "  " << std::left << std::setw(30) << symbol_list[i] << " ( " << std::setw(40) << fname
           << "    " << std::setw(6) << "" << ") " << end_offset << "\n";
      }
      // NOLINTNEXTLINE(hicpp-no-malloc,cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
      free(ret);
    } else {
      // couldn't parse the line? print the whole line.
      os << "  " << std::setw(40) << symbol_list[i] << "\n";
      // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
  }

  // NOLINTNEXTLINE(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory,hicpp-no-malloc,bugprone-multi-level-implicit-pointer-conversion)
  free(static_cast<void*>(symbol_list));
}

void StackTrace::Impl::abort(int signum, siginfo_t* signal_info, void* unused) {
  (void)signal_info;
  (void)unused;

  std::cerr << "Caught signal " << strsignal(signum) << "\n";
  StackTrace::Impl::print(std::cerr);
  exit(signum);
}

#else

#warning "StackTrace is not implemented on Windows"

class StackTrace::Impl {
public:
  Impl() = default;

  static void print(std::ostream&) {
  }
};

#endif

// ----------------------------------------------------------------------------------------------------------

StackTrace::StackTrace() : impl_{ std::make_unique<Impl>() } {
}

StackTrace::~StackTrace() = default;

auto StackTrace::print() -> std::string {
  std::stringstream os;
  Impl::print(os);
  return os.str();
}

// NOLINTEND(misc-include-cleaner)
}  // namespace heph::utils
