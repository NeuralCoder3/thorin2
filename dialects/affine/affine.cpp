#include "dialects/affine/affine.h"

#include <thorin/config.h>
#include <thorin/pass/pass.h>

#include "thorin/dialects.h"

// #include "thorin/pass/pipelinebuilder.h"

#include "dialects/affine/passes/lower_for.h"

using namespace thorin;

extern "C" THORIN_EXPORT thorin::DialectInfo thorin_get_dialect_info() {
    return {"affine",
            [](thorin::PipelineBuilder& builder) {
                builder.extend_opt_phase([](thorin::PassMan& man) { man.add<thorin::affine::LowerFor>(); });

                // TODO: check for AD, (if delyed -> needs opt pass to generate code)
                // builder.extend_opt_phase(106, [](thorin::PassMan& man) { man.add<thorin::affine::LowerFor>(); });
            },
            [](Passes& passes) { register_pass<affine::lower_for_pass, affine::LowerFor>(passes); }, nullptr, nullptr};
}
