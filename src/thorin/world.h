#ifndef THORIN_WORLD_H
#define THORIN_WORLD_H

#include <cassert>
#include <iostream>
#include <functional>
#include <initializer_list>
#include <string>
#include <variant>

#include "thorin/enums.h"
#include "thorin/primop.h"
#include "thorin/util/hash.h"
#include "thorin/util/stream.h"
#include "thorin/config.h"

namespace thorin {

using Name = std::variant<const char*, const Def*>;

class Dbg {
public:
    Dbg()
        : loc()
        , name((const Def*) nullptr)
    {}
    Dbg(Debug debug)
        : loc(debug.loc())
        , name(debug.name())
    {}
    Dbg(Loc loc)
        : loc(loc)
        , name((const Def*) nullptr)
    {}
    Dbg(Loc loc, Name name)
        : loc(loc)
        , name(name)
    {}
    Dbg(Name name)
        : loc()
        , name(name)
    {}

    Loc loc;
    Name name;
};

/**
 * The World represents the whole program and manages creation of Thorin nodes (Def%s).
 * In particular, the following things are done by this class:
 *
 *  - @p Def unification: \n
 *      There exists only one unique @p Def.
 *      These @p Def%s are hashed into an internal map for fast access.
 *      The getters just calculate a hash and lookup the @p Def, if it is already present, or create a new one otherwise.
 *      This is corresponds to value numbering.
 *  - constant folding
 *  - canonicalization of expressions
 *  - several local optimizations like <tt>x + 0 -> x</tt>
 *
 *  Use @p cleanup to remove dead and unreachable code.
 *
 *  You can create several worlds.
 *  All worlds are completely independent from each other.
 *
 *  Note that types are also just @p Def%s and will be hashed as well.
 */
class World : public Streamable {
public:
    struct DefHash {
        static uint64_t hash(const Def* def) { return def->hash(); }
        static bool eq(const Def* def1, const Def* def2) { return def1->equal(def2); }
        static const Def* sentinel() { return (const Def*)(1); }
    };

    struct BreakHash {
        static uint64_t hash(size_t i) { return i; }
        static bool eq(size_t i1, size_t i2) { return i1 == i2; }
        static size_t sentinel() { return size_t(-1); }
    };

    using Sea         = HashSet<const Def*, DefHash>;///< This @p HashSet contains Thorin's "sea of nodes".
    using Breakpoints = HashSet<size_t, BreakHash>;
    using Externals   = GIDSet<Def*>;

    World(const World&) = delete;
    World(World&&) = delete;
    World& operator=(const World&) = delete;

    explicit World(uint32_t cur_gid, const char* name = nullptr, Loc loc = {});
    World(World& other)
        : World(other.cur_gid(), other.name(), other.loc())
    {
        pe_done_ = other.pe_done_;
#if THORIN_ENABLE_CHECKS
        track_history_ = other.track_history_;
        breakpoints_   = other.breakpoints_;
#endif
    }
    ~World();

    // getters
    const char* name() const { return name_; }
    Loc loc() const { return loc_; }
    const Sea& defs() const { return defs_; }
    std::vector<Lam*> copy_lams() const;

