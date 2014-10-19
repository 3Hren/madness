#include <array>
#include <string>

#ifdef BLACKHOLE_HAS_STATIC_PRINTF_OPERATOR_PUSH_SUPPORT
#   include <blackhole/detail/traits/supports/stream_push.hpp>
#endif

int main(int, char**) {
    return 0;
}

template<std::size_t N>
constexpr
static
bool
match_whatever(const char(&fmt)[N], const std::size_t n);

template<std::size_t N, class T, typename... Args>
constexpr
static
bool
match_whatever(const char(&fmt)[N], const std::size_t n, const T& arg, const Args&... args);

template<class> struct supported {
    template<std::size_t N, class T, class... Args>
    constexpr
    static
    bool
    match_type(const char(&)[N], const std::size_t, std::size_t, const T&, const Args&...) {
        return false; // Unregistered.
    }
};

template<class R>
struct supported_base {
    template<std::size_t N, class T, class... Args>
    constexpr
    static
    bool
    match_type(const char(&fmt)[N], std::size_t n, size_t e, const T& arg, const Args&... args) {
        return
#ifdef BLACKHOLE_HAS_STATIC_PRINTF_OPERATOR_PUSH_SUPPORT
            n < N && fmt[n] == 's' && blackhole::traits::supports::stream_push<T>::value?
                match_whatever(fmt, n + 1, args...):
#endif
            e < sizeof(R::expected) - 1 && n < N ?
                fmt[n] == R::expected[e] && match_type(fmt, n + 1, e + 1, arg, args...):
                n < N ?
                    match_whatever(fmt, n, args...):  // Some format characters left, but there is no more characters of type to be matched.
                    false; // No more characters left, but there is arguments.
    }
};

template<>
struct supported<int> {
    template<std::size_t N, class T, class... Args>
    constexpr
    static
    bool
    match_type(const char(&fmt)[N], std::size_t n, size_t, const T&, const Args&... args) {
        return n < N?
#ifdef BLACKHOLE_HAS_STATIC_PRINTF_OPERATOR_PUSH_SUPPORT
            fmt[n] == 's' && blackhole::traits::supports::stream_push<T>::value?
                match_whatever(fmt, n + 1, args...):
#endif
                (fmt[n] == 'd' || fmt[n] == 'i' || fmt[n] == 'o' || fmt[n] == 'x' || fmt[n] == 'X')?
                    match_whatever(fmt, n + 1, args...):
                    false:
            false; // No more characters left, but there is arguments.
    }
};

template<>
struct supported<char> : public supported_base<supported<char>> {
    constexpr static char expected[sizeof("c")] = { "c" };
};

template<>
struct supported<unsigned> : public supported_base<supported<unsigned>> {
    constexpr static char expected[sizeof("u")] = { "u" };
};

template<>
struct supported<long> : public supported_base<supported<long>> {
    constexpr static char expected[sizeof("ld")] = { "ld" };
};

template<>
struct supported<double> {
    template<std::size_t N, class T, class... Args>
    constexpr
    static
    bool
    match_type(const char(&fmt)[N], std::size_t n, size_t, const T&, const Args&... args) {
        return n < N?
#ifdef BLACKHOLE_HAS_STATIC_PRINTF_OPERATOR_PUSH_SUPPORT
            fmt[n] == 's' && blackhole::traits::supports::stream_push<T>::value?
                match_whatever(fmt, n + 1, args...):
#endif
                (fmt[n] == 'f' || fmt[n] == 'F' || fmt[n] == 'e' || fmt[n] == 'E' ||
                 fmt[n] == 'g' || fmt[n] == 'G' || fmt[n] == 'a' || fmt[n] == 'A')?
                    match_whatever(fmt, n + 1, args...):
                    false:
            false; // No more characters left, but there is arguments.
    }
};

template<std::size_t N>
struct supported<char[N]> : public supported_base<supported<char[N]>> {
    constexpr static char expected[sizeof("s")] = { "s" };
};

template<class Char, class Traits, class Allocator>
struct supported<std::basic_string<Char, Traits, Allocator>> :
    public supported_base<supported<std::basic_string<Char, Traits, Allocator>>>
{
    constexpr static char expected[2] = { "s" };
};

template<std::size_t N>
constexpr
static
bool
match_type(const char(&)[N], const std::size_t) {
    return false;
}

template<std::size_t N, class T, typename... Args>
constexpr
static
bool
match_type(const char(&fmt)[N], const std::size_t n, const T& arg, const Args&... args) {
    return supported<T>::match_type(fmt, n, 0, arg, args...);
}

