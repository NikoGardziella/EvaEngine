#include "pch.h"
#include "Engine/Core/Assert.h"
#include "Engine/Core/Log.h"

#include <cstdarg>
#include <cstdio>


 #include <spdlog/fmt/fmt.h>



namespace Engine
{
    static const char* EE_Basename(const char* path)
    {
        const char* a = std::strrchr(path, '\\');
        const char* b = std::strrchr(path, '/');
        const char* last = (a && b) ? (a > b ? a : b) : (a ? a : b);
        return last ? last + 1 : path;
    }
    void AssertFailNoMsg(AssertChannel channel,
        const char* expr,
        const char* file,
        int line)
    {
        const char* f = EE_Basename(file);
        if (channel == AssertChannel::Core)
            EE_CORE_ERROR("Assertion failed: '{}' at {}:{}", expr, f, line);
        else
            EE_ERROR("Assertion failed: '{}' at {}:{}", expr, f, line);
    }

    void AssertFailFmt(AssertChannel channel,
        const char* expr,
        const char* file,
        int line,
        const char* fmtStr, ...)
    {
     
        const char* f = EE_Basename(file);

        if (channel == AssertChannel::Core)
            EE_CORE_ERROR("Assertion failed: '{}' | {} at {}:{}", expr, fmtStr, f, line);
        else
            EE_ERROR("Assertion failed: '{}' | {} at {}:{}", expr, fmtStr, f, line);
    }
}
