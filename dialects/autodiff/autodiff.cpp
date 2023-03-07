#include "dialects/autodiff/autodiff.h"

#include <thorin/config.h>
#include <thorin/pass/pass.h>

#include "thorin/dialects.h"

using namespace thorin;
using namespace thorin::autodiff;

extern "C" THORIN_EXPORT thorin::DialectInfo thorin_get_dialect_info() {
    return {"autodiff",
            [](Passes& passes) {
                // all passes are deletegated at normalization time
                // TODO: is this correct?
                // maybe run both => no custom axioms, only group them in let bindings
            },
            nullptr, [](Normalizers& normalizers) { register_normalizers(normalizers); }};
}
