!{
  ~res <- (
    ~f = (
      \[] -> false |
      \(~h : ~t) -> (~f ~t)
    );
    (~f (.a : (.b : [])))
  );
  !println (~~to_str ~res);
};