template<std::size_t N, class T, typename... Args>
constexpr
static
bool
match_precision_number_maybe(const char(&fmt)[N], const std::size_t n, const T& arg, const Args&... args) {
    return n < N?
        (fmt[n] == '0' || fmt[n] == '1' || fmt[n] == '2' || fmt[n] == '3' || fmt[n] == '4' ||
         fmt[n] == '5' || fmt[n] == '6' || fmt[n] == '7' || fmt[n] == '8' || fmt[n] == '9')?
            match_precision_number_maybe(fmt, n + 1, arg, args...):
            match_type(fmt, n, arg, args...):
        false; // No more characters, but args left => extra arguments.
}

template<std::size_t N>
constexpr
static
bool
match_precision(const char(&)[N], const std::size_t) {
    return false;
}

template<std::size_t N, class T, typename... Args>
constexpr
static
bool
match_precision(const char(&fmt)[N], const std::size_t n, const T& arg, const Args&... args) {
    return n < N?
        fmt[n] == '.'? // Characters left, at least one arg left.
            n + 1 < N? // Matched '.'
                fmt[n + 1] == '*'? // At least there is one char.
                    std::is_same<T, int>::value? // User specified precision - check T == int
                        match_type(fmt, n + 2, args...): // Valid precision type - check placeholder type.
                        false: // Invalid precision type.
                    match_precision_number_maybe(fmt, n + 1, arg, args...): // Not '*', check other variants.
                false: // No more characters, but args left => extra arguments.
            match_type(fmt, n, arg, args...):  // Not matched '.' - it's a zero precision. Scan for type.
        false; // No more characters, but args left => extra arguments.
}

template<std::size_t N, class T, typename... Args>
constexpr
static
bool
match_width_number_maybe(const char(&fmt)[N], const std::size_t n, const T& arg, const Args&... args) {
    return n < N?
        (fmt[n] == '0' || fmt[n] == '1' || fmt[n] == '2' || fmt[n] == '3' || fmt[n] == '4' ||
         fmt[n] == '5' || fmt[n] == '6' || fmt[n] == '7' || fmt[n] == '8' || fmt[n] == '9')?
            match_width_number_maybe(fmt, n + 1, arg, args...):
            match_precision(fmt, n, arg, args...):
        false; // No more characters, but args left => extra arguments.
}

template<std::size_t N, class T, typename... Args>
constexpr
static
bool
match_width(const char(&fmt)[N], const std::size_t n, const T& arg, const Args&... args) {
    return n < N?
        fmt[n] == '*'? // Characters left, at least one arg left.
            std::is_same<T, int>::value? // User specified width - check T == int
                match_precision(fmt, n + 1, args...): // Valid user speified width type - check precision.
                false:                                // Invalid user speified width type.
            match_width_number_maybe(fmt, n, arg, args...): // Not '*', check other variants (number or nothing).
        false; // No more characters, but args left => extra arguments.
}

enum flags_t {
    none  = 0x00,
    minus = 0x01,
    plus  = 0x02,
    space = 0x04,
    sharp = 0x08,
    zero  = 0x10
};

template<std::size_t N, class T, typename... Args>
constexpr
static
bool
match_flags(const char(&fmt)[N], const std::size_t n, int flags, const T& arg, const Args&... args) {
    return n < N?
        fmt[n] == '-' ?
            (flags & zero) == zero?
                false:
                match_flags(fmt, n + 1, flags | minus, arg, args...):
            fmt[n] == '+'?
                (flags & space) == space?
                    false: // Combination of '+' and ' ' is not allowed.
                    match_flags(fmt, n + 1, flags | plus, arg, args...):
                fmt[n] == ' '?
                    (flags & plus) == plus?
                        false: // Combination of '+' and ' ' is not allowed.
                        match_flags(fmt, n + 1, flags | space, arg, args...):
                    fmt[n] == '#'?
                        match_flags(fmt, n + 1, flags | sharp, arg, args...):
                        fmt[n] == '0'?
                            (flags & minus) == minus?
                                false:
                                match_flags(fmt, n + 1, flags | zero, arg, args...):
                            match_width(fmt, n, arg, args...):
        false; // No more characters, but args left => extra arguments.
}

template<std::size_t N>
constexpr
static
bool
match_whatever(const char(&fmt)[N], const std::size_t n) {
    return n < N?
        fmt[n] == '%'? // Characters left, no more args left.
            fmt[n + 1] == '%'? // First '%' match.
                match_whatever(fmt, n + 2): // Matched '%%' - continue.
                false:                      // Not '%%' - just '%', but no there is more args.
            match_whatever(fmt, n + 1):     // Common character - start again.
        true; // No more characters, and no more args.
}

