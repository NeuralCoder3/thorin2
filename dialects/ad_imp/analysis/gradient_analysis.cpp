#include "dialects/ad_imp/analysis/gradient_analysis.h"

#include "thorin/def.h"
#include "thorin/tuple.h"

#include "dialects/ad_imp/analysis/analysis.h"
#include "dialects/ad_imp/analysis/analysis_factory.h"
#include "dialects/mem/mem.h"

namespace thorin::ad_imp {

bool GradientLattice::set(Type type) {
    Type prev = type_;
    type_     = type_ | type;
    return type_ != prev;
}

GradientAnalysis::GradientAnalysis(AnalysisFactory& factory)
    : Analysis(factory) {}

GradientLattice& GradientAnalysis::get_lattice(const Def* def) {
    auto it = lattices.find(def);
    if (it == lattices.end()) { it = lattices.emplace(def, std::make_unique<GradientLattice>(def)).first; }
    return *it->second;
}

DefSet& GradientAnalysis::defs() {
    lazy_run();
    return gradient_set;
}

bool GradientAnalysis::has_gradient(const Def* def) {
    lazy_run();
    return gradient_set.contains(def);
}

bool GradientAnalysis::is_const(const Def* def) {
    if (def->isa<Lit>()) {
        return true;
    } else if (auto app = def->isa<App>()) {
        return is_const(app->arg());
    } else if (auto tuple = def->isa<Tuple>()) {
        for (auto op : tuple->ops()) {
            if (!is_const(op)) { return false; }
        }

        return true;
    } else if (auto pack = def->isa<Pack>()) {
        return is_const(pack->body());
    }

    return false;
}

void GradientAnalysis::meet(GradientLattice& present, GradientLattice& next) {
    todo_ |= present.set(next.type() & GradientLattice::Has);
    todo_ |= next.set(present.type() & GradientLattice::Required);
}

void GradientAnalysis::meet(const Def* present, const Def* next) { meet(get_lattice(present), get_lattice(next)); }

void GradientAnalysis::meet_projs(const Def* present, const Def* next) {
    if (present->num_projs() > 1) {
        for (size_t i = 0; i < present->num_projs(); i++) { meet_projs(present->proj(i), next->proj(i)); }
    } else {
        meet(present, next);
    }
}

void GradientAnalysis::visit(const Def* def) {
    if (auto tuple = def->isa<Tuple>()) {
        for (auto proj : tuple->ops()) {
            if (match<mem::M>(proj->type())) continue;
            meet(proj, tuple);
        }
    } else if (auto pack = def->isa<Pack>()) {
        meet(pack->body(), pack);
    } else if (auto app = def->isa<App>()) {
        auto arg = app->arg();
        if (match<math::arith>(app) || match<math::extrema>(app) || match<math::exp>(app) || match<math::gamma>(app) ||
            match<math::tri>(app) || match<math::rt>(app) || match<math::conv>(app) || match<core::conv>(app)) {
            meet(arg, app);
        } else if (auto lea = match<mem::lea>(app)) {
            auto arr = arg->proj(0);
            meet(lea, arr);
            meet(arr, lea);
        } else if (auto store = match<mem::store>(app)) {
            auto ptr = arg->proj(1);
            auto val = arg->proj(2);
            meet(val, ptr);
        } else if (auto load = match<mem::load>(app)) {
            auto val = load->proj(1);
            auto ptr = arg->proj(1);
            meet(ptr, val);
        } else if (auto bitcast = match<core::bitcast>(app)) {
            auto ptr = bitcast->arg();
            meet(ptr, bitcast);
            meet(bitcast, ptr);
        }
    }
}

void GradientAnalysis::meet_app(const Def* arg, AffineCFNode* node) {
    for (auto succ : node->succs()) {
        if (auto lam = succ->def()->isa_nom<Lam>()) {
            const Def* target = lam->var();
            if (factory().utils().is_loop_body_var(lam->var())) { target = target->proj(1); }
            meet_projs(arg, target);
        } else {
            meet_app(arg, succ);
        }
    }
}

void GradientAnalysis::require(const Def* def) { todo_ |= get_lattice(def).set(GradientLattice::Required); }

void GradientAnalysis::has(const Def* def) { todo_ |= get_lattice(def).set(GradientLattice::Has); }

void GradientAnalysis::run() {
    auto lam = factory().lam();
    /*for (auto var : lam->vars()) {
        if (match<mem::M>(var->type())) continue;

        // gradients of all input arguments are required
        require(var);

        // we have the gradients of all input pointers
        if (match<mem::Ptr>(var->type())) { has(var); }
    }

    { // ret arguments get gradients
        auto& cfa     = factory().cfa();
        auto ret_node = cfa.node(lam->ret_var());
        auto ret_wrap = ret_node->pred();
        auto ret_app  = ret_wrap->def()->as_nom<Lam>()->body()->as<App>();
        auto ret_arg  = ret_app->arg();

        for (auto proj : ret_arg->projs()) {
            if (match<mem::M>(proj->type())) continue;
            has(proj);
        }
    }*/

    auto& utils = factory().utils();
    auto& cfa   = factory().cfa();
    auto& dfa   = factory().dfa();

    todo_ = true;
    while (todo_) {
        todo_ = false;

        for (auto lam : utils.lams()) {
            for (auto def : utils.scope(lam).bound()) { visit(def); }

            auto node = cfa.node(lam);
            auto arg  = thorin::ad_imp::arg(lam->body()->as<App>());
            meet_app(arg, node);
        }
    }

    for (auto& [def, lattice] : lattices) {
        if (lattice->type() == GradientLattice::Top) {
            def->dump();
            gradient_set.insert(def);
        }
    }
}

} // namespace thorin::ad_imp
