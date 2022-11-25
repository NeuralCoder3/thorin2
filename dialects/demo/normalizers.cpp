#include "thorin/world.h"

#include "dialects/demo/demo.h"

namespace thorin::demo {

const Def* normalize_const(const Def*, const Def*, const Def* arg, const Def*) {
    auto& world = arg->world();
    return world.lit(world.type_idx(arg), 42);
}

const Def* normalize_id_fun(const Def* type, const Def* callee, const Def* arg, const Def* dbg) {
    auto& world = arg->world();
    world.DLOG("Normalize id_fun with arg {}", arg);
    return world.raw_app(callee, arg, dbg);
}

THORIN_demo_NORMALIZER_IMPL

} // namespace thorin::demo
