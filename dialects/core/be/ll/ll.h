#pragma once

#include <deque>
#include <fstream>
#include <iomanip>
#include <ostream>

#include "thorin/dialects.h"

namespace thorin {

class World;

namespace ll {

void emit(World&, std::ostream&, Extensions);

int compile(World&, std::string name);
int compile(World&, std::string ll, std::string out);
int compile_and_run(World&, std::string name, std::string args = {});

struct BB {
    BB()          = default;
    BB(const BB&) = delete;
    BB(BB&& other) { swap(*this, other); }

    std::deque<std::ostringstream>& head() { return parts[0]; }
    std::deque<std::ostringstream>& body() { return parts[1]; }
    std::deque<std::ostringstream>& tail() { return parts[2]; }

    template<class... Args>
    std::string assign(std::string_view name, const char* s, Args&&... args) {
        print(print(body().emplace_back(), "{} = ", name), s, std::forward<Args&&>(args)...);
        return std::string(name);
    }

    template<class... Args>
    void tail(const char* s, Args&&... args) {
        print(tail().emplace_back(), s, std::forward<Args&&>(args)...);
    }

    friend void swap(BB& a, BB& b) {
        using std::swap;
        swap(a.phis, b.phis);
        swap(a.parts, b.parts);
    }

    DefMap<std::vector<std::pair<std::string, std::string>>> phis;
    std::array<std::deque<std::ostringstream>, 3> parts;
};

class Emitter : public thorin::Emitter<std::string, std::string, BB, Emitter, Extensions> {
public:
    using Super = thorin::Emitter<std::string, std::string, BB, Emitter, Extensions>;

    Emitter(World& world, std::ostream& ostream, Extensions extensions)
        : Super(world, "llvm_emitter", ostream, extensions) {}

    bool is_valid(std::string_view s) { return !s.empty(); }
    void start() override;
    void emit_imported(Lam*);
    void emit_epilogue(Lam*);
    std::string convert(const Def*);
    std::string emit_bb(BB&, const Def*);
    std::string prepare(const Scope&);
    void prepare(Lam*, std::string_view);
    void finalize(const Scope&);

private:
    std::string id(const Def*, bool force_bb = false) const;
    std::string convert_ret_pi(const Pi*);

    std::ostringstream type_decls_;
    std::ostringstream vars_decls_;
    std::ostringstream func_decls_;
    std::ostringstream func_impls_;
};

} // namespace ll
} // namespace thorin
