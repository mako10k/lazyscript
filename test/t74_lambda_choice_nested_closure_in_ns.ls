!{
  ~ns <- ((~prelude nsnew0));
  # ‘|’ラムダの分岐内でネストしたクロージャを生成・即時適用
  ~~nsdefv ~ns .map (\~f -> \~opt -> (\None -> ((\~u -> None) ())) | (\(Some ~x) -> ((\~u -> Some (~f ~x)) ()))) ~opt);
  ~~println (~~to_str (((~ns .map) (\~n -> ~~add ~n 1)) (Some 2)));
};
