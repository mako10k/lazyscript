!{
  ns <- (~~nsnew0);
  opt <- (~~nsnew0);
  (~~nsdefv opt .None None);
  (~~nsdefv opt .Some (\~x -> Some ~x));
  (~~nsdefv ns .Option ~opt);
  (~~nsdefv ns .map (\~f -> \~optv -> (\(Some ~x) -> Some (~f ~x)) ~optv));
  ~~println (~~to_str ((((~ns .map) (\~x -> ~~add ~x 1)) (((~ns .Option) Some) 2))));
};
