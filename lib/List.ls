{
  ~self <- ((~prelude .nsSelf));
  .nil   = [];
  .cons  = \~x -> \~xs -> (~x : ~xs);

  .foldl = (\~f -> \~acc0 -> \~xs0 -> (
    !{
      ~go <- (\~acc -> \~xs -> (
        \[] -> ~acc |
        \(~h : ~t) -> ((~go ((~f ~acc) ~h)) ~t)
      ));
      ((~go ~acc0) ~xs0)
    }
  ));

  .foldr = (\~f -> \~xs0 -> \~z0 -> (
    !{
      ~go <- (\~xs -> \~z -> (
        \[] -> ~z |
        \(~h : ~t) -> ((~f ~h) ((~go ~t) ~z))
      ));
      ((~go ~xs0) ~z0)
    }
  ));

  .map = (\~f -> \~xs -> (((~self .foldr) (\~x -> \~acc -> ((~f ~x) : ~acc)) ~xs [])));

  .filter = (\~p -> \~xs0 -> (
    !{
      ~go <- (\~xs -> (
        \[] -> [] |
        \(~h : ~t) -> (((~p ~h) : (~go ~t)) | (~go ~t))
      ));
      (~go ~xs0)
    }
  ));

  .append = (\~xs0 -> \~ys -> (
    !{
      ~go <- (\~xs -> (
        \[] -> ~ys |
        \(~h : ~t) -> (~h : (~go ~t))
      ));
      (~go ~xs0)
    }
  ));

  .reverse = (\~xs -> ((((~self .foldl) (\~acc -> \~x -> (~x : ~acc)) []) ~xs)));
  .flatMap = (\~f -> \~xs -> (((~self .foldr) (\~x -> \~acc -> ((~self .append) (~f ~x) ~acc)) ~xs [])));
};
