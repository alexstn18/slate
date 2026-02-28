#pragma once
#include <cassert>

#define FMT_HEADER_ONLY
#include <fmt/include/fmt/core.h>

static constexpr auto MAGENTA = "\033[35m";
static constexpr auto GREEN = "\033[32m";
static constexpr auto RED = "\033[31m";
static constexpr auto RESET = "\033[0m";

namespace slate::log
{
    template<typename... Args>
    inline void Info(const fmt::format_string<Args...>& fmt, const Args&... args)
    {
        printf( "[%sinfo%s] ", GREEN, RESET );
        fmt::print( fmt::runtime( fmt ), args... );
        printf( "\n" );
    }

    template<typename... Args>
    inline void Warn(const fmt::format_string<Args...>& fmt, const Args&... args)
    {
        printf( "[%swarn%s] ", MAGENTA, RESET );
        fmt::print( fmt::runtime( fmt ), args... );
        printf( "\n" );
    }

    template<typename... Args>
    inline void Error(const fmt::format_string<Args...>& fmt, const Args&... args)
    {
        printf( "[%serror%s] ", RED, RESET );
        fmt::print( fmt::runtime( fmt ), args... );
        printf( "\n" );
    }

    template<typename... Args>
    inline void Critical(const fmt::format_string<Args...>& fmt, const Args&... args)
    {
        printf( "[%scritical%s] ", RED, RESET );
        fmt::print( fmt::runtime( fmt ), args... );
        printf( "\n" );
        assert( false );
    }

    template<typename... Args>
    inline void Assert(bool expression, const fmt::format_string<Args...>& fmt, const Args&... args)
    {
#if defined(_DEBUG)
        if ( !expression ) {
            printf( "[%sassert%s] ", RED, RESET );
            fmt::print( fmt::runtime( fmt ), args... );
            printf( "\n" );
            assert( false );
        }
#endif
    }


    inline void ThrowIfFailed(HRESULT hr)
    {
        if ( FAILED( hr ) ) {
            char* hrCstr = nullptr;
            FormatMessageA( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                nullptr,
                hr,
                0,
                ( LPSTR )&hrCstr,
                0,
                nullptr
            );
            Critical( "HRESULT Error: {0}", hrCstr );
        }
    }
}