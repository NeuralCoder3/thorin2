#include "dialects/ad_func/passes/autodiff_zero_cleanup.h"

#include <iostream>

#include <thorin/lam.h>

#include "dialects/ad_func/autodiff.h"
#include "dialects/affine/affine.h"
#include "dialects/core/core.h"
#include "dialects/mem/mem.h"

namespace thorin::ad_func {

const Def* AutoDiffZeroCleanup::rewrite(const Def* def) {
    // Ideally this code segment is never reached.
    // (all zeros are resolved before)
    if (auto zero_app = match<zero>(def); zero_app) {
        // callee = zero
        // arg = type T
        auto T = zero_app->arg();
        world().DLOG("found a remaining autodiff::zero of {}", T);
        // generate ⊥:T
        auto dummy = world().bot(T);
        return dummy;
    }

    return def;
}

} // namespace thorin::ad_func
