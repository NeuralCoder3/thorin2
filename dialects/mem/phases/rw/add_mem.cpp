#include "add_mem.h"

#include <memory>

#include "thorin/analyses/schedule.h"

#include "dialects/mem/mem.h"

namespace thorin::mem {

static std::pair<const App*, Array<Lam*>> isa_apped_nom_lam_in_tuple(const Def* def) {
    if (auto app = def->isa<App>()) {
        std::vector<Lam*> lams;
        std::deque<const Def*> wl;
        wl.push_back(app->callee());
        while (!wl.empty()) {
            auto elem = wl.front();
            wl.pop_front();
            if (auto nom = elem->isa_nom<Lam>()) {
                lams.push_back(nom);
            } else if (auto extract = elem->isa<Extract>()) {
                if (auto tuple = extract->tuple()->isa<Tuple>()) {
                    for (auto&& op : tuple->ops()) wl.push_back(op);
                } else {
                    return {nullptr, {}};
                }
            } else {
                return {nullptr, {}};
            }
        }
        return {app, lams};
    }
    return {nullptr, {}};
}

// @pre isa_apped_nom_lam_in_tuple(def) valid
template<class F, class H>
static const Def* rewrite_nom_lam_in_tuple(const Def* def, F&& rewrite, H&& rewrite_idx) {
    auto& w = def->world();
    if (auto nom = def->isa_nom<Lam>()) { return std::forward<F>(rewrite)(nom); }

    auto extract = def->as<Extract>();
    auto tuple   = extract->tuple()->as<Tuple>();
    DefArray new_ops{tuple->ops(), [&](const Def* op) {
                         return rewrite_nom_lam_in_tuple(op, std::forward<F>(rewrite), std::forward<H>(rewrite_idx));
                     }};
    return w.extract(w.tuple(new_ops, tuple->dbg()), rewrite_idx(extract->index()), extract->dbg());
}

// @pre isa_apped_nom_lam_in_tuple(def) valid
template<class RewriteCallee, class RewriteArg, class RewriteIdx>
static const Def* rewrite_apped_nom_lam_in_tuple(const Def* def,
                                                 RewriteCallee&& rewrite_callee,
                                                 RewriteArg&& rewrite_arg,
                                                 RewriteIdx&& rewrite_idx) {
    auto& w     = def->world();
    auto app    = def->as<App>();
    auto callee = rewrite_nom_lam_in_tuple(app->callee(), std::forward<RewriteCallee>(rewrite_callee),
                                           std::forward<RewriteIdx>(rewrite_idx));
    auto arg    = std::forward<RewriteArg>(rewrite_arg)(app->arg());
    return app->rebuild(w, app->type(), {callee, arg}, app->dbg());
}

static const Def* follow_mem(const Def2Def& val2mem, const Def* mem) {
    auto it = val2mem.find(mem);
    while (it != val2mem.end()) {
        mem = it->second;
        it  = val2mem.find(mem);
    }
    return mem;
}

void AddMem::start() {
    for (const auto& [_, nom] : world().externals()) {
        Scope scope{nom};
        sched_.push_back(Scheduler{scope});
        curr_external_ = nom->as_nom<Lam>();
        rewrite(nom)->as_nom()->make_external();
        sched_.pop_back();
    }
}

const Def* AddMem::add_mem_to_lams(Lam* curr_lam, const Def* def) {
    auto place   = static_cast<Lam*>(sched().smart(def));
    auto mem_var = [&](Lam* nom) -> const Def* {
        world().DLOG("get mem_var for {}", nom);
        if (nom->num_vars() > 0)
            if (auto mem_var = mem::mem_var(nom)) return mem_var;
        if (auto it = val2mem_.find(nom); it != val2mem_.end()) return it->second;
        unreachable();
        return nullptr;
    };

    world().DLOG("rewriting {} in {}", def, place);

    if (auto nom_lam = def->isa_nom<Lam>(); nom_lam && !nom_lam->is_set()) return def;
    if (auto it = mem_rewritten_.find(def); it != mem_rewritten_.end()) {
        auto tmp = it->second;
        if (match<mem::M>(def->type())) {
            world().DLOG("already known mem {} in {}", def, curr_lam);
            return follow_mem(val2mem_, mem_var(curr_lam));
        }
        return tmp;
    }
    if (match<mem::M>(def->type())) { world().DLOG("new mem {} in {}", def, curr_lam); }

    // todo: do we need to stub noms ..?
    if (auto nom = def->isa_nom<Lam>()) {
        mem_rewritten_[nom] = nom;
        for (size_t i = 0, n = nom->num_ops(); i < n; ++i) {
            static_cast<Def*>(nom)->set(i, add_mem_to_lams(nom, nom->op(i)));
        }
        return nom;
    }
    assert(!def->isa_nom());

    if (auto apped_nom = isa_apped_nom_lam_in_tuple(def); apped_nom.first) {
        auto rewrite_arg = [&](const Def* arg) -> const Def* {
            if (arg->type()->num_projs() > 0 && match<mem::M>(arg->type()->proj(0)))
                return arg->rebuild(arg->world(), arg->type(),
                                    DefArray{arg->ops(), [&](const Def* op) { return add_mem_to_lams(place, op); }},
                                    arg->dbg());

            // return arg->rebuild(arg->world(), arg->type(),
            // DefArray{arg->ops(),
            //          [&](const Def* op) {
            //              if (match<mem::M>(op->type()))
            //                  return add_mem_to_lams(place,
            //                                         follow_mem(val2mem_, mem_var(place)));
            //              return add_mem_to_lams(place, op);
            //          }},
            // arg->dbg());

            if (arg->isa<Tuple>() || arg->isa<Arr>() || arg->isa<Pack>()) {
                DefArray new_args{arg->num_projs() + 1};
                for (int i = arg->num_projs(); i >= 0; i--) {
                    new_args[i] = i == 0 ? add_mem_to_lams(place, follow_mem(val2mem_, mem_var(place)))
                                         : add_mem_to_lams(place, arg->proj(i - 1));
                }
                return arg->world().tuple(new_args, arg->dbg());
            }

            return arg->world().tuple(
                {add_mem_to_lams(place, follow_mem(val2mem_, mem_var(place))), add_mem_to_lams(place, arg)},
                arg->dbg());
        };

        auto rewrite_lam = [&](Lam* nom) -> const Def* {
            if (auto it = mem_rewritten_.find(nom); it != mem_rewritten_.end()) // already rewritten
                if (auto pi = it->second->type()->as<Pi>(); pi->num_doms() > 0 && match<mem::M>(pi->dom(0_s)))
                    return it->second;
            bool is_bound = sched().scope().bound(nom);
            auto deleter  = [this, is_bound](Scope* scp) {
                if (!is_bound) sched_.pop_back();
                delete scp;
            };
            std::unique_ptr<Scope, decltype(deleter)> bound_scope{nullptr, deleter};
            if (!is_bound) {
                world().DLOG("free lam {}", nom);
                bound_scope.reset(new Scope{nom});
                sched_.emplace_back(*bound_scope);
            }

            if (auto pi = nom->type()->as<Pi>();
                pi->num_doms() > 0 && match<mem::M>(pi->dom(0_s))) { // already has a mem
                                                                     // todo: deal with unbound lams...
                if (!is_bound) return add_mem_to_lams(nom, nom);
                return add_mem_to_lams(place, nom);
            }

            auto pi            = nom->type()->as<Pi>();
            auto dom           = pi->dom();
            const Def* new_dom = nullptr;
            if (dom->isa<Sigma>() || dom->isa<Arr>() || dom->isa<Pack>()) { // what about packs / arrays..?
                new_dom = world().sigma(DefArray{dom->num_projs() + 1, [&](size_t i) {
                                                     return i == 0 ? mem::type_mem(world()) : dom->proj(i - 1);
                                                 }});
            } else {
                new_dom = world().sigma({mem::type_mem(world()), dom});
            }

            auto new_nom = nom->stub(world(), world().pi(new_dom, pi->codom()), nom->dbg());

            for (size_t i = 0; i < nom->num_vars(); ++i) mem_rewritten_[nom->var(i)] = new_nom->var(i + 1);
            if (nom->num_vars() > 1) mem_rewritten_[nom->var()] = new_nom->var();

            mem_rewritten_[new_nom] = new_nom;
            mem_rewritten_[nom]     = new_nom;
            val2mem_[new_nom]       = new_nom->var(0_s);
            val2mem_[nom]           = new_nom->var(0_s);

            new_nom->set(add_mem_to_lams(place, nom->filter()), add_mem_to_lams(place, nom->body()));

            return new_nom;
        };

        return mem_rewritten_[def] =
                   rewrite_apped_nom_lam_in_tuple(def, rewrite_lam, std::move(rewrite_arg),
                                                  [&](const Def* def) { return add_mem_to_lams(place, def); });
    }

    DefArray new_ops{def->ops(), [&](const Def* op) { return add_mem_to_lams(place, op); }};

    auto tmp = mem_rewritten_[def] = def->rebuild(world(), def->type(), new_ops, def->dbg());
    return tmp;
}

const Def* AddMem::rewrite(const Def* def) { return add_mem_to_lams(curr_external_, def); }

} // namespace thorin::mem
