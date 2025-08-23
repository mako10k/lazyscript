{
  .nsMembers = ((~prelude nsMembers));
  .nsHas = (\~ns -> \~k -> (
    (
      (\~self -> \~xs -> (
        \[] -> false |
        \(~h : ~t) -> (
          (((\true -> true) | (\false -> ((~self ~self) ~t))) ((((~prelude eq)) ~h ~k)))
        )
      ))
      (\~self -> \~xs -> (
        \[] -> false |
        \(~h : ~t) -> (
          (((\true -> true) | (\false -> ((~self ~self) ~t))) ((((~prelude eq)) ~h ~k)))
        )
      ))
      (((~prelude nsMembers)) ~ns)
    )
  ));
  .nsGetOr = (\~ns -> \~k -> \~d -> (
  (((~ns ~k)) | (~d))
  ))
};
