!{
  ~L <- (~prelude .env .include) "lib/List.ls";
  !println (~~to_str (~L.nil));
  !println (~~to_str (~L.cons 1 (~L.cons 2 (~L.nil))));
  !println (~~to_str (~L.map (\~x -> ~~add ~x 1) [1,2,3]));
  !println (~~to_str (~L.filter (\~x -> (~~lt ~x 3)) [1,2,3]));
  !println (~~to_str (~L.append [1] [2,3]));
  !println (~~to_str (~L.reverse [1,2,3]));
  !println (~~to_str (~L.flatMap (\~x -> (~L.cons ~x (~L.cons ~x (~L.nil)))) [1,2]));
};
