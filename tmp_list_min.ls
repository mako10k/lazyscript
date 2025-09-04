{
  .map = \~f -> \~xs -> (
    ~go = (\~f -> \~xs -> (
      (\[] -> [] |
       \(~h : ~rest) -> ((~f ~h) : (~go ~f ~rest))) ~xs
    ));
    (~go ~f ~xs)
  )
}
