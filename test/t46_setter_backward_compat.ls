!{
  ns <- !nsnew0;
  # Backward-compatible setter access via symbol .__set
  ((~ns .__set) .x 1);
  ~~println (~~to_str ((~prelude nsMembers) ~ns));
};
