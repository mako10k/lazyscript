!{
  ~nsMembers = (~prelude nsMembers);
  ~nsHas = \~ns -> \~k -> (
    ~m <- (~nsMembers ~ns);
    ~go = \[] -> false | \(~h : ~t) -> (
      ((~eq ~h ~k)) | (~go ~t)
    );
    ~go ~m
  );
  ~nsGetOr = \~ns -> \~k -> \~d -> (
    ((~ns ~k)) | ~d
  )
};
