//
// Created by yuwenyong on 17-9-12.
//

#include "net4cxx/common/utilities/errors.h"


NS_BEGIN

const char* Exception::what() const noexcept {
    if (_what.empty()) {
        _what += _file;
        _what += ':';
        _what += std::to_string(_line);
        _what += " in ";
        _what += _func;
        _what += ' ';
        _what += getTypeName();
        _what += "\n\t";
        _what += std::runtime_error::what();
        if (!_backtrace.empty()) {
            _what += "\nBacktrace:\n";
            _what += _backtrace;
        }
    }
    return _what.c_str();
}


NS_END