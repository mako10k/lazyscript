!{
  # ある程度大きいNSに混在（値/関数）を投入してからmap適用
  ns <- !!{
    .k0 = 0; .k1 = 1; .k2 = 2; .k3 = 3; .k4 = 4; .k5 = 5; .k6 = 6; .k7 = 7; .k8 = 8; .k9 = 9;
    .f = (\~f -> \~opt -> (\None -> None | \(Some ~x) -> Some (~f ~x)) ~opt)
  };
  ~~println (~~to_str (((~ns .f) (\~n -> ~~add ~n 1)) (Some 2)));
};
