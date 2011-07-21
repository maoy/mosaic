(* $Id: localize.ml 212 2009-04-14 22:17:25Z maoy $ *)

open Cabs
open Printf
open Util
open Symbol

type locationspec =
    M_REMOTE of string
  | M_ANY
  | M_LOCAL;;

let split_locspec_from_expr arg: string list * mexpression =
  match arg with
      MLocspec(MVariable(v)) -> ([v],MVariable(v))
    | MLocspec(_) -> 
        error_loc Compile_error "Location specifier can only be applied to a variable now."
    | expr -> ([], expr);;

let split_locspec_from_functor (func:mfunctor) : (locationspec* mfunctor) =
  let name, args, negate, loc = func in
    current_loc := loc;
    let locs, newargs = List.split (List.map split_locspec_from_expr args) in
    let newfunc = (name, newargs, negate, loc) in
    let locations = List.concat locs in
      match name, locations with
          "periodic", [] -> (M_ANY, newfunc)
        | "periodic", _ -> 
            error_loc Compile_error "Wrong usage of periodic. It should not have any location specifier"
        | _, [] -> (M_LOCAL, newfunc)
        | _, [l] -> (M_REMOTE(l), newfunc)
        | _ -> mlocerror Compile_error 
            (sprintf "term %s has multiple location specifiers defined." name) loc;;

(* 
   compare two location specifiers, return true if the first and the second
   are on the same node
*)
let compare_locspec ls1 ls2 =
  match ls1, ls2 with
      M_ANY, _ 
    | M_LOCAL, M_LOCAL -> true
    | M_REMOTE(s1), M_REMOTE(s2) -> String.compare s1 s2==0
    | _ -> false;;

let conservative_compare_locspec ls1 ls2 =
  match ls1, ls2 with
      M_ANY, M_LOCAL -> true 
    | M_LOCAL, M_LOCAL -> true
    | M_REMOTE(s1), M_REMOTE(s2) -> String.compare s1 s2==0
    | _ -> false;;

let rec localize_body body locspec: mterm list * locationspec =
  match body with
      [] -> [], locspec
    | MFunctor(func)::rest -> 
        let ls,newfunc= split_locspec_from_functor func in
          if not (compare_locspec locspec ls) then
            error_loc Compile_error "The rule body is not localized. localization is not implemented yet.";
          let restbody, ret_ls = localize_body rest ls in
            MFunctor(newfunc)::restbody, ret_ls
    | t::rest ->
        let restbody, ret_ls = localize_body rest locspec in
          t::restbody, ret_ls

let rec localize_clauses clauses =
  List.concat (List.map localize_clause clauses)
and localize_clause clause: mclause list = match clause with
    MEcaRule(( (etype,efunc), body, (atype,afunc) ),loc) -> begin
      current_loc := loc;
      let e_locspec, newefunc = split_locspec_from_functor efunc in
      let newbody, body_ls = localize_body body e_locspec in
      let a_locspec, newafunc = split_locspec_from_functor afunc in
        if conservative_compare_locspec body_ls a_locspec then
          [ MEcaRule(( (etype,efunc), newbody, (atype,newafunc) ),loc) ]
        else
          [ MEcaRule(( (etype,efunc), newbody, (atype,afunc) ),loc) ]
    end
  | MViewRule(rule_name, rule_head, rule_body, loc) -> begin
      current_loc := loc;
      let newbody, body_ls = localize_body rule_body M_ANY in
      let headfunc =  rule_head in
      let h_ls, newhfunc = split_locspec_from_functor headfunc in
        if compare_locspec body_ls h_ls then
          [MViewRule(rule_name, newhfunc, rule_body, loc)]
        else
          [MViewRule(rule_name, rule_head, rule_body, loc)]
    end
  | _ -> [];;

  
