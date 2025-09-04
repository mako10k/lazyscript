!{
	{- #include "lib/List.ls" -}
	!println (1 : []);
	!println ((List.cons 2) ((List.cons 3) List.nil))
};
