{
  .nsMembers = ((~prelude nsMembers));
  .nsHas = (\~ns -> \~k -> (
    (
      (\~self -> \~xs -> (
        \[] -> false |
        \(~h : ~t) -> ((~eq ~h ~k) | ((~self ~self) ~t))
      ))
      (\~self -> \~xs -> (
        \[] -> false |
        \(~h : ~t) -> ((~eq ~h ~k) | ((~self ~self) ~t))
      ))
      (((~prelude nsMembers)) ~ns)
    )
  ));
  .nsGetOr = (\~ns -> \~k -> \~d -> (
    ((~ns ~k)) | ~d
  ))
};
