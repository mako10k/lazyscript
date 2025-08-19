!{
  ns <- { Inc = (\n -> (~add n 1)) };
  ~~println (~to_str (((~ns Inc) 41)));
};
