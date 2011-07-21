(* $Id: util.ml 212 2009-04-14 22:17:25Z maoy $ *)

open Cabs
open Printf

let merror ex msg = 
  output_string stderr (msg ^ "\n");
  raise ex

let mlocerror ex msg loc = 
  fprintf stderr "File \"%s\", line %d:\n%s\n" loc.filename loc.lineno msg;
  raise ex

let debug msg =
  if true then begin
    output_string stderr "[debug] ";
    output_string stderr msg;
    output_string stderr "\n";
  end
    
let error_loc ex msg = 
  mlocerror ex msg !current_loc

let warning_loc msg =
  let loc = !current_loc in
    fprintf stderr "File \"%s\", line %d:\n" loc.filename loc.lineno;
    fprintf stderr "Warning: %s\n" msg;;

exception Compile_error
exception Internal_error

module Int_set = Set.Make (struct
                             type t = int
                             let compare = compare
                           end)

let string_of_key k =
  String.concat "," (List.map string_of_int k);;

let string_of_aggtype = function
  | M_AGG_MIN -> "min"
  | M_AGG_MAX -> "max"
  | M_AGG_COUNT -> "count";;


let rec list2set (l:int list) : Int_set.t =
  match l with
      [] -> Int_set.empty
    | hd::tl -> Int_set.add hd (list2set tl);;

let list_subset l1 l2 : bool =
  (*fprintf stderr "try subset [%s] < [%s]\n" (string_of_key l1) (string_of_key l2);*)
  let s1 = list2set l1 in
  let s2 = list2set l2 in
    Int_set.subset s1 s2;;

let list_diff l1 l2: int list =
  let s1 = list2set l1 in
  let s2 = list2set l2 in
    Int_set.elements (Int_set.diff s1 s2);;

let rec find_locspec (args:mexpression list): mexpression =
  match args with
      [] -> raise Not_found
    | hd::tl -> match hd with 
          MLocspec(expr) -> expr
        | _ -> find_locspec tl;;

let term2functor t =
  match t with
    MFunctor( name, args, negate, loc ) -> (name, args, negate, loc)
  | _ -> 
      merror Compile_error "term is not not a functor"

let ev_type2str = function
  | EV_INSERTED -> "ins"
  | EV_RECV | EV_PERIODIC(_) -> "recv"
  | EV_DELETED -> "del"

let functorname :mfunctor -> string  = function
  (name, _, _, _) -> name;;

let variable2varname :mexpression -> string = function
  | MVariable(varname) -> varname
  | _ -> error_loc Compile_error "the expression is not a variable";;

let rec expr_has_agg = function
  | MAggregation(a, MVariable(varname) ) -> [(a,varname)]
  | MAggregation(a, _) -> merror Internal_error "aggregation applied on non-variable"
  | MUnary (_, r) -> expr_has_agg r
  | MBinary(_, r1, r2 ) -> (expr_has_agg r1) @ (expr_has_agg r2)
  | MQuestion(r1, r2, r3) -> (expr_has_agg r1) @ (expr_has_agg r2) @ (expr_has_agg r3)
  | MMemberOf(r, _) -> expr_has_agg r
  | MParen(r) -> expr_has_agg r
  | MLocspec (r) -> expr_has_agg r
  | MNewVector (r) -> expr_has_agg r
  | MVariable _ -> []
  | MConstant _ -> []
  | MDynamicCast(_)|MStaticCast(_)|MCall(_)|MMemberOfPtr(_) -> []
  | MIndex(r1, r2 ) -> (expr_has_agg r1) @ (expr_has_agg r2)

and expr_list_has_agg rlist : (maggtype*string) list =
  List.fold_left (@) [] (List.map expr_has_agg rlist);;

let eca_event_loc (e: ecaEvent) :cabsloc =
  match e with
      (_,(_,_,_,loc))->loc

let rec find_all_vars_in_expr = function
  | MConstant(_) -> []
  | MVariable(v) -> [v]
  | MUnary(_, e) | MMemberOf(e,_) | MMemberOfPtr(e,_) | MParen(e)
  | MAggregation(_,e) | MLocspec(e) | MNewVector(e) 
  | MStaticCast(_,e) | MDynamicCast(_,e) 
    -> find_all_vars_in_expr e
  | MBinary(_,e1,e2) | MIndex(e1,e2)
    -> (find_all_vars_in_expr e1) @ (find_all_vars_in_expr e2)
  | MQuestion(e1,e2,e3) ->
      (find_all_vars_in_expr e1) 
      @ (find_all_vars_in_expr e2)
      @ (find_all_vars_in_expr e3)
  | MCall(_,elist) -> 
      List.flatten (List.map find_all_vars_in_expr elist)

let find_all_vars_in_func = function
    (_,args,_,_) ->
      List.concat (List.map find_all_vars_in_expr args)

let find_all_vars_in_term = function
  | MFunctor(f) -> find_all_vars_in_func f
  | MAssign(lhs, rhs, _) ->
      lhs :: (find_all_vars_in_expr rhs)
  | MExpr(e) -> find_all_vars_in_expr e

let list_unique l =
  let tbl = Hashtbl.create 10 in
  List.iter (fun i -> Hashtbl.replace tbl i ()) l;
  Hashtbl.fold (fun key data accu -> key :: accu) tbl []
