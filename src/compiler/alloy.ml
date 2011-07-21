(* $Id: alloy.ml 212 2009-04-14 22:17:25Z maoy $ *)

open Cabs
open Symbol
open Util
open Printf

let rec string_of_primitive_type t =
  match t with
      MTint -> "Int"
    | MUserType("UDPAddress") -> "Node"
    | MUserType(t) -> t
    | MTstring -> "gc_string"
    | MTdouble -> "double"
    | MTlong -> "int64_t"
    | MTemplate(t,tlist) ->
        sprintf "%s< %s >" t (String.concat ", " (List.map string_of_primitive_type tlist))
    | MUnknownType 
    | MTableType(_)
    | MViewType(_)
    | MEventType(_)
      -> merror Internal_error "unknown primitive type"

let alloy_event_name name = match name with
    "initEvent" -> "initEvent"
  | _ ->  sprintf "Event_%s" name;;

let alloy_field_name name = sprintf "f_%s" name;;

let alloy_variable_name name = sprintf "%s" name;;

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


let print_event_def buf name tupletype = match name with
    "initEvent" -> ()
  | _ -> begin
      bprintf buf "sig %s extends UserEvent {\n" (alloy_event_name name);
      let print_field f =
	bprintf buf "  %s:%s,\n" 
	  (alloy_field_name (fst f)) (string_of_primitive_type (snd f))
      in
	List.iter print_field tupletype;
	bprintf buf "}\n"
    end

let relations: (string (*def*), string (*primary key fact*)) Hashtbl.t
    = Hashtbl.create 10;;

let print_table_def buf name tupletype pkey =
  let print_field f = string_of_primitive_type (snd f) in
  let definition = sprintf "%s:%s"
    name (String.concat "->" (List.map print_field (List.tl tupletype)))
  in
  let pkey_fact = sprintf "" (*TODO*)
  in
    Hashtbl.add relations definition pkey_fact;;

let print_def buf name def = 
  match def with
    | MDef( MEventType(tupletype), name, _) ->
	print_event_def buf name tupletype
    | MDef( MTableType(tupletype, pkey, _, _ ), name, _) ->
        print_table_def buf name tupletype pkey
    | MDef( MViewType(tupletype, pkey,_,_), name, _) ->
        print_table_def buf name tupletype !pkey
    | _ -> ()

let print_defs buf =
  Hashtbl.iter (print_def buf) symbols;
  bprintf buf "sig Node {\n";
  let print_relation definition pkey_fact =
    bprintf buf "  %s,\n" definition 
  in
    Hashtbl.iter print_relation relations;
    bprintf buf "} --end of Node\n"


let bound_functor_field f_name n pos expr = (* functor[pos] = expr*)
  sprintf "%s=%s.%s" 
    (string_of_expr expr) 
    n
    (alloy_field_name (get_field_name f_name pos))

let string_of_term term = match term with
    MExpr(expr) -> string_of_expr expr
  | MAssign( varname, expr, _) ->
      sprintf "%s=%s" varname (string_of_expr expr)
  | MFunctor(f) ->
      let (fname, exprs, negate, _) = f in
      let relation = String.concat "->" (List.map string_of_expr exprs) in
        sprintf "%s %s in %s" 
          relation
          (if negate then "not" else "")
          fname;;
        
let string_of_vars var_types: string list =
  let string_of_var var var_type l=
    (sprintf "%s:%s" (alloy_variable_name var) (string_of_primitive_type var_type))::l
  in
    Hashtbl.fold string_of_var var_types [];;

