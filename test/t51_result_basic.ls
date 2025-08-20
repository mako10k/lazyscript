!{
  ~~require "lib/result.ls";
  ~~println (~to_str (((~Result Ok) 1)));
  ~~println (~to_str ((((~map (\~x -> ~add ~x 1))) (((~Result Ok) 2)))));
  ~~println (~to_str ((((~withDefault 9)) (((~Result Err) "e")))));
};
