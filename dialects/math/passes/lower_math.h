#pragma once

#include <string>
#include <vector>

#include <thorin/def.h>
#include <thorin/pass/pass.h>

#include "dialects/affine/affine.h"
#include "dialects/autodiff/analysis/analysis_factory.h"
#include "dialects/autodiff/analysis/cache_analysis.h"
#include "dialects/autodiff/analysis/gradient_analysis.h"
#include "dialects/autodiff/passes/def_inliner.h"
#include "dialects/autodiff/utils/helper.h"
#include "dialects/mem/mem.h"

namespace thorin::math {

class LowerMath : public RWPass<LowerMath, Lam> {
public:


    LowerMath(PassMan& man)
    : RWPass(man, "lower_math") {}

    const Def* rewrite(const Def*) override;
};

} // namespace thorin::autodiff
