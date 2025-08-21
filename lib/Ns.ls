{
  ~self <- ((~prelude .nsSelf));
  .nsMembers = ((~prelude .nsMembers));
  .nsHas = (\~ns -> \~k -> (
    !{
      ~go <- (\~ms -> (
        \[] -> false |
        \(~h : ~t) -> ((\(~k) -> true | \_ -> (~go ~t)) ~h)
      ));
      (~go ((~prelude .nsMembers) ~ns))
    }
  ));
  .nsGetOr = (\~ns -> \~k -> \~d -> (
    !{
      ~go <- (\~ms -> (
        \[] -> ~d |
        \(~h : ~t) -> ((\(~k) -> ((~ns ~h)) | \_ -> (~go ~t)) ~h)
      ));
      (~go ((~prelude .nsMembers) ~ns))
    }
  ))
};
