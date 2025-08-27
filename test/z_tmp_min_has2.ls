(
  ~has [ .a, .b ];
  ~has = (
    \[] -> false |
    \(~h : ~t) -> (((\true -> true) | (\false -> (~has ~t))) (((~prelude eq)) ~h .a))
  )
)
