#include "thorin/normalize.h"
#include "thorin/world.h"

#include "dialects/rusty/rust_rusty/rust_rusty.h"
#include "dialects/rusty/rusty.h"

namespace thorin::rusty {

const Def* normalize_constant(const Def* type, const Def* callee, const Def* arg, const Def* dbg) {
    auto& world = type->world();

    return world.lit_int_width(32, 42);

    // do nothing normalizer
    // return world.raw_app(callee, arg, dbg);
}

const Def* normalize_constant_fun(const Def* type, const Def* callee, const Def* arg, const Def* dbg) {
    auto& world = type->world();

    int val = rust_rusty::rust_echo(35);
    return world.lit_int_mod(as_lit(arg), val);
}

THORIN_rusty_NORMALIZER_IMPL

} // namespace thorin::rusty
