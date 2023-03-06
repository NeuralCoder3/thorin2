#include "dialects/ad_imp/analysis/affine_cfa.h"

#include "dialects/ad_imp/analysis/analysis.h"
#include "dialects/ad_imp/analysis/analysis_factory.h"

namespace thorin::ad_imp {

AffineCFA::AffineCFA(AnalysisFactory& factory)
    : Analysis(factory) {
    run(factory.lam());
}

} // namespace thorin::ad_imp
