!{
  ~~require "lib/option.ls";
  ~~println (~to_str ((~Option Some) 1));
  ~~println (~to_str ((~map (\~x -> ~add ~x 1)) ((~Option Some) 2)));
  ~~println (~to_str (((~getOrElse 99) ((~Option None)))));
};
