Load helper_lemmas.

Section Addition.

  Hypotheses 
    (in_I32 : nat -> I32)
    (faithful_in_I32: forall n, get_nat (in_I32 n) = n).

  Fixpoint
    add2 (args: (I32)*Cn I32) : Cont :=
    let '(a,ret) := args in
    let b := @core_wrap_add _32 0 (a, in_I32 2) in
    ret b.

Goal add2 (in_I32 40, fun c => c=in_I32 42) .
  unfold add2.
  unfold core_wrap_add.
  apply Fin.to_nat_inj.
  rewrite mod_lift_prop.
  repeat rewrite faithful_in_I32.
  apply PeanoNat.Nat.mod_small_iff.
  - easy. (* _32<>0 *)
  - admit. (* 42<_32 *)
Admitted.

End Addition.
