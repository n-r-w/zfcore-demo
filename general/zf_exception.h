#ifndef ZF_EXCEPTION_H
#define ZF_EXCEPTION_H

#include <exception>
#include <stdexcept>
#include "zf_error.h"

namespace zf
{
//! Для генерации исключений при Utils::isFatalHaltBlocked
class ZCORESHARED_EXPORT FatalException : public std::runtime_error
{
public:
    FatalException(const Error& error);

    Error error() const;

    void raise [[noreturn]] () const { throw *this; }
    FatalException* clone() const { return new FatalException(*this); }

private:
    Error _error;
};

//! Служебный класс для автоматического отключения blockFatalHalt
class ZCORESHARED_EXPORT FatalExceptionHelper
{
public:
    FatalExceptionHelper();
    ~FatalExceptionHelper();

    void reset() { _initialized = true; }

private:
    bool _initialized = false;
};

} // namespace zf

#endif // ZF_EXCEPTION_H
