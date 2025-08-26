!{
  let res = !requireOpt "no_such_file.ls";
  let res2 = !requireOpt "lib_req.ls";
  (~Prelude .to_str) (~Prelude .Option res) ~res ++ "|" ++ (~Prelude .to_str) (~Prelude .Option res2) ~res2
};
