
Require Import Coq.Vectors.Fin.
Require Import Coq.Numbers.NatInt.NZDiv.
Require Import NPeano.
Require Import Lia.


Definition Int n := Fin.t n.
Definition Cn (A:Type) := A -> Prop.


Definition mod_lift
    (f:nat -> nat -> nat) n:
    Int n * Int n -> Int n.
Proof.
  intros [a b].
  destruct (Fin.to_nat a) as [na Ha].
  destruct (Fin.to_nat b) as [nb Hb].
  pose (nc := (f na nb) mod n).
  apply (@Fin.of_nat_lt nc).
  destruct n.
  - inversion a.
  - now apply PeanoNat.Nat.mod_upper_bound.
Defined.

Section code_definitions.

  Variable (
      f: Cn ((Int 32) * Cn (Int 32))
  ).

  Hypothesis(f_spec:
  forall a ret,
      f (a, ret) = ret (mod_lift mult 32 (mod_lift plus 32 (a,a), @Fin.of_nat_lt 2 32 (ltac:(lia))))).

      Search ((mult _ _ ) mod _).

  (* or reason with to_nat *)
  Lemma f_prop a:
    f (a,(fun a => a = mod_lift mult 32 (@Fin.of_nat_lt 4 32 (ltac:(lia)), a))).
  Proof.
    rewrite f_spec.
    cbn.
    destruct (to_nat a) as [na Ha].
    repeat rewrite Fin.to_nat_of_nat.
    rewrite PeanoNat.Nat.mul_mod_idemp_l.
    lia.


End code_definitions.


