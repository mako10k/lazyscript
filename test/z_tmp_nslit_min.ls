{ .Option = { .None = None; .Some = \~x -> Some ~x }; .map = (\~f -> \~opt -> (\(Some ~x) -> Some (~f ~x)) ~opt) };
