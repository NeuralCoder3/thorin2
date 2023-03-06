#pragma once

#include <thorin/analyses/scope.h>
#include <thorin/axiom.h>
#include <thorin/def.h>
#include <thorin/lam.h>

#include "dialects/ad_imp/analysis/affine_cfa.h"
#include "dialects/ad_imp/analysis/affine_dfa.h"
#include "dialects/ad_imp/analysis/alias_analysis.h"
#include "dialects/ad_imp/analysis/cache_analysis.h"
#include "dialects/ad_imp/analysis/gradient_analysis.h"
#include "dialects/ad_imp/analysis/ptr_analysis.h"
#include "dialects/ad_imp/analysis/utils.h"
#include "dialects/ad_imp/analysis/war_analysis.h"
#include "dialects/affine/affine.h"
#include "dialects/mem/mem.h"
#include "live_analysis.h"

namespace thorin::ad_imp {

class AnalysisFactory {
    Lam* lam_;
    Scope scope_;
    std::unique_ptr<GradientAnalysis> gradient_;
    std::unique_ptr<CacheAnalysis> cache_;
    std::unique_ptr<AliasAnalysis> alias_;
    std::unique_ptr<Utils> utils_;
    std::unique_ptr<AffineCFA> cfa_;
    std::unique_ptr<AffineDFA> dfa_;
    std::unique_ptr<PtrAnalysis> ptr_;
    std::unique_ptr<WarAnalysis> war_;
    std::unique_ptr<LiveAnalysis> live_;

public:
    explicit AnalysisFactory(Lam* lam)
        : lam_(lam)
        , scope_(lam) {}

    Lam* lam() { return lam_; }

    template<class T>
    inline T& lazy_init(std::unique_ptr<T>& ptr) {
        return *(ptr ? ptr : ptr = std::make_unique<T>(*this));
    }

    GradientAnalysis& gradient() { return lazy_init(gradient_); }

    CacheAnalysis& cache() { return lazy_init(cache_); }

    AliasAnalysis& alias() { return lazy_init(alias_); }

    Utils& utils() { return lazy_init(utils_); }

    AffineCFA& cfa() { return lazy_init(cfa_); }

    AffineDFA& dfa() { return lazy_init(dfa_); }

    PtrAnalysis& ptr() { return lazy_init(ptr_); }

    WarAnalysis& war() { return lazy_init(war_); }

    LiveAnalysis& live() { return lazy_init(live_); }
};

} // namespace thorin::ad_imp
