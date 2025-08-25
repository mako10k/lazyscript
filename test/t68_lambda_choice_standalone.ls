# ‘|’ を含むラムダを単体適用（NSやnslitは絡めない）
((\None -> 0 | \(Some ~x) -> ~x) (Some 3));
