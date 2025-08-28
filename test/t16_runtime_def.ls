!{
	!withImport { .Foo = 123 } (\ _ -> ());
	!println (~~to_str ~Foo)
};
