!{
  ~ns <- ((~prelude nsnew0));
  ((~prelude chain)
    (~~nsdefv ~ns .map (\~f -> \~opt -> (\None -> None | \(Some ~x) -> Some (~f ~x)) ~opt))
    (\~_ ->
      ((~prelude chain)
        (~~nsdefv ~ns .seed (Some 2))
  (\~_ -> ~~println (~~to_str (((~ns .map) (\~n -> ~~add ~n 1)) (~ns .seed)))))));
};
