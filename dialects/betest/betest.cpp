#include "dialects/betest/betest.h"

#include <thorin/config.h>
#include <thorin/dialects.h>
#include <thorin/pass/pass.h>

#include "dialects/betest/be/ll/extension.h"

using namespace thorin;

extern "C" THORIN_EXPORT thorin::DialectInfo thorin_get_dialect_info() {
    return {"betest", nullptr,
            [](Backends& backends, BackendExtensions& extensions) {
                if (!extensions.contains("ll")) { extensions["ll"] = {}; }
                // extensions["ll"].push_back([]() { return std::optional<std::string>{}; });
                // extensions["ll"].push_back();
                // extensions["ll"].push_back();
            },
            [](Normalizers& normalizers) { betest::register_normalizers(normalizers); }};
}
