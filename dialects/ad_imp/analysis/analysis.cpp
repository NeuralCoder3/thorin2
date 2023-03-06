#include "dialects/ad_imp/analysis/analysis.h"

#include "dialects/ad_imp/analysis/analysis_factory.h"

namespace thorin::ad_imp {

Analysis::Analysis(AnalysisFactory& factory)
    : factory_(factory) {}

AnalysisFactory& Analysis::factory() { return factory_; }

} // namespace thorin::ad_imp
