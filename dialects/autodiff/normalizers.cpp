#include <iostream>

#include "thorin/axiom.h"
#include "thorin/def.h"
#include "thorin/world.h"

#include "dialects/ad_func/autodiff.h"
#include "dialects/autodiff/autogen.h"
// #include "dialects/ad_func/auxiliary/autodiff_aux.h"
#include "dialects/core/core.h"
#include "dialects/math/math.h"
#include "dialects/mem/mem.h"

namespace thorin::autodiff {

const Def* normalize_ad(const Def* type, const Def* callee, const Def* arg, const Def* dbg) {
    auto& world     = type->world();
    auto [ax, args] = collect_args(callee);

    args.push_back(arg);
    auto app_ad = apply_args(world.ax<ad_func::ad>(), args, dbg);
    return app_ad;
}

const Def* normalize_AD(const Def* type, const Def* callee, const Def* arg, const Def* dbg) {
    auto& world     = type->world();
    auto [ax, args] = collect_args(callee);

    args.push_back(arg);
    auto app_ad = apply_args(world.ax<ad_func::AD>(), args, dbg);
    return app_ad;
}

THORIN_autodiff_NORMALIZER_IMPL

} // namespace thorin::autodiff
