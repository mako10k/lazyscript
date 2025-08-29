!{
  # expr-choice: recovers on Bottom from left; does not evaluate right when left succeeds
  !println (~~to_str ((^("err") || 42)));
  !println (~~to_str ((7 || ^("nope"))));
};
