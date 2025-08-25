!{
  !println (~~to_str (((\(Some ~x) -> 1 | \None -> 2) (Some 0))));
  !println (~~to_str (((\(Some ~x) -> 1 | \None -> 2) (None))));
  !println (~~to_str (((((\(Some ~x) -> 1) | (\None -> 2)) (Some 0)))));
  !println (~~to_str (((((\(Some ~x) -> 1) | (\None -> 2)) (None)))));
};
