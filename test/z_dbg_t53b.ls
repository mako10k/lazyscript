!{
  ~L <- ((~prelude requirePure) "lib/List.ls");
  ~~println (~to_str ((~L .nil)));
  ~~println (~to_str ((((~L .cons) 1) (((~L .cons) 2) (~L .nil)))));
  ~~println (~~to_str ((((~L .map) (\~x -> ~~add ~x 1)) [1,2,3])));
};