    /// @name manage global identifier - a unique number for each Def
    //@{
    uint32_t cur_gid() const { return cur_gid_; }
    uint32_t next_gid() { return ++cur_gid_; }
    //@}
    /// @name Universe and Kind
    //@{
    const Universe* universe() { return cache_.universe_; }
    const Kind* kind(NodeTag tag) { return cache_.kinds_[(size_t) tag - (size_t) Node_KindArity]; }
    const Kind* kind_arity() { return cache_.kind_.kind_arity_; }
    const Kind* kind_multi() { return cache_.kind_.kind_multi_; }
    const Kind* kind_star()  { return cache_.kind_.kind_star_; }
    //@}
    /// @name Param
    //@{
    const Param* param(Lam* lam, Dbg dbg = {}) { return unify<Param>(1, lam->domain(), lam, debug(dbg)); }
    //@}
    /// @name Pi
    //@{
    const Pi* pi(Defs domain, const Def* codomain, Dbg dbg = {}) { return pi(sigma(domain), codomain, dbg); }
    const Pi* pi(const Def* domain, const Def* codomain, Dbg dbg = {});
    //@}
    /// @name Pi: continuation type, i.e., Pi type with codomain Bottom
    //@{
    const Pi* cn() { return cn(sigma()); }
    const Pi* cn(Defs domains) { return cn(sigma(domains)); }
    const Pi* cn(const Def* domain) { return pi(domain, bot_star()); }
    //@}
    /// @name Lambda: nominal
    //@{
    Lam* lam(const Pi* cn, CC cc = CC::C, Intrinsic intrinsic = Intrinsic::None, Dbg dbg = {}) {
        auto lam = insert<Lam>(2, cn, cc, intrinsic, debug(dbg));
        lam->destroy(); // set filter to false and body to top
        return lam;
    }
    Lam* lam(const Pi* cn, Dbg dbg = {}) { return lam(cn, CC::C, Intrinsic::None, dbg); }
    //@}
    /// @name Lambda: structural
    const Lam* lam(const Def* domain, const Def* filter, const Def* body, Dbg dbg);
    const Lam* lam(const Def* domain, const Def* body, Dbg dbg) { return lam(domain, lit_true(), body, dbg); }
    //@}
    /// @name App
    //@{
    const Def* app(const Def* callee, const Def* op, Dbg dbg = {});
    const Def* app(const Def* callee, Defs ops, Dbg dbg = {}) { return app(callee, tuple(ops), dbg); }
    //@}
    /// @name Sigma: structural
    //@{
    const Def* sigma(const Def* type, Defs ops, Dbg dbg = {});
    /// a @em structural @p Sigma of type @p star
    const Def* sigma(Defs ops, Dbg dbg = {}) { return sigma(kind_star(), ops, dbg); }
    const Sigma* sigma() { return cache_.sigma_; } ///< the unit type within @p kind_star()
    //@}
    /// @name Sigma: nominal
    //@{
    Sigma* sigma(const Def* type, size_t size, Dbg dbg = {}) { return insert<Sigma>(size, type, size, debug(dbg)); }
    Sigma* sigma(size_t size, Dbg dbg = {}) { return sigma(kind_star(), size, dbg); } ///< a @em nominal @p Sigma of type @p star
    //@}
    /// @name Variadic
    //@{
    const Def* variadic(const Def* arity, const Def* body, Dbg dbg = {});
    const Def* variadic(Defs arities, const Def* body, Dbg dbg = {});
    const Def* variadic(u64 a, const Def* body, Dbg dbg = {}) { return variadic(lit_arity(a, dbg), body, dbg); }
    const Def* variadic(ArrayRef<u64> a, const Def* body, Dbg dbg = {}) {
        return variadic(Array<const Def*>(a.size(), [&](size_t i) { return lit_arity(a[i], dbg); }), body, dbg);
    }
    const Def* unsafe_variadic(const Def* body, Dbg dbg = {}) { return variadic(top_arity(), body, dbg); }
    //@}
    /// @name Tuple
    //@{
    /// ascribes @p type to this tuple - needed for dependetly typed and structural @p Sigma%s
    const Def* tuple(const Def* type, Defs ops, Dbg dbg = {});
    const Def* tuple(Defs ops, Dbg dbg = {});
    const Def* tuple_str(const char* s, Dbg = {});
    const Def* tuple_str(const std::string& s, Dbg dbg = {}) { return tuple_str(s.c_str(), dbg); }
    const Tuple* tuple() { return cache_.tuple_; } ///< the unit value of type <tt>[]</tt>
    //@}
    /// @name Pack
    //@{
    const Def* pack(const Def* arity, const Def* body, Dbg dbg = {});
    const Def* pack(Defs arities, const Def* body, Dbg dbg = {});
    const Def* pack(u64 a, const Def* body, Dbg dbg = {}) { return pack(lit_arity(a, dbg), body, dbg); }
    const Def* pack(ArrayRef<u64> a, const Def* body, Dbg dbg = {}) {
        return pack(Array<const Def*>(a.size(), [&](auto i) { return lit_arity(a[i], dbg); }), body, dbg);
    }
    //@}
    /// @name Extract
    //@{
    const Def* extract(const Def* agg, const Def* i, Dbg dbg = {});
    const Def* extract(const Def* agg, u32 i, Dbg dbg = {}) { return extract(agg, lit_index(agg->arity(), i, dbg), dbg); }
    const Def* extract(const Def* agg, u32 a, u32 i, Dbg dbg = {}) { return extract(agg, lit_index(a, i, dbg), dbg); }
    const Def* unsafe_extract(const Def* agg, const Def* i, Dbg dbg = {}) { return extract(agg, cast(agg->arity(), i, dbg), dbg); }
    const Def* unsafe_extract(const Def* agg, u64 i, Dbg dbg = {}) { return unsafe_extract(agg, lit_qu64(i, dbg), dbg); }
    //@}
    /// @name Insert
    //@{
    const Def* insert(const Def* agg, const Def* i, const Def* value, Dbg dbg = {});
    const Def* insert(const Def* agg, u32 i, const Def* value, Dbg dbg = {}) { return insert(agg, lit_index(agg->arity(), i, dbg), value, dbg); }
    const Def* unsafe_insert(const Def* agg, const Def* i, const Def* value, Dbg dbg = {}) { return insert(agg, cast(agg->arity(), i, dbg), value, dbg); }
    const Def* unsafe_insert(const Def* agg, u32 i, const Def* value, Dbg dbg = {}) { return unsafe_insert(agg, lit_qu64(i, dbg), value, dbg); }
    //@}
    /// @name LEA - load effective address
    //@{
    const Def* lea(const Def* ptr, const Def* index, Dbg dbg);
    const Def* unsafe_lea(const Def* ptr, const Def* index, Dbg dbg) { return lea(ptr, cast(ptr->type()->as<PtrType>()->pointee()->arity(), index, dbg), dbg); }
    //@}
    /// @name Literal
    //@{
    const Lit* lit(const Def* type, Box box, Dbg dbg = {}) { return unify<Lit>(0, type, box, debug(dbg)); }
    const Lit* lit(PrimTypeTag tag, Box box, Dbg dbg = {}) { return lit(type(tag), box, dbg); }
    //@}
    /// @name Literal: Arity - note that this is a type
    //@{
    const Lit* lit_arity(u64 a, Dbg dbg = {}) { return lit(kind_arity(), {a}, dbg); }
    const Lit* lit_arity_1() { return cache_.lit_arity_1_; } ///< unit arity 1ₐ
    //@}
    /// @name Literal: Index - the inhabitants of an Arity
    //@{
    const Lit* lit_index(u64 arity, u64 idx, Dbg dbg = {}) { return lit_index(lit_arity(arity), idx, dbg); }
    const Lit* lit_index(const Def* arity, u64 index, Dbg dbg = {});
    const Lit* lit_index_0_1() { return cache_.lit_index_0_1_; } ///< unit index 0₁ of type unit arity 1ₐ
    //@}
    /// @name Literal: Nat
    //@{
    const Lit* lit_nat(int64_t val, Dbg dbg = {}) { return lit(type_nat(), {val}, dbg); }
    const Lit* lit_nat_0 () { return cache_.lit_nat_0_; }
    const Lit* lit_nat_1 () { return cache_.lit_nat_[0]; }
    const Lit* lit_nat_2 () { return cache_.lit_nat_[1]; }
    const Lit* lit_nat_4 () { return cache_.lit_nat_[2]; }
    const Lit* lit_nat_8 () { return cache_.lit_nat_[3]; }
    const Lit* lit_nat_16() { return cache_.lit_nat_[4]; }
    const Lit* lit_nat_32() { return cache_.lit_nat_[5]; }
    const Lit* lit_nat_64() { return cache_.lit_nat_[6]; }
    //@}
    /// @name Literal: Bool
    //@{
    const Lit* lit_bool(bool val) { return cache_.lit_bool_[size_t(val)]; }
    const Lit* lit_false() { return cache_.lit_bool_[0]; }
    const Lit* lit_true()  { return cache_.lit_bool_[1]; }
    //@}
    /// @name Literal: PrimTypes
    //@{
#define THORIN_ALL_TYPE(T, M) \
    const Def* lit_##T(T val, Dbg dbg = {}) { return lit(PrimType_##T, Box(val), dbg); }
#include "thorin/tables/primtypetable.h"
    const Lit* lit_zero(PrimTypeTag tag, Dbg dbg = {}) { return lit(tag, 0, dbg); }
    const Lit* lit_zero(const Def* type, Dbg dbg = {}) { return lit_zero(type->as<PrimType>()->primtype_tag(), dbg); }
    const Lit* lit_one(PrimTypeTag tag, Dbg dbg = {}) { return lit(tag, 1, dbg); }
    const Lit* lit_one(const Def* type, Dbg dbg = {}) { return lit_one(type->as<PrimType>()->primtype_tag(), dbg); }
    const Lit* lit_allset(PrimTypeTag tag, Dbg dbg = {});
    const Lit* lit_allset(const Def* type, Dbg dbg = {}) { return lit_allset(type->as<PrimType>()->primtype_tag(), dbg); }
    //@}
    /// @name Top/Bottom
    //@{
    const Def* bot_top(bool is_top, const Def* type, Dbg dbg = {});
    const Def* bot(const Def* type, Dbg dbg = {}) { return bot_top(false, type, dbg); }
    const Def* top(const Def* type, Dbg dbg = {}) { return bot_top(true,  type, dbg); }
    const Def* bot(PrimTypeTag tag, Dbg dbg = {}) { return bot_top(false, type(tag), dbg); }
    const Def* top(PrimTypeTag tag, Dbg dbg = {}) { return bot_top( true, type(tag), dbg); }
    const Def* bot_star () { return cache_.bot_star_; }
    const Def* top_arity() { return cache_.top_arity_; } ///< use this guy to encode an unknown arity, e.g., for unsafe arrays
    //@}
    /// @name Variant
    //@{
    const VariantType* variant_type(Defs ops, Dbg dbg = {}) { return unify<VariantType>(ops.size(), kind_star(), ops, debug(dbg)); }
    const Def* variant(const VariantType* variant_type, const Def* value, Dbg dbg = {}) { return unify<Variant>(1, variant_type, value, debug(dbg)); }
    //@}
    /// @name misc types
    //@{
#define THORIN_ALL_TYPE(T, M) \
    const PrimType* type_##T() { return type(PrimType_##T); }
#include "thorin/tables/primtypetable.h"
    const PrimType* type(PrimTypeTag tag) {
        size_t i = tag - Begin_PrimType;
        assert(i < (size_t) Num_PrimTypes);
        return cache_.primtypes_[i];
    }
    const MemType* mem_type() const { return cache_.mem_; }
    const PtrType* ptr_type(const Def* pointee, AddrSpace addr_space = AddrSpace::Generic, Dbg dbg = {}) { return unify<PtrType>(1, kind_star(), pointee, addr_space, debug(dbg)); }
    //@}
    /// @name ArithOps
    //@{
    /// Creates an \p ArithOp or a \p Cmp.
    const Def* binop(int tag, const Def* lhs, const Def* rhs, Dbg dbg = {});
    const Def* arithop_not(const Def* def, Dbg dbg = {});
    const Def* arithop_minus(const Def* def, Dbg dbg = {});
    const Def* arithop(ArithOpTag tag, const Def* lhs, const Def* rhs, Dbg dbg = {});
#define THORIN_ARITHOP(OP) \
    const Def* arithop_##OP(const Def* lhs, const Def* rhs, Dbg dbg = {}) { \
        return arithop(ArithOp_##OP, lhs, rhs, dbg); \
    }
#include "thorin/tables/arithoptable.h"
    //@}
    /// @name Cmps
    //@{
    const Def* cmp(CmpTag tag, const Def* lhs, const Def* rhs, Dbg dbg = {});
#define THORIN_CMP(OP) \
    const Def* cmp_##OP(const Def* lhs, const Def* rhs, Dbg dbg = {}) { \
        return cmp(Cmp_##OP, lhs, rhs, dbg);  \
    }
#include "thorin/tables/cmptable.h"
    //@}
    /// @name Casts
    //@{
    const Def* convert(const Def* to, const Def* from, Dbg dbg = {});
    const Def* cast(const Def* to, const Def* from, Dbg dbg = {});
    const Def* bitcast(const Def* to, const Def* from, Dbg dbg = {});
    //@}
    /// @name memory-related operations
    //@{
    const Def* load(const Def* mem, const Def* ptr, Dbg dbg = {});
    const Def* store(const Def* mem, const Def* ptr, const Def* val, Dbg dbg = {});
    const Slot* slot(const Def* type, const Def* mem, Dbg dbg = {});
    const Alloc* alloc(const Def* type, const Def* mem, Dbg dbg = {});
    const Def* global(const Def* init, bool is_mutable = true, Dbg dbg = {});
    const Def* global_immutable_string(const std::string& str, Dbg dbg = {});
    const Assembly* assembly(const Def* type, Defs inputs, std::string asm_template, ArrayRef<std::string> output_constraints,
                             ArrayRef<std::string> input_constraints, ArrayRef<std::string> clobbers, Assembly::Flags flags, Dbg dbg = {});
    const Assembly* assembly(Defs types, const Def* mem, Defs inputs, std::string asm_template, ArrayRef<std::string> output_constraints,
                             ArrayRef<std::string> input_constraints, ArrayRef<std::string> clobbers, Assembly::Flags flags, Dbg dbg = {});
    //@}
    /// @name partial evaluation related operations
    //@{
    const Def* hlt(const Def* def, Dbg dbg = {});
    const Def* known(const Def* def, Dbg dbg = {});
    const Def* run(const Def* def, Dbg dbg = {});
    //@}
    /// @name misc operations
    //@{
    const Analyze* analyze(const Def* type, Defs ops, u64 index, Dbg dbg = {}) { return unify<Analyze>(ops.size(), type, ops, index, debug(dbg)); }
    const Def* size_of(const Def* type, Dbg dbg = {});
    //@}
    /// @name select
    //@{
    const Def* select(const Def* cond, const Def* t, const Def* f, Dbg dbg = {});
    const Def* branch(const Def* cond, const Def* t, const Def* f, const Def* mem, Dbg dbg = {}) { return app(select(cond, t, f, dbg), mem, dbg); }
    //@}
    // TODO not all of them are axioms right now
    /// @name Axioms
    //@{
    Axiom* axiom(const Def* type, Normalizer, Dbg dbg = {});
    Axiom* axiom(const Def* type, Dbg dbg = {}) { return axiom(type, nullptr, dbg); }
    Axiom* type_nat() { return cache_.type_nat_; }
    Lam* match(const Def* type, size_t num_patterns);
    Lam* end_scope() const { return cache_.end_scope_; }
    //@}
    /// @name partial evaluation done?
    //@{
    void mark_pe_done(bool flag = true) { pe_done_ = flag; }
    bool is_pe_done() const { return pe_done_; }
    //@}
    /// @name manage externals
    //@{
    bool empty() { return externals().empty(); }
    const Externals& externals() const { return externals_; }
    void make_external(Def* def) { externals_.emplace(def); }
    void make_internal(Def* def) { externals_.erase(def); }
    bool is_external(const Def* def) { return externals().contains(const_cast<Def*>(def)); }
    //@}

#if THORIN_ENABLE_CHECKS
    /// @name debugging features
    //@{
    void breakpoint(size_t number) { breakpoints_.insert(number); }
    const Breakpoints& breakpoints() const { return breakpoints_; }
    bool track_history() const { return track_history_; }
    void enable_history(bool flag = true) { track_history_ = flag; }
    //@}
#endif

