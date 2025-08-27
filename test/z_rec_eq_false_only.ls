!{
  ns <- (.a : (.b : []));
  ~get = (
    \[] -> 99 |
    \(~h : ~t) -> (((\true -> 0) | (\false -> (~get ~t))) ((~~eq ~h .c)))
  );
  !println (~~to_str (~get ~ns));
};
