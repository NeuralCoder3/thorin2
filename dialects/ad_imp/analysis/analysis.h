#pragma once

#include <thorin/analyses/scope.h>
#include <thorin/axiom.h>
#include <thorin/def.h>
#include <thorin/lam.h>

#include "dialects/ad_imp/utils/helper.h"
#include "dialects/affine/affine.h"
#include "dialects/math/math.h"
#include "dialects/mem/mem.h"

namespace thorin::ad_imp {

class AnalysisFactory;

class Analysis {
protected:
public:
    AnalysisFactory& factory_;
    explicit Analysis(AnalysisFactory& factory);
    Analysis(Analysis& other) = delete;

    AnalysisFactory& factory();
};

} // namespace thorin::ad_imp
