Require Import Coq.Numbers.NatInt.NZDiv.
Require Import NPeano.
Require Import Lia.


Definition Int (n:nat) := nat.
Definition Cn (A:Type) := A -> Prop.


Definition mod_lift
    (f:nat -> nat -> nat) n:
    Int n * Int n -> Int n :=
    fun '(a,b) => f a b mod n.

Section code_definitions.

  (* #[bypass_check(guard=yes)]
  Fixpoint ... . *)
  Unset Guard Checking.

  Fixpoint
      f (arg:(Int 32* Cn (Int 32))) : Prop :=
    let (a, ret) := arg in
    ret (mod_lift mult 32 (mod_lift plus 32 (a,a), 2)).

  Set Guard Checking.

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