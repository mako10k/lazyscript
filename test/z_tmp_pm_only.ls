(\~f -> \~opt -> (\None -> None | \(Some ~x) -> Some (~f ~x)) ~opt)
