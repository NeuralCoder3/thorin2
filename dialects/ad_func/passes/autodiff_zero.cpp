#include "dialects/ad_func/passes/autodiff_zero.h"

#include <iostream>

#include <thorin/lam.h>

#include "dialects/ad_func/autodiff.h"
#include "dialects/ad_func/auxiliary/autodiff_aux.h"
#include "dialects/affine/affine.h"
#include "dialects/core/core.h"
#include "dialects/mem/mem.h"

namespace thorin::ad_func {

const Def* AutoDiffZero::rewrite(const Def* def) {
    if (auto zero_app = match<zero>(def); zero_app) {
        // callee = zero
        // arg = type T
        auto T = zero_app->arg();
        world().DLOG("found a autodiff::zero of {}", T);
        auto zero = zero_def(T);
        if (zero) return zero;
        return def;
    }

    return def;
}

} // namespace thorin::ad_func
