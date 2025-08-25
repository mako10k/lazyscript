!{
  ~ns <- !!{
    .map = (\~f -> \~opt -> (\None -> None | \(Some ~x) -> Some (~f ~x)) ~opt);
  };
  !println (~to_str (~ns.map (\~n -> ~add ~n 1) (Some 2)));
};