let print_eca_fwd buf var_types ( (etype, efunc), body, (atype, afunc) ) =
  let (e_name, e_args, _, _) = efunc in
  let (a_name, a_args, _, _) = afunc in
  bprintf buf "fact {\n";
    (* for all event, and all variables*)
  bprintf buf "  (all ";
  let str_event = sprintf "e:%s" (alloy_event_name e_name) in
  let str_all_vars = string_of_vars var_types in
    bprintf buf "%s |\n" (String.concat ", " (str_event::str_all_vars) );
    (* bound the event to the vars and constants *)
    let ev_constraints = Array.to_list 
      (Array.mapi (bound_functor_field e_name "e") (Array.of_list e_args))
    in
    (* conditions are met*)
    let conditions = List.map string_of_term body in
      bprintf buf "    (%s)\n    =>\n" (String.concat " && " (ev_constraints @ conditions));
      (* one action satisfies*)
      bprintf buf "    ( one a:%s | \n" (alloy_event_name a_name);
      let action_constraints = Array.to_list 
        (Array.mapi (bound_functor_field a_name "a") (Array.of_list a_args))
      in
        bprintf buf "      %s\n" (String.concat " && " ("a.cause=e"::action_constraints));
        bprintf buf "    )\n";
        bprintf buf "  ) --end of rule\n"; 
        bprintf buf "} -- end of fact\n";;

(* the fwd direction: if rule body is true, then rule head is true*)
let print_fwd buf clause = match clause with 
    MEcaRule( eca, loc) ->
      print_eca_fwd 
        buf 
        (Hashtbl.find Types.rule_types (Types.get_rule_name clause)
        )
        eca
  | MViewRule(rule_name, rule_head, rule_body, loc) ->
      ()
  | _ -> ();;

let string_of_eca_rwd_one_case 
    ( ( (etype, efunc), body, (atype, afunc) ), clause) =
  (* (some e:Event | a.cause =e && 
         (some vars:vartypes|conditions && actions)) *)
  let buf = Buffer.create 100 in
  let (e_name, e_args, _, _) = efunc in
  let (a_name, a_args, _, _) = afunc in
  let var_types = Hashtbl.find Types.rule_types (Types.get_rule_name clause) in
    bprintf buf "(some e:%s | a.cause=e && " (alloy_event_name e_name);
    bprintf buf "  (some %s | " (String.concat ", " (string_of_vars var_types) );
    let conditions = List.map string_of_term body in
    let action_constraints = Array.to_list 
      (Array.mapi (bound_functor_field a_name "a") (Array.of_list a_args))
    in
      bprintf buf "(%s) ) )\n" (String.concat " && " (conditions@action_constraints) );
      Buffer.contents buf;;

let print_eca_rwd buf action_lookup a_name symbol = match symbol with
    MDef(MEventType(_), _, _) ->
      bprintf buf "fact {\n";
      let all_rules = Hashtbl.find_all action_lookup a_name in
        (match all_rules with
            [] -> (
              match a_name with
                  "initEvent" -> ()
                | _ -> bprintf buf "  no %s\n" (alloy_event_name a_name)
            )
          | _ -> begin
              bprintf buf "  ( all a:%s |\n" (alloy_event_name a_name);
              bprintf buf "    (%s)\n"  
                (String.concat "    ||\n    "
                   (List.map string_of_eca_rwd_one_case all_rules)
                );
              bprintf buf "  )\n";
            end
        );
        bprintf buf "} --end of fact \n"
  | _ -> ();;

let print_clauses buf program = 
(* 
  for ECA rules, first print forward alloy facts:
    for all events, if the conditions are satisified, then there is one action;
  then print backward alloy facts:
    for all actions, there must be an event as the cause, and the conditions are satisified.
  backward facts are "or" if an action can be triggered by multiple rules.
*)

  let clauses = snd program in
    bprintf buf " -- forward conditions\n";
    List.iter (print_fwd buf) clauses;
    bprintf buf " -- backward conditions\n";
    let action_lookup: (string, (ecaRule* mclause) ) Hashtbl.t = Hashtbl.create 100 in
    let insert_action clause = match clause with
        MEcaRule( (e,c,a), _) -> 
          let (_,(a_name, _, _,_)) = a in
            Hashtbl.add action_lookup a_name ((e,c,a),clause)
      | _ -> ()
    in
      List.iter insert_action clauses;
      Hashtbl.iter (print_eca_rwd buf action_lookup) symbols;;
