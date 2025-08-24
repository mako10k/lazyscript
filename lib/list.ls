# DEPRECATED: This legacy file defines global cons/nil with side-effects.
# Prefer: requirePure "lib/List.ls" and use (~L .cons)/(.nil), or load "lib/std.ls".

!{ !def cons (\ ~x -> \ ~xs -> (~x : ~xs)); !def nil ([]) };
