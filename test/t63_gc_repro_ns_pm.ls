!{
  # Use vetted Option lib; keep the pattern-matching map lambda locally to mimic repro shape
  ~M <- ((~prelude .requirePure) "lib/Option.ls");
  ns <- { .Option = (~M .Option);
  .map    = (\~f -> \~opt -> ((((\(Some ~x) -> (Some (~f ~x))) ~opt) | None))) };
  ~~println (~to_str ((~ns .map) (\~x -> ~add ~x 1) ((~ns .Option) .Some 2)));
};
 
