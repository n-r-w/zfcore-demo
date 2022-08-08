#include "zf_exception.h"
#include "zf_utils.h"
#include <QDebug>

namespace zf
{
FatalExceptionHelper::FatalExceptionHelper()
{
}

FatalExceptionHelper::~FatalExceptionHelper()
{
    if (_initialized)
        Utils::unBlockFatalHalt();
}

FatalException::FatalException(const Error& error)
    : std::runtime_error(error.fullText().toLocal8Bit().data())
    , _error(error)
{    
}

Error FatalException::error() const
{
    return _error;
}

} // namespace zf