constexpr
static
bool warn_extra_arg() {
    return false;
}

template<std::size_t N, class T, typename... Args>
constexpr
static
bool
match_whatever(const char(&fmt)[N], const std::size_t n, const T& arg, const Args&... args) {
    return n < N?
        fmt[n] == '%'? // Characters left, at least one arg left.
            fmt[n + 1] == '%'? // First '%' match.
                match_whatever(fmt, n + 2, arg, args...): // Found '%%' - start again.
                match_flags(fmt, n + 1, none, arg, args...): // Try to match flags.
            match_whatever(fmt, n + 1, arg, args...):     // Common character - start again.
        warn_extra_arg(); // No more characters, but args left => extra arguments.
}

template<std::size_t N, typename... Args>
constexpr
static
bool
check_syntax(const char(&fmt)[N], const Args&... args) {
    return match_whatever(fmt, 0, args...);
}

static_assert(!check_syntax("", 42),                "fail | incomplete | extra arg");
static_assert(!check_syntax("incomplete", 42),      "fail | incomplete | extra arg");
static_assert(!check_syntax("%"),                   "fail | incomplete | single");
static_assert(!check_syntax("%", 42),               "fail | incomplete | single");
static_assert(!check_syntax("%d %", 42),            "fail | digit | single | incomplete");

static_assert( check_syntax("%%"),                  "pass | percent | single");
static_assert( check_syntax(">%%"),                 "pass | percent | single | prefix");
static_assert( check_syntax("%%>"),                 "pass | percent | single | suffix");
static_assert( check_syntax(">%%>"),                "pass | percent | single | prefix suffix");
static_assert(!check_syntax("%%", 42),              "fail | percent | single | extra arg");

static_assert( check_syntax("%d", 42),              "pass | digit | single");
static_assert(!check_syntax("%d"),                  "fail | digit | single | unsufficient args");
static_assert(!check_syntax("%d", 42, 10),          "fail | digit | single | extra arg");

static_assert( check_syntax("%ld", 42L),            "pass | long digit | single");
static_assert(!check_syntax("%ld", 42),             "fail | long digit | single | type mismatch");

static_assert( check_syntax(">%d", 42),             "pass | digit | single | prefix");
static_assert( check_syntax("%d>", 42),             "pass | digit | single | suffix");
static_assert( check_syntax(">%d>", 42),            "pass | digit | single | prefix suffix");

static_assert( check_syntax("%d %d", 42, 10),       "pass | digit | multiple");
static_assert(!check_syntax("%d %d", 42),           "fail | digit | multiple | unsufficient args");
static_assert(!check_syntax("%d %d", 42, 10, 5),    "fail | digit | multiple | extra arg");

static_assert(!check_syntax("%d", 3.14),            "fail | digit | single | type mismatch");
static_assert(!check_syntax("%d", "lit"),           "fail | digit | single | type mismatch");

static_assert( check_syntax("%.d", 42),             "pass | digit | signle | precision zero");
static_assert( check_syntax("%.6d", 42),            "pass | digit | signle | precision digit");
static_assert( check_syntax("%.10d", 42),           "pass | digit | signle | precision digits");
static_assert( check_syntax("%.100d", 42),          "pass | digit | signle | precision digits");
static_assert( check_syntax("%.*d", 6, 42),         "pass | digit | signle | precision star");
static_assert(!check_syntax("%.*d", 6),             "fail | digit | signle | unsufficient args");
static_assert(!check_syntax("%.*d", 3.14, 42),      "fail | digit | signle | precision star | type mismatch");

static_assert( check_syntax("%6d", 42),             "pass | digit | signle | width digit");
static_assert( check_syntax("%10d", 42),            "pass | digit | signle | width digits");
static_assert( check_syntax("%100d", 42),           "pass | digit | signle | width digits");
static_assert( check_syntax("%*d", 6, 42),          "pass | digit | signle | width star");
static_assert(!check_syntax("%*d", 6),              "fail | digit | signle | width star | unsufficion args");
static_assert(!check_syntax("%*d", 3.1415, 42),     "fail | digit | signle | width star | type mismatch");

static_assert( check_syntax("%6.4d", 42),           "pass | digit | signle | width digit | precision digit");
static_assert( check_syntax("%6.*d", 4, 42),        "pass | digit | signle | width digit | precision star");
static_assert( check_syntax("%*.*d", 6, 4, 42),     "pass | digit | signle | width star | precision star");

