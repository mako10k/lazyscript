{
  .nil = [];
  .cons = \~x -> \~xs -> (~x : ~xs);

  .map = \~f -> \~xs -> (
    ~go ~xs;
    ~go = (
      \[] -> [] |
      \(~h : ~rest) -> ((~f ~h) : (~go ~rest))
    )
  );

  .filter = \~p -> \~xs -> (
    ~go ~xs;
    ~go = (
      \[] -> [] |
      \(~h : ~rest) -> ((\true  -> (~h : (~go ~rest)) | \false -> (~go ~rest)) ((~p ~h)))
    )
  );

  .append = \~xs -> \~ys -> (
    ~go ~xs;
    ~go = (
      \[] -> ~ys |
      \(~h : ~rest) -> (~h : (~go ~rest))
    )
  );

  .reverse = \~xs -> (
    ~go [] ~xs;
    ~go = (\~acc -> \~zs -> (
      (\[] -> ~acc | \(~h : ~rest) -> (~go (~h : ~acc) ~rest)) ~zs
    ))
  );

  .flatMap = \~f -> \~xs -> (
    ~go ~xs;
    ~append = (\~as -> \~bs -> (
      ~app ~as;
      ~app = (
        \[] -> ~bs |
        \(~h1 : ~t1) -> (~h1 : (~app ~t1 ~bs))
      )
    ));
    ~go = (
      \[] -> [] |
      \(~h : ~rest) -> (~append ((~f ~h)) ((~go ~rest)))
    )
  )
};
