
#include "dialects/mem/passes/rw/reshape.h"

#include <functional>

#include "thorin/check.h"

#include "dialects/mem/mem.h"

namespace thorin::mem {

void Reshape::enter() { rewrite(curr_nom()); }

const Def* Reshape::rewrite(const Def* def) {
    if (auto i = old2new_.find(def); i != old2new_.end()) return i->second;
    auto new_def  = rewrite_(def);
    old2new_[def] = new_def;
    // TODO: necessary?
    old2new_[new_def] = new_def;
    return new_def;
}

// TODO: should be handled more generally
const Def* Reshape::rewrite_(const Def* def) {
    // ignore types
    switch (def->node()) {
        // TODO: check if bot: Cn[[A,B],Cn[Ret]] is handled correctly
        // case Node::Bot:
        // case Node::Top:
        case Node::Type:
        case Node::Univ:
        case Node::Nat: return def;
    }

    // Var should be handled by memoization
    if (def->isa<Var>() || def->isa<Axiom>()) { return def; }

    auto& w       = world();
    auto new_type = rewrite(def->type());
    auto new_dbg  = def->dbg() ? rewrite(def->dbg()) : nullptr;

    if (auto app = def->isa<App>()) {
        // For applications, we shape the argument tuple according to the calle type.
        auto callee = rewrite(app->callee());
        auto arg    = rewrite(app->arg());

        // Axiom types are fixed
        // TODO: callee should not change for axioms => remove if
        // if (!callee->isa<Axiom>()) {
        // arg = reshape(arg, callee->type()->as<Pi>());
        auto reshaped_arg = reshape(arg);
        // }

        auto new_app = w.app(callee, reshaped_arg);
        return new_app;
    } else if (auto lam = def->isa_nom<Lam>()) {
        Lam* new_lam = lam;
        // if (lam->is_set()) { new_lam = convert(lam)->as_nom<Lam>(); }

        // old2new_[def] = new_lam;
        // if (lam->is_set()) {
        //     auto new_body = rewrite(lam->body());
        //     new_lam->set_body(new_body);
        // }

        return reshapeLam(lam);
    } else {
        auto new_ops = DefArray(def->num_ops(), [&](auto i) { return rewrite(def->op(i)); });
        auto new_def = def->rebuild(w, new_type, new_ops, new_dbg);
        return new_def;
    }
}

Lam* Reshape::reshapeLam(Lam* def) {
    auto pi_ty  = def->type();
    auto new_ty = reshape_type(pi_ty)->as<Pi>();
    // if (new_ty == pi_ty) return def;

    auto& w       = def->world();
    Lam* new_lam  = w.nom_lam(new_ty, w.dbg(def->name() + "_reshaped"));
    old2new_[def] = new_lam;

    // We associate the arguments (reshape the old vars).
    // Alternatively, we could use beta reduction (reduce) to do this for us.

    // auto arg     = reshape(wrapper->var(), def_ty);
    // auto arg       = reshape(def->var(), new_ty->dom());
    auto arg       = reshape(def->var());
    auto num_projs = arg->num_projs();
    auto new_arg   = new_lam->var();
    assert(num_projs == new_arg->num_projs() && "Reshape of lambda should agree with tuple reshape");
    for (int i = 0; i < num_projs; i++) { old2new_[arg->proj(i)] = new_arg->proj(i); }

    auto new_body = rewrite(def->body());
    new_lam->set_body(new_body);
    new_lam->set_filter(true);

    return new_lam;
}

bool should_flatten(const Def* T) { return T->isa<Sigma>(); }

std::vector<const Def*> flatten_ty(const Def* T) {
    std::vector<const Def*> types;
    if (should_flatten(T)) {
        for (auto P : T->projs()) {
            auto inner_types = flatten_ty(P->type());
            types.insert(types.end(), inner_types.begin(), inner_types.end());
        }
    } else {
        types.push_back(T);
    }
    return types;
}

bool is_mem_ty(const Def* T) { return match<mem::M>(T); }
DefArray vec2array(const std::vector<const Def*>& vec) { return DefArray(vec.begin(), vec.end()); }

const Def* Reshape::reshape_type(const Def* ty) {
    auto& w = ty->world();

    if (auto pi = ty->isa<Pi>()) {
        auto new_dom = reshape_type(pi->dom());
        auto new_cod = reshape_type(pi->codom());
        return w.pi(new_dom, new_cod);
    } else if (auto sigma = ty->isa<Sigma>()) {
        auto new_types = flatten_ty(sigma);
        if (mode_ == Mode::Flat) {
            return w.sigma(vec2array(new_types));
        } else {
            if (new_types.size() == 0) return w.sigma();
            const Def* mem;
            const Def* ret;
            // find mem, erase all mems
            for (auto i = new_types.begin(); i != new_types.end(); i++) {
                if (is_mem_ty(*i)) {
                    if (!mem) mem = *i;
                    new_types.erase(i);
                }
            }
            // TODO: more fine-grained test
            if (new_types.back()->isa<Pi>()) {
                auto ret = new_types.back();
                new_types.pop_back();
            }
            // create form [[mem,args],ret]
            const Def* args = w.sigma(vec2array(new_types));
            if (mem) { args = w.sigma({mem, args}); }
            if (ret) { args = w.sigma({args, ret}); }
            return args;
        }
    } else {
        return ty;
    }
}

std::vector<const Def*> flatten_def(const Def* def) {
    std::vector<const Def*> defs;
    if (should_flatten(def->type())) {
        auto& w = def->world();
        for (auto P : def->projs()) {
            auto inner_defs = flatten_def(P);
            defs.insert(defs.end(), inner_defs.begin(), inner_defs.end());
        }
    } else {
        defs.push_back(def);
    }
    return defs;
}

// called for new lambda arguments, app arguments
const Def* Reshape::reshape(const Def* def) {
    auto& w = def->world();

    auto flat_defs = flatten_def(def);
    if (flat_defs.size() == 1) return flat_defs[0];
    if (mode_ == Mode::Flat) {
        return w.tuple(vec2array(flat_defs));
    } else {
        // arg style
        // [[mem,args],ret]
        const Def* mem;
        const Def* ret;
        // find mem, erase all mems
        for (auto i = flat_defs.begin(); i != flat_defs.end(); i++) {
            if (is_mem_ty((*i)->type())) {
                if (!mem) mem = *i;
                flat_defs.erase(i);
            }
        }
        if (flat_defs.back()->type()->isa<Pi>()) {
            ret = flat_defs.back();
            flat_defs.pop_back();
        }
        const Def* args = w.tuple(vec2array(flat_defs));
        if (mem) { args = w.tuple({mem, args}); }
        if (ret) { args = w.tuple({args, ret}); }
        return args;
    }
}

} // namespace thorin::mem
