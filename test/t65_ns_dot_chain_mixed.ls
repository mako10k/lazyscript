!{
    ~ns <- {
        .inc = \~n -> ~~add ~n 1;
        .Nest = { .X = 5 };
    };
    !println (~~to_str (~ns.inc 6));
    !println (~~to_str (~ns.Nest.X));
};
