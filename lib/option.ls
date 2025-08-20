!{
  ~Option = !!{ None = None; Some = \~x -> Some ~x };
  ~map = \~f -> \~opt -> (
    \None -> None | \(Some ~x) -> Some (~f ~x)
  ) ~opt;
  ~flatMap = \~f -> \~opt -> (
    \None -> None | \(Some ~x) -> ~f ~x
  ) ~opt;
  ~getOrElse = \~d -> \~opt -> (
    \None -> ~d | \(Some ~x) -> ~x
  ) ~opt
};
