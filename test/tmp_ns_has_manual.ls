!{
  ~has <- (\~k -> (
    (\~self -> \~xs ->
      ((\[] -> false)
       || (\(~h:~t) -> (((\true->true) | (\false-> (~self ~self ~t))) (((~prelude eq)) ~h ~k))))
      ~xs
    )
    (\~self -> \~xs ->
      ((\[] -> false)
       || (\(~h:~t) -> (((\true->true) | (\false-> (~self ~self ~t))) (((~prelude eq)) ~h ~k))))
      ~xs)
  ));

  !println (~to_str (((~has .a) [ .a, .b ])));
  !println (~to_str (((~has .c) [ .a, .b ])));
};
