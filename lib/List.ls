{
  .nil = [];
  .cons = \~x -> \~xs -> (~x : ~xs);

  .map = \~f -> \~xs -> (
    ~go ~f ~xs;
    ~go = (\~f -> \~xs -> (
      (\[] -> [] |
       \(~h : ~rest) -> ((~f ~h) : (~go ~f ~rest))) ~xs
    ))
  );

  .filter = \~p -> \~xs -> (
    ~go ~xs;
    ~go = (
      \[] -> [] |
      \(~h : ~rest) -> ((\true  -> (~h : (~go ~rest)) | \false -> (~go ~rest)) ((~p ~h)))
    )
  );

  .append = \~xs -> \~ys -> (
    ~go ~ys ~xs;
    ~go = (\~ys -> \~xs -> (
      (\[] -> ~ys |
       \(~h : ~rest) -> (~h : (~go ~ys ~rest))) ~xs
    ))
  );

  .reverse = \~xs -> (
    ~go [] ~xs;
    ~go = (\~acc -> \~zs -> (
      (\[] -> ~acc | \(~h : ~rest) -> (~go (~h : ~acc) ~rest)) ~zs
    ))
  );

  .flatMap = \~f -> \~xs -> (
    ~go ~f ~xs;
    ~append = (\~as -> \~bs -> (
      ~app ~as ~bs;
      ~app = (\~as -> \~bs -> (
        (\[] -> ~bs |
         \(~h1 : ~t1) -> (~h1 : (~app ~t1 ~bs))) ~as
      ))
    ));
    ~go = (\~f -> \~xs -> (
      (\[] -> [] |
       \(~h : ~rest) -> (~append ((~f ~h)) ((~go ~f ~rest)))) ~xs
    ));
  )
}
