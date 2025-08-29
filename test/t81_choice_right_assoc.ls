!{
  # lambda-choice '|' right-associative; use branches that match to avoid mismatch diagnostics
  !println (~~to_str (((\(Some ~x) -> ~x | \None -> 9 | \_ -> 99) (Some 2))));
  !println (~~to_str (((\None -> 0 | \(Some ~x) -> ~x | \_ -> 9) (None))));

  # expr-choice '||' is right-associative and left-biased (no bottoms here)
  !println (~~to_str (1 || 2 || 3));
  !println (~~to_str (0 || 2));
  !println (~~to_str (0 || (0 || 5)));
};
