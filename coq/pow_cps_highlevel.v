Require Import NPeano.
Require Import Lia.
Load defs.


  (*
  Proof that high-level cps corresponds to pow
  *)

  Fixpoint pow_cps_ref (a b:nat) (ret:Cn nat) :=
    match b with
    | 0 => ret 1
    | S b' => pow_cps_ref a b' (fun x => ret (a * x))
    end.

(* CPS functions are congruent over their return predicate *)
  Lemma pow_cps_congruence:
    forall a b (ret1 ret2: Cn nat),
      (forall x, ret1 x -> ret2 x) ->
      pow_cps_ref a b ret1 -> pow_cps_ref a b ret2.
  Proof.
    intros a b ret1 ret2 H H1.
    induction b in ret1, ret2, H, H1 |- *.
    - simpl in *.
      apply H.
      assumption.
    - simpl in *.
      eapply IHb.
      2: apply H1.
      intros.
      apply H.
      apply H0.
  Qed.


  Goal forall a b, pow_cps_ref a b (fun c => c = pow a b).
  Proof.
    intros.
    induction b.
     (* in a |- *. *)
    - simpl. reflexivity.
    - simpl. 
      eapply pow_cps_congruence.
      2: apply IHb.
      simpl.
      lia.
  Qed.
