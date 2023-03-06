#include "dialects/ad_imp/analysis/affine_dfa.h"

#include "dialects/ad_imp/analysis/analysis.h"
#include "dialects/ad_imp/analysis/analysis_factory.h"
#include "dialects/mem/autogen.h"

namespace thorin::ad_imp {

AffineDFA::AffineDFA(AnalysisFactory& factory)
    : Analysis(factory) {
    run();
}

void AffineDFA::run() {
    for (auto cfa_node : factory().cfa().post_order()) {
        if (auto lam = cfa_node->def()->isa_nom<Lam>()) { run(lam); }
    }
}

} // namespace thorin::ad_imp