    /// @name stream
    //@{
    // Note that we don't use overloading for the following methods in order to have them accessible from gdb.
    virtual std::ostream& stream(std::ostream&) const override; ///< Streams thorin to file @p out.
    void write_thorin(const char* filename) const;              ///< Dumps thorin to file with name @p filename.
    void thorin() const;                                        ///< Dumps thorin to a file with an auto-generated file name.
    //@}

    friend void swap(World& w1, World& w2) {
        using std::swap;
        swap(w1.root_page_,     w2.root_page_);
        swap(w1.cur_page_,      w2.cur_page_);
        swap(w1.cur_gid_,       w2.cur_gid_);
        swap(w1.buffer_index_,  w2.buffer_index_);
        swap(w1.name_,          w2.name_);
        swap(w1.loc_,           w2.loc_);
        swap(w1.externals_,     w2.externals_);
        swap(w1.defs_,          w2.defs_);
        swap(w1.pe_done_,       w2.pe_done_);
        swap(w1.cache_,         w2.cache_);
#if THORIN_ENABLE_CHECKS
        swap(w1.breakpoints_,   w2.breakpoints_);
        swap(w1.track_history_, w2.track_history_);
#endif
        swap(w1.cache_.universe_->world_, w2.cache_.universe_->world_);
        assert(&w1.universe()->world() == &w1);
        assert(&w2.universe()->world() == &w2);
    }

private:
    /// @name helpers for optional/variant arguments
    //@{
    Debug debug(Dbg dbg) {
        if (std::holds_alternative<const char*>(dbg.name)) return Debug(dbg.loc, tuple_str(std::get<const char*>(dbg.name)));
        if (std::holds_alternative<const Def* >(dbg.name)) return Debug(dbg.loc, std::get<const Def*>(dbg.name));
        THORIN_UNREACHABLE;
    }
    //const Def* filter(Filter f) { return f ? *f : lit_false(); }
    //@}

