(* $Id: printstmt.ml 212 2009-04-14 22:17:25Z maoy $ *)

open Cabs
open Symbol
open Util
open Printf

let rec string_of_expr arg: string=
    match arg with
        MVariable(varname) -> varname
      | MConstant(c) -> begin
          match c with
              M_CONST_INT(s) -> s
            | M_CONST_STRING(s) -> sprintf "\"%s\"" (String.escaped s)
            | _ -> merror Internal_error "not implemented constant"
        end
      | MBinary(op, exp1, exp2) -> begin
          let sop = match op with
              M_ADD -> "+"
            | M_SUB -> "-"
            | M_MUL -> "*"
            | M_DIV -> "/"
            | M_AND -> "&&"
            | M_MOD -> "%"
            | M_OR -> "||"
            | M_EQ -> "=="
            | M_NE -> "!="
            | M_LT -> "<"
            | M_GT -> ">"
            | M_LE -> "<="
            | M_GE -> ">=" 
            | M_SHR -> ">>"
            | M_SHL -> "<<"
            | M_XOR -> "^"
            | M_BOR -> "|"
            | M_BAND -> "&"
          in
          let sexp1 = string_of_expr exp1 in
          let sexp2 = string_of_expr exp2 in
            sprintf "%s %s %s" sexp1 sop sexp2
        end
      | MUnary(op, exp) -> begin
          let sop = match op with
              M_MINUS -> "-"
            | M_PLUS  -> "+"
            | M_NOT   -> "!" 
            | M_BNOT  -> "~"
            | M_ADDROF -> "&"
            | M_MEMOF -> "*"
          in
          let sexp = string_of_expr exp in
            sprintf "%s %s" sop sexp
        end
      | MAggregation(aggtype, MVariable(varname)) -> 
          sprintf "%s<%s>" (string_of_aggtype aggtype) varname
      | MAggregation(_,_) ->
          merror Internal_error "aggregation is not applied on a variable"
      | MNewVector(exp) -> sprintf "f_make_vector( %s )" (string_of_expr exp)
      | MLocspec(exp) -> string_of_expr exp
      | MCall(ex1,callargs) -> 
          sprintf "%s( %s )" (string_of_expr ex1) (string_of_exprs callargs)
      | MParen(ex) -> sprintf "(%s)" (string_of_expr ex)
      | MMemberOf(ex, fieldname) -> sprintf "%s.%s" (string_of_expr ex) fieldname
      | MMemberOfPtr(ex,fn) -> sprintf "%s->%s" (string_of_expr ex) fn
      | MStaticCast(t,ex) -> 
          sprintf "static_cast<%s>(%s)" 
            (Printcpp.string_of_primitive_type t) (string_of_expr ex)
      | MDynamicCast(t,ex) -> 
          sprintf "dynamic_cast<%s>(%s)" 
            (Printcpp.string_of_primitive_type t) (string_of_expr ex)
      | MQuestion(q,ex1,ex2) ->
          sprintf "%s?%s:%s" (string_of_expr q) (string_of_expr ex1) (string_of_expr ex2)
      | MIndex(ex1,ex2) ->
          sprintf "%s[ %s ]" (string_of_expr ex1) (string_of_expr ex2)

and string_list_of_exprs args =
  List.map string_of_expr args
and string_of_exprs args = 
  String.concat ", " (string_list_of_exprs args);;


let print_bindings buf bindings = 
  let print_binding buf (name, position, tp) = 
    bprintf buf "%d->%s:%s " position name (Printcpp.string_of_primitive_type tp)
  in
    bprintf buf "[";
    List.iter (print_binding buf) bindings;
    bprintf buf "]"

let print_event buf (ev_type, ev_name, ev_args, bindings, evloc) =
  bprintf buf "#%s:%d\n" evloc.filename evloc.lineno;
  bprintf buf "EVENT %s:%s" (ev_type2str ev_type) ev_name;
  bprintf buf "(%s)" (string_of_exprs ev_args);
  print_bindings buf bindings;
  bprintf buf "\n"
    

let string_of_join_terms jts =
  let string_of_join_term (pos,value) =
    sprintf "%d=%s" pos (string_of_expr value) 
  in
    String.concat "," (List.map string_of_join_term jts)

let print_term buf term = 
  match term with
      MPrimaryJoin(name, jts, bts, loc) -> begin
        bprintf buf "  PrimaryJoin on %s(%s)" name (string_of_join_terms jts);
        print_bindings buf bts;
        bprintf buf "\n";
      end
    | MSecondaryJoin(name,_, jts, bts, loc) -> begin
        bprintf buf "  SecondaryJoin on %s(%s)" name (string_of_join_terms jts);
        print_bindings buf bts;
        bprintf buf "\n";
      end
    | MQuery(name, bts, loc) -> begin
        bprintf buf "  CProduct on %s " name;
        print_bindings buf bts;
        bprintf buf "\n";
      end
    | MNegatePrimaryJoin(name, jts, bts, _) -> begin
        bprintf buf "  NegatePrimaryJoin on %s(%s)" name (string_of_join_terms jts);
        print_bindings buf bts;
        bprintf buf "\n";
      end
    | MNegateSecondaryJoin(name, _, jts, bts, _) -> begin
        bprintf buf "  NegateSecondJoin on %s(%s)" name (string_of_join_terms jts);
        print_bindings buf bts;
        bprintf buf "\n";
      end
    | MFilter([]) | MNegFilter([]) -> ();
    | MFilter(conditions) ->
        bprintf buf "  Selection on %s\n" (String.concat "&&" (string_list_of_exprs conditions))
    | MNegFilter(conditions) ->
        bprintf buf "  Selection on !(%s)\n" (String.concat "&&" (string_list_of_exprs conditions))
    | MAssignment(varname, expr, _) -> begin
        bprintf buf "  Assignment %s=%s\n" varname (string_of_expr expr);
      end
    | MAggTerm(_) -> begin
        bprintf buf " Agg\n";
      end
        
let print_body buf body = 
  List.iter (print_term buf) body

let print_action buf (act_type, act_name, act_args, act_loc) =
  bprintf buf "  ACTION %s:%s(%s)\n" 
    (action_type_to_string act_type) act_name (string_of_exprs act_args)

let print_stmt buf (event, body, action,_) =
  print_event buf event;
  print_body buf body;
  print_action buf action;
  bprintf buf "\n"

