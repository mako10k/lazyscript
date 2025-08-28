!{
	!withImport ((~prelude .env .include) "lib/List.ls") (\ _ -> ());
	!println (1 : []);
	!println ((~cons 2) ((~cons 3) ~nil))
};
