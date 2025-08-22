!{
  ~ns <- !!{ .map = (\~f -> \~opt -> (\None -> None | \(Some ~x) -> Some (~f ~x)) ~opt)
           ; .seed = (Some 1)
           };
  # 2段適用
  ~~println (~to_str (((~ns .map) (\~n -> ~add ~n 1)) ((~ns .map) (\~n -> ~add ~n 1) (~ns .seed))));
};