    /// @name memory management and hashing
    //@{
    template<class T, class... Args>
    const T* unify(size_t num_ops, Args&&... args) {
        auto def = allocate<T>(num_ops, args...);
#ifndef NDEBUG
        if (breakpoints_.contains(def->gid())) THORIN_BREAK;
#endif
        assert(!def->isa_nominal());
        auto [i, success] = defs_.emplace(def);
        if (success) {
            def->finalize();
            return def;
        }

        deallocate<T>(def);
        return static_cast<const T*>(*i);
    }

    template<class T, class... Args>
    T* insert(size_t num_ops, Args&&... args) {
        auto def = allocate<T>(num_ops, args...);
#ifndef NDEBUG
        if (breakpoints_.contains(def->gid())) THORIN_BREAK;
#endif
        auto p = defs_.emplace(def);
        assert_unused(p.second);
        return def;
    }

    struct Zone {
        static const size_t Size = 1024 * 1024 - sizeof(std::unique_ptr<int>); // 1MB - sizeof(next)
        char buffer[Size];
        std::unique_ptr<Zone> next;
    };

#ifndef NDEBUG
    struct Lock {
        Lock() { assert((allocate_guard_ = !allocate_guard_) && "you are not allowed to recursively invoke allocate"); }
        ~Lock() { allocate_guard_ = !allocate_guard_; }
        static bool allocate_guard_;
    };
#else
    struct Lock { ~Lock() {} };
#endif

