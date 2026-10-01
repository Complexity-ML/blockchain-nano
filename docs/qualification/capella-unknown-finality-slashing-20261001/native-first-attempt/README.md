# First unknown-member finality fixture — coverage failure retained

Two delayed inclusion epochs (10/11) are insufficient. Native test exits 8 after 167.82 s: proof admission at slot 96 already has finalized epoch 9; next finality includes the new member. All preceding forged-proof, checkpoint/archive and restart controls run, but unknown-member finality advance is zero, so the explicit coverage gate fails. This is not a demonstrated runtime corruption or a qualified scenario. No limits changed.
