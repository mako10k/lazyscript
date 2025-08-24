!{
  # Named/mutable namespaces are removed. This test now validates key typing:
  # nsMembers and nslit accept only symbol keys; strings are rejected.
  !println (~~to_str ((!!{ .k = 1 }) .k));
  # Attempting to index with a string key yields an error
  !println (~~to_str (((!!{ .k = 1 }) "s")));
};
