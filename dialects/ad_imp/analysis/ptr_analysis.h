#pragma once

#include <thorin/analyses/scope.h>
#include <thorin/axiom.h>
#include <thorin/def.h>
#include <thorin/lam.h>

#include "dialects/ad_imp/analysis/analysis.h"
#include "dialects/ad_imp/utils/union.h"
#include "dialects/affine/affine.h"
#include "dialects/math/math.h"
#include "dialects/mem/mem.h"

namespace thorin::ad_imp {

class AnalysisFactory;
class PtrAnalysis : public Analysis {
public:
    std::unordered_map<const Def*, std::unique_ptr<DefUnionNode>> ptr_union;

    PtrAnalysis(AnalysisFactory& factory);

    const Def* representative(const Def* def) {
        auto node = ptr_node(def);
        return find(node)->value;
    }

    DefUnionNode* ptr_node(const Def* def);

    void run();
};

} // namespace thorin::ad_imp
