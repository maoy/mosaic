(* $Id: types.ml 212 2009-04-14 22:17:25Z maoy $ *)

open Cabs
open Printf
open Util
open Symbol

let dont_care_count = ref 0

type var2type_t = (string,mtype) Hashtbl.t;;
let rule_types: (string,var2type_t) Hashtbl.t = Hashtbl.create 100;;


let var_types: var2type_t = Hashtbl.create 10;;
let init_var_types () = Hashtbl.clear var_types;;

let rec string_of_type t =
  match t with
      MTint -> "int"
    | MTstring -> "string"
    | MTdouble -> "double"
    | MUserType(t) -> t
    | MTlong -> "long"
    | MTemplate(t,tlist) ->
        sprintf "%s< %s >" t (String.concat ", " (List.map string_of_type tlist))
    | MUnknownType -> "UnInferredType" 
    | MTableType(_)
    | MViewType(_)
    | MEventType(_)
      -> error_loc Internal_error "unknown primitive type in types.ml"

let is_same_type t1 t2 =
  if t1==MUnknownType or t2==MUnknownType then
    true
  else
    (String.compare 
       (*FIXME: lower case is a hack to get around with String vs string *)
       (String.lowercase (string_of_type t1)) 
       (String.lowercase (string_of_type t2))
    )==0

let type_of_variable var =
  if is_defined var then
    (* must be a constant *)
    get_constant_type_from_def var
  else if String.contains var ':' then
    (* C++ built-in variable, ignore it *)
    MUnknownType
  else
    (* not defined, try find it in previously bounded variables *)
    Hashtbl.find var_types var;;
    
let rec type_of_expr = function
  | MVariable("_") ->
      error_loc Compile_error "Cannot have dont care variable in expression"
  | MVariable("*") ->
      MUnknownType (*FIXME *)
  | MVariable(var) -> begin
      try
        type_of_variable var
      with Not_found ->
        error_loc Compile_error
          (sprintf "variable `%s` is not defined in the expression" var)
    end
  | MParen(expr) -> type_of_expr expr
  | MStaticCast(t,expr)
  | MDynamicCast(t,expr)
    -> ignore(type_of_expr expr); t
  | MLocspec(expr) -> type_of_expr expr
  | MMemberOfPtr(e1,_)
  | MMemberOf(e1,_) 
  | MNewVector(e1)
  | MAggregation(_,e1)
  | MUnary(_,e1) -> 
      ignore(type_of_expr e1);MUnknownType
  | MIndex(e1,e2) 
  | MBinary(_,e1,e2) -> 
      ignore(type_of_expr e1);
      ignore(type_of_expr e2);
      MUnknownType
  | MConstant(M_CONST_INT(_))-> MUnknownType (*FIXME: get around with different int types*)
  | MConstant(M_CONST_FLOAT(_))-> MTdouble
  | MConstant(M_CONST_STRING(_)) -> MTstring
  | MConstant(_) -> MUnknownType
  | MQuestion(e1,e2,e3) ->
      ignore(type_of_expr e1);
      ignore(type_of_expr e2);
      type_of_expr e3
  | MCall(_) 
    -> MUnknownType

let check_functor_field allow_free_var func_name field_type expr =
  match expr with
      MVariable("_") -> begin
        dont_care_count := !dont_care_count +1;
        let var = (sprintf "dontcare%d" !dont_care_count) in
          Hashtbl.add var_types var field_type
      end
    | MLocspec( MVariable(var) )
    | MVariable(var) -> begin
        try
          let t = type_of_variable var in
            if not (is_same_type t field_type) then
              error_loc Compile_error
                (sprintf "Type mismatch: expected type `%s` in `%s`, but variable `%s` has been used as type `%s`"
                   (string_of_type field_type) func_name var (string_of_type t)
                )
        with Not_found ->
          if allow_free_var then
            Hashtbl.add var_types var field_type
          else
            error_loc Compile_error
              (sprintf "variable `%s` in `%s` is not defined."
                 var func_name 
              )
      end
    | _ -> 
        let t = type_of_expr expr in
          if not (is_same_type t field_type) then
            error_loc Compile_error
              (sprintf "Type mismatch: expected type `%s` in `%s`, but the expression has been used as type `%s`"
                 (string_of_type field_type) func_name (string_of_type t)
              )

let check_periodic args negate = 
  (*FIXME: more rigorous check for periodic syntax*)
  match args with
      MVariable(sequenceVar)::tl ->
        Hashtbl.add var_types sequenceVar MTint
    | _ -> ()

let check_functor (name, args, negate, loc) allow_free_var =
  current_loc := loc;
  match name with
      "periodic" -> 
        check_periodic args negate
    | "groupBy"|"dump"|"print"|"exit" -> () (*FIXME: should at least check if they are defined*)
    | _ -> begin
        let func_types = get_tuple_type name in
          if (List.length func_types != List.length args) then
            mlocerror Compile_error 
	      (sprintf "number of fields mismatch in term %s" name)
	      loc
          else begin
            List.iter2 (check_functor_field allow_free_var name) func_types args
          end
      end
      
let check_term :mterm -> unit = function
  | MFunctor(func) ->
      check_functor func true
  | MExpr(expr) ->
      (*FIXME: now only check if all variables in expr are binded.
        should check if the type is actually a boolean*)
      ignore (type_of_expr expr)
  | MAssign(var,expr,loc) ->
      current_loc := loc;
      try
        ignore (Hashtbl.find var_types var);
        error_loc Compile_error
          (sprintf "variable %s should not be binded before assignment" var)
      with Not_found ->
        Hashtbl.add var_types var (type_of_expr expr)

let get_rule_name r = 
  let (provided_name,loc) = match r with
      MEcaRule(_,eloc) -> ("",eloc)
    | MViewRule(n, _, _, rloc) -> (n, rloc)
    | _ -> error_loc Internal_error "try to get a rule name on a non-rule clause"
  in match provided_name with
      "" -> (* no specified name, generate one based on filename and line number*)
        sprintf "%s:%d" loc.filename loc.lineno
    | _ -> provided_name;;

let save_rule_types name = 
  if Hashtbl.mem rule_types name then begin
    error_loc Compile_error
      (sprintf "the rule name `%s` is previously used." name)
  end;
  (* a good name not previously used*)
  (* get a copy because the current version will be cleared for next rule*)
  Hashtbl.add rule_types name (Hashtbl.copy var_types);;

let check_clause clause = 
  init_var_types ();
  match clause with
      MEcaRule(( (etype,efunc), body, (atype,afunc) ),loc) -> begin
	check_functor efunc true;
	List.iter check_term body;
	check_functor afunc false;
        (* passed type checking, save it*)
        save_rule_types (get_rule_name clause)
      end
    | MViewRule(rule_name, rule_head, rule_body, loc) -> begin
        List.iter check_term rule_body;
        check_functor rule_head false;
        save_rule_types (get_rule_name clause)
      end
    | _ -> ();;

let check_program program =
  let clauses = snd program in
    List.iter check_clause clauses
