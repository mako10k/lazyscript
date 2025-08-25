!{
  ~ns <- { .x = 1 };
  ~~chain (~~return ()) (\~_ -> ());
  !println (~~to_str (~~nsMembers ~ns));
};
