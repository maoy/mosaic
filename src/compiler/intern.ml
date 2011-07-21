(* $Id: intern.ml 212 2009-04-14 22:17:25Z maoy $ *)

open Cabs
open Printf
open Symbol

let string_consts = Hashtbl.create 100;;
let str_const_idx = ref 0;;

let allocate_string_const_var s =
  try
    Hashtbl.find string_consts s
  with Not_found ->
    let varname = sprintf "_ms_str_const_%d" (!str_const_idx) in
      str_const_idx := (!str_const_idx) +1;
      Hashtbl.add string_consts s varname;
      Hashtbl.add Symbol.symbols varname
        (MConst(MTstring, varname, MConstant(M_CONST_STRING(s)), default_loc ));
      varname

let rec nostring_expr ex :mexpression=
  match ex with
      MUnary(op, e) -> MUnary(op, nostring_expr e)
    | MBinary(op,e1,e2) -> MBinary(op, nostring_expr e1, nostring_expr e2)
    | MQuestion(e1,e2,e3) -> MQuestion(nostring_expr e1, nostring_expr e2, nostring_expr e3)
    | MVariable(s) -> ex
    | MMemberOf(e,s) -> MMemberOf( nostring_expr e, s)
    | MParen(e) -> MParen( nostring_expr e)
    | MCall(e1,elist) -> MCall(nostring_expr e1, List.map nostring_expr elist)
    | MAggregation(ag, e) -> MAggregation(ag, nostring_expr e)
    | MNewVector(e) -> MNewVector( nostring_expr e)
    | MLocspec(e) -> MLocspec( nostring_expr e)
    | MDynamicCast(t,e) -> MDynamicCast(t, nostring_expr e)
    | MStaticCast(t,e) -> MStaticCast(t, nostring_expr e)
    | MMemberOfPtr(e,s) -> MMemberOfPtr(nostring_expr e,s)
    | MIndex(e1,e2) -> MIndex(nostring_expr e1, nostring_expr e2)
    | MConstant(c) ->
        match c with
            M_CONST_STRING(s)->
              MVariable( allocate_string_const_var s )
          | _ -> MConstant(c);;
          
        

let nostring_func (name, expr_list, negate, loc) :mfunctor =
  (name, List.map nostring_expr expr_list, negate, loc);;

let nostring_term t :mterm = 
  match t with
      MFunctor(f) -> MFunctor(nostring_func f)
    | MAssign(name,expr, loc) -> MAssign(name, nostring_expr expr, loc)
    | MExpr(e) -> MExpr(nostring_expr e)

let eliminate_string_consts clause =
  match clause with
      MEcaRule( ( (ev_type, ev_func), termlist, (ac_type, ac_func) ), loc) -> 
        MEcaRule( ((ev_type, nostring_func ev_func),
                  List.map nostring_term termlist,
                  (ac_type, nostring_func ac_func)),
                  loc)
    | MViewRule( nm, rule_head, rule_body, loc) -> 
        MViewRule( nm, 
               nostring_func rule_head, 
               List.map nostring_term rule_body,
               loc)
    | _ -> clause
