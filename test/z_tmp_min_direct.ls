!{
  v <- (
    ((\[] -> false) | (\(~h:~t) -> (((\true -> true) | (\false -> ((\xs -> xs) ~t))) (((~prelude eq)) ~h .a))))
    [ .a, .b ]
  );
  !println (~~to_str v);
};