    static inline size_t align(size_t n) { return (n + (sizeof(void*) - 1)) & ~(sizeof(void*)-1); }

    template<class T> static inline size_t num_bytes_of(size_t num_ops) {
        size_t result = std::is_empty<typename T::Extra>() ? 0 : sizeof(typename T::Extra);
        result += sizeof(Def) + sizeof(const Def*)*num_ops;
        return align(result);
    }

    template<class T, class... Args>
    T* allocate(size_t num_ops, Args&&... args) {
        static_assert(sizeof(Def) == sizeof(T), "you are not allowed to introduce any additional data in subclasses of Def - use 'Extra' struct");
        Lock lock;
        size_t num_bytes = num_bytes_of<T>(num_ops);
        num_bytes = align(num_bytes);
        assert(num_bytes < Zone::Size);

        if (buffer_index_ + num_bytes >= Zone::Size) {
            auto page = new Zone;
            cur_page_->next.reset(page);
            cur_page_ = page;
            buffer_index_ = 0;
        }

        auto result = new (cur_page_->buffer + buffer_index_) T(args...);
        assert(result->num_ops() == num_ops);
        buffer_index_ += num_bytes;
        assert(buffer_index_ % alignof(T) == 0);

        return result;
    }

    template<class T>
    void deallocate(const T* def) {
        size_t num_bytes = num_bytes_of<T>(def->num_ops());
        num_bytes = align(num_bytes);
        def->~T();
        if (ptrdiff_t(buffer_index_ - num_bytes) > 0) // don't care otherwise
            buffer_index_-= num_bytes;
        assert(buffer_index_ % alignof(T) == 0);
    }
    //@}

