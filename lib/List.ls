{
  .nil = [];
  .cons = \~x -> \~xs -> (~x : ~xs);

  .map = \~f -> \~xs -> (
    ~go = (\~f -> \~xs -> (
      (\[] -> [] |
       \(~h : ~rest) -> ((~f ~h) : (~go ~f ~rest))) ~xs
    ));
    (~go ~f ~xs)
  );

  .filter = \~p -> \~xs -> (
    ~go = (
      \[] -> [] |
      \(~h : ~rest) -> ((\true  -> (~h : (~go ~rest)) | \false -> (~go ~rest)) ((~p ~h)))
    );
    ~go ~xs
  );

  .append = \~xs -> \~ys -> (
    ~go = (\~ys -> \~xs -> (
      (\[] -> ~ys |
       \(~h : ~rest) -> (~h : (~go ~ys ~rest))) ~xs
    ));
    (~go ~ys ~xs)
  );

  .reverse = \~xs -> (
    ~go = (\~acc -> \~zs -> (
      (\[] -> ~acc | \(~h : ~rest) -> (~go (~h : ~acc) ~rest)) ~zs
    ));
    ~go [] ~xs
  );

  .flatMap = \~f -> \~xs -> (
    ~append = (\~as -> \~bs -> (
      ~app = (\~as -> \~bs -> (
        (\[] -> ~bs |
         \(~h1 : ~t1) -> (~h1 : (~app ~t1 ~bs))) ~as
      ));
      (~app ~as ~bs)
    ));
    ~go = (\~f -> \~xs -> (
      (\[] -> [] |
       \(~h : ~rest) -> (~append ((~f ~h)) ((~go ~f ~rest)))) ~xs
    ));
    (~go ~f ~xs)
  )
};
