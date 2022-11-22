#pragma once

#include <thorin/analyses/scope.h>
#include <thorin/axiom.h>
#include <thorin/def.h>
#include <thorin/lam.h>

#include "dialects/affine/affine.h"
#include "dialects/autodiff/auxiliary/analysis.h"
#include "dialects/autodiff/auxiliary/autodiff_alias_analysis.h"
#include "dialects/autodiff/auxiliary/autodiff_aux.h"
#include "dialects/math/math.h"
#include "dialects/mem/mem.h"

namespace thorin::autodiff {

class AnalysisFactory;

class AliasAnalysis;
class CacheAnalysis : public Analysis {
public:
    AliasAnalysis& alias;
    DefSet requirements;
    DefSet requirements_filtered;
    DefSet targets_;
    DefSet loads_;
    LamMap<DefSet> lam2loads_;
    explicit CacheAnalysis(AnalysisFactory& factory);
    CacheAnalysis(CacheAnalysis& other) = delete;

    DefSet& targets() { return targets_; }
    DefSet& loads() { return loads_; }

    void depends_on_loads(const Def* def, DefSet& set, DefSet& loads);
    bool requires_caching(const Def* def);
    DefSet& requires_loading(Lam* lam);
    void require(const Def* def);
    void visit(const Def* def);
    void filter(const Def* def);
    void filter();
    void run();
};

} // namespace thorin::autodiff
