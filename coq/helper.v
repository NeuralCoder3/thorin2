Load defs.
Require Import NPeano.
Require Import Lia.

Definition proj 
  {n:nat} 
  (t:heterogen_vector n) (i:Idx n) : projT1 (Vector.nth t i) :=
    projT2 (Vector.nth t i).

Definition inject {T:Type} (t:T) : {T:Type & T} := existT _ T t.

Definition cast {T:Type} (U:Type) (t:T) (p:T = U) : U :=
  match p with
  | eq_refl => t
  end.

Definition mod_lift
    (f:nat -> nat -> nat) n:
    Idx n * Idx n -> Idx n.
    refine (fun '(a,b) => _).
    pose proof (Fin.to_nat a) as [a_nat Ha].
    pose proof (Fin.to_nat b) as [b_nat Hb].
    pose (res := f a_nat b_nat).
    pose (res_mod := res mod n).
    apply @Fin.of_nat_lt with (p:=res_mod).
    subst res_mod.
    apply Nat.mod_upper_bound.
    destruct n;[|easy].
    inversion a.
Defined.

Definition core_wrap_add {s:nat} (m:nat) (ab:Idx s * Idx s) : Idx s :=
  mod_lift (fun a b => a + b + m) s ab.