static_assert(!check_syntax("%6.4d"),               "fail | digit | signle | width digit | precision digit | unsufficion args");
static_assert(!check_syntax("%6.*d"),               "fail | digit | signle | width digit | precision star | unsufficion args");
static_assert(!check_syntax("%6.*d", 42),           "fail | digit | signle | width digit | precision star | unsufficion args");
static_assert(!check_syntax("%*.*d"),               "fail | digit | signle | width star | precision star | unsufficion args");
static_assert(!check_syntax("%*.*d", 42),           "fail | digit | signle | width star | precision star | unsufficion args");
static_assert(!check_syntax("%*.*d", 6, 42),        "fail | digit | signle | width star | precision star | unsufficion args");

static_assert( check_syntax("%-d", 42),             "pass | digit | signle | left align flag");
static_assert( check_syntax("%--d", 42),            "pass | digit | signle | left align flag duplicated");
static_assert( check_syntax("%+d", 42),             "pass | digit | signle | plus flag");
static_assert( check_syntax("%++d", 42),            "pass | digit | signle | plus flag duplicated");
static_assert( check_syntax("% d", 42),             "pass | digit | signle | space flag");
static_assert( check_syntax("%  d", 42),            "pass | digit | signle | space flag duplicated");
static_assert( check_syntax("%#x", 42),             "pass | digit | signle | sharp flag");
static_assert( check_syntax("%##x", 42),            "pass | digit | signle | sharp flag duplicated");
//static_assert(!check_syntax("%#d", 42),             "fail | digit | signle | sharp flag");
//static_assert(!check_syntax("%##d", 42),            "fail | digit | signle | sharp flag duplicated");
static_assert( check_syntax("%0d", 42),             "pass | digit | signle | zero flag");
static_assert( check_syntax("%00d", 42),            "pass | digit | signle | zero flag duplicated");

static_assert( check_syntax("%-+-+d", 42),          "pass | digit | signle | combination");

static_assert(!check_syntax("% +d", 42),            "fail | digit | signle | '+' and ' ' not allowed");
static_assert(!check_syntax("%+ d", 42),            "fail | digit | signle | '+' and ' ' not allowed");
static_assert(!check_syntax("%- +d", 42),           "fail | digit | signle | '+' and ' ' not allowed");
static_assert(!check_syntax("%-+ d", 42),           "fail | digit | signle | '+' and ' ' not allowed");
static_assert(!check_syntax("%-0d", 42),            "fail | digit | signle | '-' and '0' not allowed");
static_assert(!check_syntax("%0-d", 42),            "fail | digit | signle | '-' and '0' not allowed");
static_assert(!check_syntax("%- +0d", 42),          "fail | digit | signle | '+' and ' ' not allowed");
static_assert(!check_syntax("%-+ 0d", 42),          "fail | digit | signle | '+' and ' ' not allowed");

static_assert( check_syntax("%i", 42),              "pass | int | single");
static_assert( check_syntax("%o", 42),              "pass | oct | single");
static_assert( check_syntax("%x", 42),              "pass | hex | single");
static_assert( check_syntax("%X", 42),              "pass | Hex | single");

static_assert( check_syntax("%u", 42u),             "pass | unsigned int | single");

static_assert( check_syntax("%f", 3.14),            "pass | float | single");
static_assert( check_syntax("%F", 3.14),            "pass | float uppercase | single");
static_assert( check_syntax("%e", 3.14),            "pass | scientific | single");
static_assert( check_syntax("%E", 3.14),            "pass | scientific uppercase | single");
static_assert( check_syntax("%g", 3.14),            "pass | float shortest | single");
static_assert( check_syntax("%G", 3.14),            "pass | float shortest uppercase | single");
static_assert( check_syntax("%a", 3.14),            "pass | float hex | single");
static_assert( check_syntax("%A", 3.14),            "pass | float hex uppercase | single");

static_assert( check_syntax("%c", '5'),             "pass | char | single");
static_assert( check_syntax("%s", "lit"),           "pass | literal | single");

std::string s("str");
static_assert( check_syntax("%s", s),               "pass | string | single");

#ifdef BLACKHOLE_HAS_STATIC_PRINTF_OPERATOR_PUSH_SUPPORT
static_assert( check_syntax("%s", 42),              "pass | string | single | convertible via <<");
static_assert( check_syntax("%s", 3.14),            "pass | string | single | convertible via <<");
#endif
