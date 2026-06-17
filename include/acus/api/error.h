// Acus - A C++ library for generating Brainfuck programs.
// Copyright (C) 2026 Joren Heit
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <exception>
#include <sstream>
#include <string>
#include <utility>
#include "acus/api/error_codes.h"

namespace acus::error {


  struct Error: std::exception {

    ErrorCode errorCode;
    std::string filename;
    int line;
    int column;
    std::string msg;

    Error(ErrorCode errorCode, std::string fname, int ln, int col, std::string message):
      errorCode(errorCode),
      filename(std::move(fname)),
      line(ln),
      column(col),
      msg(std::move(message))
    {}

    ErrorCode code() const noexcept {
      return errorCode;
    }

    Error &relocate(std::string fname, int ln, int col) noexcept {
      filename = std::move(fname);
      line = ln;
      column = col;
      return *this;
    }
    
    virtual char const *what() const noexcept override {
      static std::string str;
      str.clear();
      str = filename + ":" + std::to_string(line) + ":" + std::to_string(column) + ": " + msg;
      return str.c_str();
    }
  };

  template <typename ... Args>
  void throw_if(bool condition, ErrorCode errorCode,
                std::string const &filename, int line, int column,
                Args ... args) {
    if (not condition) return;
    std::ostringstream oss;
    (oss << ... << args);
    throw Error(errorCode, filename, line, column, oss.str());
    std::unreachable();
  }  

} // namespace acus::error
