#include "thorin/world.h"

#include "dialects/betest/betest.h"

namespace thorin::betest {

const Def* normalize_const(const Def*, const Def*, const Def* arg, const Def*) {
    auto& world = arg->world();
    return world.lit_idx(world.type_idx(arg), 42);
}

THORIN_betest_NORMALIZER_IMPL

} // namespace thorin::betest
