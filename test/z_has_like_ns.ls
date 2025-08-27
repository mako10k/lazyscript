!{
  ~res <- (
    ~has (.a : (.b : []));
    ~has = (
      \[] -> false |
      \(~h : ~t) -> (
        (((\true -> true) | (\false -> (~has ~t))) ((~~eq ~h .c)))
      )
    )
  );
  !println (~~to_str ~res);
};
