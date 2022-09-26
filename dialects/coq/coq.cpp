#include "dialects/coq/coq.h"

#include <thorin/config.h>
#include <thorin/dialects.h>
#include <thorin/pass/pass.h>

#include "dialects/coq/be/coq/coq.h"

using namespace thorin;

extern "C" THORIN_EXPORT thorin::DialectInfo thorin_get_dialect_info() {
    return {"coq", nullptr, [](Backends& backends) { backends["coq"] = &coq::emit; }, nullptr};
    // return {"coq", [](thorin::PipelineBuilder& builder) { builder.extend_codegen_prep_phase([](PassMan& man) {}); },
    //         nullptr, nullptr};
    // nullptr, [](Normalizers& normalizers) { coq::register_normalizers(normalizers); }};
}

namespace thorin::coq {} // namespace thorin::coq
