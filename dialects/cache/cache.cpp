#include "dialects/cache/cache.h"

#include <thorin/pass/pass.h>
#include <thorin/plugin.h>

using namespace thorin;

extern "C" THORIN_EXPORT Plugin thorin_get_plugin() {
    return {"cache", nullptr,
            [](Passes& passes) {
                passes[flags_t(Annex::Base<cache::memoize>)] = [&](World&, PipelineBuilder& builder, const Def* app) {
                    auto phase_code = app->as<App>()->arg();
                    // TODO: save phase like pass
                    // auto phase = builder.phase(phase_code);
                    // TODO: phase should not run at other place; just here
                    // not already added; just registered => wrap in register axiom that separately constructs but not
                    // adds (directly separate branch during static parsing)

                    // Ideally, a clean restructure of passes and phases in the builder
                    // => first construct
                    // then add
                };
            },
            nullptr};
}
