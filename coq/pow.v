Require Import Coq.Numbers.NatInt.NZDiv.
Require Import NPeano.
Require Import Lia.
Require Import Vector.
Require Import FunInd.

Import VectorNotations.

Load helper.


Definition mod_lift
    (f:nat -> nat -> nat) n:
    Idx n * Idx n -> Idx n :=
    fun '(a,b) => f a b mod n.

Definition core_wrap_mul := mod_lift mult.
Definition core_wrap_sub := mod_lift minus.
Definition core_icmp_e (n:nat) (ab:(Idx n * Idx n)) : Bool := 
  let '(a,b) := ab in
    if (Nat.eq_dec a b) then 1 else 0.
Definition core_icmp_e_fin (n:nat) (ab:(Idx n * Idx n)) : Fin.t 2 := 
  let '(a,b) := ab in
    if (Nat.eq_dec a b) then (Fin.FS Fin.F1) else Fin.F1.

(* Inductive DepTuple : forall n, vector Type n -> Type :=
| DepTuple_nil : DepTuple 0 (Vector.nil _)
| DepTuple_cons A (a:A) n (v:vector Type n) (a:A):
    DepTuple n v -> DepTuple (S n) (Vector.cons _ A n v). *)

(* Hypothesis (IdxInj: forall n, Idx n -> Fin.t n).     *)

(* Check Vector.t. *)
(* Definition proj 
  {n:nat} {A:vector Type n}
  (t:DepTuple n A) (i:Idx n) : Vector.nth A (IdxInj _ i).
Admitted. *)

(* Definition heterogen_vector := 
  vector {T:Type & T}. *)

(* Definition proj 
  {n:nat} 
  (t:heterogen_vector n) (i:Idx n) : projT1 (Vector.nth t (IdxInj _ i)) :=
    projT2 (Vector.nth t (IdxInj _ i)).

Definition proj_fin
  {n:nat} 
  (t:heterogen_vector n) (i:Fin.t n) : projT1 (Vector.nth t i) :=
    projT2 (Vector.nth t i). *)
  
(* Definition inject {T:Type} (t:T) : {T:Type & T} := existT _ T t.

Definition cast {T:Type} (U:Type) (t:T) (p:T = U) : U :=
  match p with
  | eq_refl => t
  end. *)



Section code_definitions.

  Check pow.
    

  Axiom (free_cast: forall {T:Type} {a b:T}, a=b).

  (* #[bypass_check(guard=yes)]
  Fixpoint ... . *)
  Unset Guard Checking.


(* .cn f [[a:I32, b:I32], ret: .Cn [I32]] = {
    .cn pow_then [] = {
        ret (1:I32)
    };
    .cn pow_cont [v:I32] = {
        .let m = %core.wrap.mul (0,4294967296) (a,v);
        ret m
    };
    .cn pow_else [] = {
        .let b_1 = %core.wrap.sub (0,4294967296) (b,1:I32);
        f ((a,b_1),pow_cont)
    };
    .let cmp = %core.icmp.e 4294967296 (b,0:I32);
    ((pow_else, pow_then)#cmp) ()
}; *)


  Fixpoint
    f (args: (I32 * I32)*Cn I32) : Cont :=
    let '((a,b),ret) := args in
  let pow_then (args:Unit) : Cont :=
    ret (1:I32)
  in
  let pow_cont (v: I32) : Cont :=
    let m := core_wrap_mul _32 (a,v) in
    ret m
  in
  let pow_else (args:Unit) : Cont :=
    let b1 := core_wrap_sub _32 (b,1:I32) in
    f ((a,b1),pow_cont)
  in
  let cmp := core_icmp_e_fin _32 (b,0:I32) in
  let tup := 
    (inject pow_else) :: (inject pow_then) :: (nil _ : heterogen_vector 0) in
  (* (cast (Cn Unit) (proj tup cmp) (ltac:( admit))) I. *)
  (cast (Cn Unit) (proj_fin tup cmp) free_cast) I.

  Set Guard Checking.

  Check f.

  Definition pow_prop a b c :=
    c = mod_lift pow 32 (a,b).
  
  (* Check (forall a b, f ((a,b),(pow_prop a b))). *)
  (* Goal forall a b c, pow_prop a b c. *)

  Goal forall a b, f ((a,b),(fun _ => True)).
  Proof.
    intros.

    unfold f.
    (* cbv [f]. *)



  Lemma f_prop (a b:I32):
    .


  Lemma f_prop a:
    f (a,(fun b => b = mod_lift mult 32 (4,a))).
  Proof.
    unfold f.
    unfold mod_lift.
    rewrite PeanoNat.Nat.mul_mod_idemp_l;[|lia].
    f_equal.
    lia.
  Qed.

End code_definitions.
