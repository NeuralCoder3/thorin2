#include "dialects/betest/be/ll/extension.h"

#include "dialects/core/be/ll/ll.h"
#include "dialects/core/core.h"

using namespace thorin;

std::optional<std::string> extend(ll::Emitter* emitter, ll::BB* bb, const Def*& def) {
    if (auto wrap = match<core::wrap>(def)) {
        auto [a, b]        = wrap->args<2>([emitter](auto def) { return emitter->emit(def); });
        auto t             = emitter->convert(wrap->type());
        auto [mode, width] = wrap->decurry()->args<2>(as_lit<nat_t>);

        std::string op;
        switch (wrap.id()) {
            case core::wrap::add: op = "add"; break;
            case core::wrap::sub: op = "sub"; break;
            case core::wrap::mul: op = "mul"; break;
            case core::wrap::shl: op = "shl"; break;
        }

        if (mode & core::WMode::nuw) op += " nuw";
        if (mode & core::WMode::nsw) op += " nsw";

        auto name = emitter->id(def);
        return std::optional(bb->assign(name, "{} {} {}, {}", op, t, a, b));
    }

    return std::optional<std::string>{};
}
