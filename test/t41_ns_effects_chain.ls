!{
  ~ns <- !nsnew0;
  ~~chain (!nsdefv ~ns .x 1) (\~_ -> ());
  !nsMembers ~ns
};
