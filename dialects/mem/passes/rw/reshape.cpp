
#include "dialects/mem/passes/rw/reshape.h"

#include <functional>
#include <sstream>
#include <vector>

#include "thorin/check.h"
#include "thorin/def.h"

#include "dialects/mem/mem.h"

namespace thorin::mem {

void Reshape::enter() { rewrite(curr_nom()); }

const Def* Reshape::rewrite(const Def* def) {
    // world().DLOG("rewrite {} : {} [{}]", def, def->type(), def->node_name());
    if (auto i = old2new_.find(def); i != old2new_.end()) return i->second;
    auto new_def  = rewrite_(def);
    old2new_[def] = new_def;
    return new_def;
}

bool should_flatten(const Def* T) {
    // handle [] cases
    if (T->isa<Sigma>()) return true;
    // also handle normalized tuple-arrays ((a:I32,b:I32) : <<2;I32>>)
    if (auto lit = T->arity()->isa<Lit>()) { return lit->get<u64>() > 1; }
    return false;
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

    std::stringstream ss;
    ss << def << " : " << def->type() << " [" << def->node_name() << "]";
    std::string str = ss.str();
    // world().DLOG("rewrite_ {} : {} [{}]", def, def->type(), def->node_name());

    // Var should be handled by memoization
    // if (def->isa<Var>() || def->isa<Axiom>()) { return def; }
    if (def->isa<Axiom>()) { return def; }

    if (def->isa<Var>()) { world().DLOG("Var: {}", def); }
    assert(!def->isa<Var>());

    // if (def->type()->isa<Sigma>()) {
    //     world().DLOG("flatten {} : {}", def, def->type());
    //     auto num = def->type()->num_projs();
    //     world().DLOG("num_projs: {}", num);
    //     DefArray projs(num, [&](int i) { return rewrite(def->proj(num, i)); });
    //     return world().tuple(projs);
    // }
    // if (def->isa<Var>()) {
    //     DefArray projs(def->projs(), [this](const Def* def) { return rewrite(def); });
    //     return world().tuple(projs);
    // }

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
        // if (lam->is_set()) { new_lam = convert(lam)->as_nom<Lam>(); }

        // old2new_[def] = new_lam;
        // if (lam->is_set()) {
        //     auto new_body = rewrite(lam->body());
        //     new_lam->set_body(new_body);
        // }

        world().DLOG("rewrite lam {} : {}", def, def->type());
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

    w.DLOG("Reshape: {} : {}", def, pi_ty);
    w.DLOG("     to: {} : {}", new_lam, new_ty);

    // We associate the arguments (reshape the old vars).
    // Alternatively, we could use beta reduction (reduce) to do this for us.

    // auto arg     = reshape(wrapper->var(), def_ty);
    // auto arg       = reshape(def->var(), new_ty->dom());

    // auto arg       = reshape(def->var());
    // auto num_projs = arg->num_projs();
    auto new_arg = new_lam->var();
    // assert(num_projs == new_arg->num_projs() && "Reshape of lambda should agree with tuple reshape");

    // old2new_[arg] = new_arg;
    // for (unsigned int i = 0; i < num_projs; i++) {
    //     w.DLOG("associate {} with {}", arg->proj(i), new_arg->proj(i));
    //     old2new_[arg->proj(i)] = new_arg->proj(i);
    // }
    // TODO: deep associate def->var() with new_arg
    // def->var with tuple(...) (reconstructed shape)
    // idea: first make new_arg into "atomar" def list, then recrusively imitate def->var
    auto reformed_new_arg = reshape(new_arg, def->var()->type()); // def->var()->type() = pi_ty
    w.DLOG("var {} : {}", def->var(), def->var()->type());
    w.DLOG("new var {} : {}", new_arg, new_arg->type());
    w.DLOG("reshaped new_var {} : {}", reformed_new_arg, reformed_new_arg->type());
    w.DLOG("{}", def->var()->type());
    w.DLOG("{}", reformed_new_arg->type());
    old2new_[def->var()] = reformed_new_arg;
    // TODO: why is this necessary?
    old2new_[new_arg] = new_arg;

    auto new_body = rewrite(def->body());
    new_lam->set_body(new_body);
    new_lam->set_filter(true);

    if (def->is_external()) {
        new_lam->make_external();
        def->make_internal();
    }

    w.DLOG("finished transforming: {} : {}", new_lam, new_ty);
    return new_lam;
}

std::vector<const Def*> flatten_ty(const Def* T) {
    std::vector<const Def*> types;
    if (should_flatten(T)) {
        for (auto P : T->projs()) {
            auto inner_types = flatten_ty(P);
            types.insert(types.end(), inner_types.begin(), inner_types.end());
        }
    } else {
        types.push_back(T);
    }
    return types;
}

bool is_mem_ty(const Def* T) { return match<mem::M>(T); }
DefArray vec2array(const std::vector<const Def*>& vec) { return DefArray(vec.begin(), vec.end()); }

const Def* Reshape::reshape_type(const Def* T) {
    auto& w = T->world();

    if (auto pi = T->isa<Pi>()) {
        auto new_dom = reshape_type(pi->dom());
        auto new_cod = reshape_type(pi->codom());
        return w.pi(new_dom, new_cod);
    } else if (auto sigma = T->isa<Sigma>()) {
        auto new_types = flatten_ty(sigma);
        if (mode_ == Mode::Flat) {
            return w.sigma(vec2array(new_types));
        } else {
            if (new_types.size() == 0) return w.sigma();
            if (new_types.size() == 1) return new_types[0];
            const Def* mem = nullptr;
            const Def* ret = nullptr;
            // find mem, erase all mems
            for (auto i = new_types.begin(); i != new_types.end(); i++) {
                if (is_mem_ty(*i)) {
                    if (!mem) mem = *i;
                    new_types.erase(i);
                }
            }
            // TODO: more fine-grained test
            if (new_types.back()->isa<Pi>()) {
                ret = new_types.back();
                new_types.pop_back();
            }
            // create form [[mem,args],ret]
            const Def* args = w.sigma(vec2array(new_types));
            if (mem) { args = w.sigma({mem, args}); }
            if (ret) { args = w.sigma({args, ret}); }
            return args;
        }
    } else {
        return T;
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

const Def* Reshape::reshape(std::vector<const Def*>& defs, const Def* T) {
    if (should_flatten(T)) {
        DefArray tuples(T->projs(), [&](auto P) { return reshape(defs, P); });
        return T->world().tuple(tuples);
    } else {
        assert(defs.size() > 0 && "Reshape: not enough arguments");
        auto def = defs.front();
        defs.erase(defs.begin());
        assert(def->type() == T && "Reshape: argument type mismatch");
        return def;
    }
}

const Def* Reshape::reshape(const Def* def, const Def* target) {
    auto flat_defs = flatten_def(def);
    return reshape(flat_defs, target);
}

// called for new lambda arguments, app arguments
// We can not (directly) replace it with the more general version above due to the mem erasure.
// TODO: ignore mem erase, replace with more general
// TODO: capture names
const Def* Reshape::reshape(const Def* def) {
    auto& w = def->world();

    auto flat_defs = flatten_def(def);
    if (flat_defs.size() == 1) return flat_defs[0];
    if (mode_ == Mode::Flat) {
        return w.tuple(vec2array(flat_defs));
    } else {
        // arg style
        // [[mem,args],ret]
        const Def* mem = nullptr;
        const Def* ret = nullptr;
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
