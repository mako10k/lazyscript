!{
  ~has <- (
    ~go (.a : (.b : []));
    ~go = (
      \[] -> false |
      \(~h : ~t) -> (
        (((\true -> true) | (\false -> (~go ~t))) ((~~eq ~h .c)))
      )
    )
  );
  !println (~~to_str ~has);
};