    std::unique_ptr<Zone> root_page_;
    Zone* cur_page_;
    size_t buffer_index_ = 0;
    const char* name_;
    Loc loc_;
    Externals externals_;
    Sea defs_;
    uint32_t cur_gid_;
    bool pe_done_ = false;
#if THORIN_ENABLE_CHECKS
    bool track_history_ = false;
    Breakpoints breakpoints_;
#endif
    struct PrimTypeStruct {
#define THORIN_ALL_TYPE(T, M) const PrimType* T##_;
#include "thorin/tables/primtypetable.h"
    };

    struct Kinds {
        const Kind* kind_arity_;
        const Kind* kind_multi_;
        const Kind* kind_star_;
    };

    struct Cache {
        Universe* universe_;
        union {
            Kinds kind_;
            std::array<const Kind*, 3> kinds_;
        };
        union {
            PrimTypeStruct primtype_;
            std::array<const PrimType*, Num_PrimTypes> primtypes_;
        };
        const BotTop* bot_star_;
        const BotTop* top_arity_;
        const Sigma* sigma_;
        const Tuple* tuple_;
        const MemType* mem_;
        Axiom* type_nat_;
        const Lit* lit_nat_0_;
        std::array<const Lit*, 7> lit_nat_;
        std::array<const Lit*, 2> lit_bool_;
        const Lit* lit_arity_1_;
        const Lit* lit_index_0_1_;
        Lam* end_scope_;
    } cache_;

    friend class Cleaner;
    friend void Def::replace(Tracker) const;
};

}

#endif
