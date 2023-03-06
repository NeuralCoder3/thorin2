#pragma once

#include <thorin/analyses/scope.h>
#include <thorin/axiom.h>
#include <thorin/def.h>
#include <thorin/lam.h>

#include "dialects/ad_imp/analysis/gradient_analysis.h"
#include "dialects/ad_imp/utils/helper.h"
#include "dialects/affine/affine.h"
#include "dialects/math/math.h"
#include "dialects/mem/mem.h"

namespace thorin::ad_imp {

class AnalysisFactory;

class LiveAnalysis : public Analysis {
public:
    LiveAnalysis(AnalysisFactory& factory);
    LiveAnalysis(LiveAnalysis& other) = delete;

    std::unique_ptr<DefMap<Lam*>> end_of_live_;

    void run();

    void build_end_of_live();
    DefMap<Lam*>& end_of_live();
    Lam* end_of_live(const Def* def);
};

} // namespace thorin::ad_imp
