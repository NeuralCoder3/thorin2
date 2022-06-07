#include "dialects/core.h"

#include <thorin/config.h>
#include <thorin/pass/pass.h>

#include "thorin/dialects.h"

using namespace thorin;

extern "C" THORIN_EXPORT DialectInfo thorin_get_dialect_info() {
    return {"core", nullptr, nullptr, [](Normalizers& normalizers) { core::register_normalizers(normalizers); }};
}
