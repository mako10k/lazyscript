!{
  {- #include "lib/List.ls" -}
  ~L <- List;
  !println (~~to_str (List.nil));
  !println (~~to_str (List.cons 1 (List.cons 2 (List.nil))));
  !println (~~to_str (List.map (\~x -> ~~add ~x 1) [1,2,3]));
  !println (~~to_str (List.filter (\~x -> (~~lt ~x 3)) [1,2,3]));
  !println (~~to_str (List.append [1] [2,3]));
  !println (~~to_str (List.reverse [1,2,3]));
  !println (~~to_str (List.flatMap (\~x -> (List.cons ~x (List.cons ~x (List.nil)))) [1,2]));
};
