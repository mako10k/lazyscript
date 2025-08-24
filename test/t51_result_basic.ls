!{
  ~R <- ~~include "lib/Result.ls";
  ~~println (~to_str (~R.Result.Ok 1));
  ~~println (~to_str (~R.map (\~x -> ~~add ~x 1) (~R.Result.Ok 2)));
  ~~println (~to_str (~R.withDefault 9 (~R.Result.Err "e")));
};
