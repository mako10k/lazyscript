!{
        ns <- !!{ Foo = (~add 41 1) };
	~~println (~to_str ((~ns Foo)))
};
