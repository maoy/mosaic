(* $Id: printcpp.ml 212 2009-04-14 22:17:25Z maoy $ *)

open Cabs
open Symbol
open Util
open Printf

let rec string_of_primitive_type t =
  match t with
      MTint -> "int32_t"
    | MTstring -> "gc_string"
    | MTdouble -> "double"
    | MUserType(t) -> t
    | MTlong -> "int64_t"
    | MTemplate(t,tlist) ->
        sprintf "%s< %s >" t (String.concat ", " (List.map string_of_primitive_type tlist))
    | MUnknownType 
    | MTableType(_)
    | MViewType(_)
    | MEventType(_)
      -> merror Internal_error "unknown primitive type"

let print_comma_and buf f v = 
  bprintf buf ", "; f buf v

let print_comma_sep_list buf l f = (*f: print element; l : list*)
  match l with
      [] -> ()
    | h::t ->
        f buf h;
        List.iter (print_comma_and buf f) t
  
let print_tuple_type buf tupletype = 
  bprintf buf "tuple< %s >" 
    (String.concat ", " (List.map string_of_primitive_type tupletype) );;

let print_tuple_def buf name tupletype =
  let realtuple= snd (List.split tupletype) in
    bprintf buf "typedef ";
    print_tuple_type buf realtuple;
    bprintf buf " type_tuple_%s;\n" name;
    bprintf buf "DEFINE_TUPLE( %s )\n" name;;

let print_keys buf (keylist:mkey) =
  bprintf buf "Keys< %s >" (string_of_key keylist);;	  

let print_tuple_id buf name def = 
  match def with
      MDef( MTableType(tupletype, _,_, _), name, _) 
    | MDef( MViewType(tupletype, _,_,_), name, _)
    | MDef( MEventType(tupletype), name, _) ->
	bprintf buf "  ,ID_Tuple_%s\n" name
    | _ -> ()

let print_tuple_ids buf =
  bprintf buf "enum {\n  ID_unused = TUPLE_RESERVED\n";
  Hashtbl.iter (print_tuple_id buf) symbols;
  bprintf buf "};\n"


let event_handlers = Hashtbl.create 100

let get_handler_id ev_type name = 
  try
    let c = Hashtbl.find event_handlers (ev_type,name) in
    let n = c+1 in
      Hashtbl.replace event_handlers (ev_type,name) n;
      n
  with Not_found ->
    Hashtbl.add event_handlers (ev_type,name) 0;
    0;;

(*given event_type and event_name,
  generate a new handler name if id<0,
  or return the existing handler name based on id
*)
let get_handler_name ev_type (name:string) (id:int) :string =
  let hid = 
    if id >=0 then id 
    else get_handler_id ev_type name 
  in
    sprintf "%s_%s_handler_%d" (ev_type2str ev_type)  name hid;;



let var_binding = Hashtbl.create 100
let level = ref 0

let unnamed_aggvar_count = ref 0;;
let create_unnamed_aggvariable () = 
  unnamed_aggvar_count := !unnamed_aggvar_count +1;
  sprintf "ms_agg_%d" !unnamed_aggvar_count;;


let print_indent buf =
  Buffer.add_string buf (String.make ((!level + 1)*2) ' ')

let init_var_binding () =
  Hashtbl.clear var_binding;;

let add_one_binding buf (b:bindingTerm) =
  let (varname, pos, t) = b in
    try
      let s = Hashtbl.find var_binding varname in
        merror Internal_error (sprintf "variable already is bounded to %s" s)
    with Not_found ->
      let s = sprintf "boost::get<%d>(*t%d)" pos !level in
        (*debug (sprintf "binding %s to %s" varname s);*)
        match t with
            MUnknownType ->
              Hashtbl.add var_binding varname s;
          | _ ->
              Hashtbl.add var_binding varname varname;
              print_indent buf;
              bprintf buf "%s const& %s = %s;\n" (string_of_primitive_type t) varname s;;

let add_binding buf (bl:bindingTerm list) :unit =
  List.iter (add_one_binding buf) bl;;

let add_agg_modifier_to_varname aggtype varname =
 sprintf "%s_%s" (string_of_aggtype aggtype) varname;;

let rec string_of_expr arg: string=
    match arg with
	MVariable(varname) ->
	  begin
            match varname with
                "_" -> merror Internal_error "DONT CARE variable should have been elliminated"
              | _ -> begin
                  if is_defined varname then varname else
                    try
                      Hashtbl.find var_binding varname
                    with Not_found -> begin
                      if String.contains varname ':' then
                        varname
                      else
                        merror Internal_error ("uninitialized variable:" ^ varname)
                    end
                end
          end
      | MConstant(c) -> begin
	  match c with
	      M_CONST_INT(s) -> s
	    | M_CONST_STRING(s) -> sprintf "gc_string(\"%s\")" (String.escaped s)
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
      | MAggregation(aggtype, MVariable(varname)) -> begin
          let name = add_agg_modifier_to_varname aggtype varname in
            try
              Hashtbl.find var_binding name
            with Not_found -> begin
              merror Internal_error ("uninitialized aggregation:" ^ name)
            end
        end
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
            (string_of_primitive_type t) (string_of_expr ex)
      | MDynamicCast(t,ex) -> 
          sprintf "dynamic_cast<%s>(%s)" 
            (string_of_primitive_type t) (string_of_expr ex)
      | MQuestion(q,ex1,ex2) ->
          sprintf "%s?%s:%s" (string_of_expr q) (string_of_expr ex1) (string_of_expr ex2)
      | MIndex(ex1,ex2) ->
          sprintf "%s[ %s ]" (string_of_expr ex1) (string_of_expr ex2)

and string_list_of_exprs args =
  List.map string_of_expr args
and string_of_exprs args = 
  String.concat ", " (string_list_of_exprs args);;

let add_assignment_binding buf varname expr = 
    try
      let s = Hashtbl.find var_binding varname  in
        merror Internal_error (sprintf "variable already is bounded to %s" s)
    with Not_found ->
      let s = string_of_expr expr in
        (*Hashtbl.add var_binding varname s;*)
        Hashtbl.add var_binding varname varname;
        print_indent buf;
        bprintf buf "typeof(%s) const %s = %s;\n" s varname s;;

let print_def buf name def = 
  match def with
      MDef( MTableType(tupletype, key, sec_keys, MRefTable(needRefCount)), name, _) ->
        print_tuple_def buf name tupletype;
        bprintf buf 
          "typedef RefTable< Tuple_%s," name ;
        print_keys buf key;
        (*fprintf stderr "second key len=%d\n" (List.length !sec_keys);*)
        if List.length !sec_keys > 0 then begin
          bprintf buf ", ";
          print_comma_sep_list buf !sec_keys print_keys;
        end;
        bprintf buf " > Table_%s;\n" name;
        bprintf buf "Table_%s t_%s(&taskQ, %b);\n" name name !needRefCount
    | MDef( MViewType(tupletype, key, sec_keys, MRefTable(needRefCount)), name, _) ->
        print_tuple_def buf name tupletype;
        bprintf buf 
          "typedef RefTable< Tuple_%s," name ;
        print_keys buf !key;
        (*fprintf stderr "second key len=%d\n" (List.length !sec_keys);*)
        if List.length !sec_keys > 0 then begin
          bprintf buf ", ";
          print_comma_sep_list buf !sec_keys print_keys;
        end;
        bprintf buf " > Table_%s;\n" name;
        bprintf buf "Table_%s t_%s(&taskQ, %b);\n" name name !needRefCount
    | MDef( MTableType(tupletype, key, sec_keys, MSoftTable(expire,maxSize)), name, _) ->
        print_tuple_def buf name tupletype;
        bprintf buf 
          "typedef SoftTable< Tuple_%s," name ;
        print_keys buf key;
        if List.length !sec_keys > 0 then begin
          bprintf buf ", ";
          print_comma_sep_list buf !sec_keys print_keys;
        end;
        bprintf buf " > Table_%s;\n" name;
        bprintf buf "Table_%s t_%s(TimeDuration::fromLong(%d),%d,&taskQ);\n" 
          name name expire maxSize
    | MDef( MEventType(tupletype), name, _) ->
	print_tuple_def buf name tupletype
    | MConst( c_type, varname, value, _) ->
        bprintf buf "const %s %s = %s;\n" 
          (string_of_primitive_type c_type) varname (string_of_expr value);
    | _ -> merror Internal_error "not definition";;

let print_defs buf =
  print_tuple_ids buf;
  Hashtbl.iter (print_def buf) symbols;;

let print_debug_action actType args buf =
  let action_arg_str = String.concat "<<' '<<" (string_list_of_exprs args) in
    print_indent buf;
    bprintf buf "std::cout << \"[%s]\" << %s << std::endl;\n" 
      (action_type_to_string actType) action_arg_str

(*let print_orderby_action (action:stmtAction) buf =
  let (actType, name, args, loc ) = action in
  let action_arg_str = string_of_exprs args in
    match actType with 
	ACT_INSERT -> 
	  bprintf buf "t_%s.insert( new Tuple_%s(%s));\n" name name action_arg_str
      | ACT_DELETE ->
	  bprintf buf "t_%s.erase( new Tuple_%s(%s));\n" name name action_arg_str
      | ACT_LOCAL_SEND ->
	  bprintf buf "local_send_action(new Tuple_%s(%s));\n" name action_arg_str
      | ACT_REMOTE_SEND(l) ->
          bprintf buf "send_action(new Tuple_%s(%s, %s);\n" 
            name action_arg_str (string_of_expr l )
      | _ -> merror Internal_error "unknown action type"
*)
let print_action (action:stmtAction) buf =
  let actType, name, args, loc = action in match actType with
      ACT_ORDERBY(_,_,_,_) -> 
        (*print_orderby_action action buf*)
        merror Internal_error "orderby should be printed else where"
    | _ -> match name with
          "print" ->
            print_debug_action actType args buf;
        | "dump" -> begin
            match args with
                [MVariable(tname)] ->
                  if is_defined tname then begin
	            print_indent buf;
                    bprintf buf "t_%s.dump();\n" tname
                  end else
                    merror Compile_error "illegal useage of dump: arg should be a table name";
              | _ -> merror Compile_error "illegal useage of dump: arg should be a table name";
          end
        | "exit" -> begin
            print_indent buf;
            bprintf buf "exit( ";
            begin
              match args with
                  [] -> ()
                | [arg] -> bprintf buf "%s" (string_of_expr arg )
              | _ -> merror Compile_error "wrong usage of exit( errcode=0 )"
            end;
            bprintf buf " );\n"
          end
        | _ -> 
            let action_arg_str = string_of_exprs args in
	      print_indent buf;
	      match actType with 
	          ACT_LOCAL_INSERT -> 
	            bprintf buf "t_%s.insert( new Tuple_%s(%s));\n" name name action_arg_str;
	        | ACT_LOCAL_DELETE ->
                    bprintf buf "t_%s.erase( new Tuple_%s(%s));\n" name name action_arg_str;
                | ACT_LOCAL_SEND ->
	            bprintf buf "local_send_action(new Tuple_%s(%s));\n" name action_arg_str
                | ACT_REMOTE_SEND(l) ->
                    bprintf buf "send_action(new Tuple_%s(%s), %s);\n" 
                      name action_arg_str (string_of_expr l )
                | ACT_AGGVIEW(_) ->
	            bprintf buf "t_%s.insert( new Tuple_%s(%s));\n" name name action_arg_str;
                | ACT_SEND | ACT_INSERT | ACT_DELETE | ACT_ORDERBY(_) ->
                    merror Internal_error "no more act_send/insert/delete"

(* print the join key in the following format:
  key_type key(field0, field2,...);
  typedef Keys<0,2,..> key_##level;
   Note that the key is allocated on the stack.
*)
let bprint_init_join_key buf name jts = 
  print_indent buf;
  let join_positions, join_values = List.split jts in
    bprintf buf "typedef Keys<%s> key_type_%d;\n" 
      (string_of_key join_positions) !level;
    print_indent buf;
    match jts with
        [] -> merror Internal_error ("join key is empty in" ^ name )
      | [ (pos, value)] ->
          (* special case if the key length is 1: key is not a tuple,
             but the actual field *)
          let keytype = get_table_field_type name pos in
            bprintf buf "%s const& " (string_of_primitive_type keytype);
            bprintf buf " ms_t%d_key( %s );\n" !level
              (string_of_expr value)
      | _ ->
          (*first figure out key type*)
          let joinkey_tuple = 
            List.map ( get_table_field_type name ) join_positions in
            print_tuple_type buf joinkey_tuple;
            bprintf buf " ms_t%d_key( %s );\n" !level
              (string_of_exprs join_values);;

let rec print_stmt_terms terms (action:stmtAction) buf =
  level := (!level)+1; 
  begin
  match terms with
      [] -> print_action action buf
    | term :: rest_terms ->
	match term with
            MPrimaryJoin(name, jts, bts, loc) -> begin
              bprint_init_join_key buf name jts;
	      print_indent buf;
	      bprintf buf "TABLE_PRIMARY_LOOKUP(%s,%d,&ms_t%d_key){\n" name !level !level;
              add_binding buf bts;
              print_stmt_terms rest_terms action buf;
              print_indent buf;
              bprintf buf "}\n" 
            end
          | MSecondaryJoin(name, skey, jts, bts, loc) -> begin
              bprint_init_join_key buf name jts;
	      print_indent buf;
	      bprintf buf "TABLE_SECONDARY_LOOKUP(%s,%d,&ms_t%d_key,key_type_%d){\n" 
                name !level !level !level;
              add_binding buf bts;
              print_stmt_terms rest_terms action buf;
              print_indent buf;
              bprintf buf "}\n" 
            end
          | MQuery(name, bts, loc) -> begin
	      print_indent buf;
	      bprintf buf "TABLE_FOREACH(%s,%d){\n" name !level;
              add_binding buf bts;
              print_stmt_terms rest_terms action buf;
              print_indent buf;
              bprintf buf "}\n" 
            end
          |   MNegatePrimaryJoin(name, jts, bts, loc) -> begin
              bprint_init_join_key buf name jts;
	      print_indent buf;
	      bprintf buf "TABLE_PRIMARY_NOTIN(%s,%d,&ms_t%d_key){\n" name !level !level;
              add_binding buf bts;
              print_stmt_terms rest_terms action buf;
              print_indent buf;
              bprintf buf "}\n" 
            end
          | MNegateSecondaryJoin(name, skey, jts, bts, loc) -> begin
              bprint_init_join_key buf name jts;
	      print_indent buf;
	      bprintf buf "TABLE_SECONDARY_NOTIN(%s,%d,&ms_t%d_key,key_type_%d){\n" 
                name !level !level !level;
              add_binding buf bts;
              print_stmt_terms rest_terms action buf;
              print_indent buf;
              bprintf buf "}\n" 
            end
          | MFilter([]) | MNegFilter([])->
              print_stmt_terms rest_terms action buf;
	  | MFilter(exprs) -> begin
	      print_indent buf;
              bprintf buf "if ( %s ){\n" 
                (String.concat " && " (string_list_of_exprs exprs) );
              print_stmt_terms rest_terms action buf;
              print_indent buf;
              bprintf buf "}\n"
            end
	  | MNegFilter(exprs) -> begin
	      print_indent buf;
              bprintf buf "if ( !(%s) ){\n" 
                (String.concat " && " (string_list_of_exprs exprs) );
              print_stmt_terms rest_terms action buf;
              print_indent buf;
              bprintf buf "}\n"
            end
          | MAssignment(varname, expr, loc) -> begin
              add_assignment_binding buf varname expr;
              print_stmt_terms rest_terms action buf;
            end
          | MAggTerm(tablename, gkey, aggpos, varname, aggtype) -> begin
              print_indent buf;
              let aggvar = create_unnamed_aggvariable () in
                bprintf buf "Aggregate<Table_%s, Keys<%s>,%d> %s(&t_%s, t0);\n"
                tablename (string_of_key gkey) aggpos aggvar tablename;
              let agg_expr = sprintf "%s.%s()" aggvar (string_of_aggtype aggtype) in
              let agg_bindingname = add_agg_modifier_to_varname aggtype varname in
                Hashtbl.add var_binding agg_bindingname agg_expr;
                print_stmt_terms rest_terms action buf;
            end
  end;
  level := (!level)-1;;
  
let is_bindingTerm_at_pos pos (_,idx,_)=
  idx!=pos;;

let print_orderby_rule (ev_type, name, evArgs, bindings, eloc) body 
    ( (groupby:mkey), orderby, groupbyHead, otype)
    actName actArgs actLoc buf = 
  if body != [] then
    mlocerror Internal_error "aggregation rule must only have 1 term in the body" eloc;
  print_indent buf;
  (*groupby and orderby key are inferred from variable bindings in orderby_rewrite *)
  bprintf buf "Aggregate<Table_%s, Keys<%s>,%d> agg(&t_%s, t0);\n" 
    name (string_of_key groupby) orderby name;
  print_indent buf;
  begin match otype with
      M_ORDER_MIN ->
        bprintf buf "Tuple_%s* t1 = agg.min_tuple();\n" name;
    | M_ORDER_MAX ->
        bprintf buf "Tuple_%s* t1 = agg.max_tuple();\n" name;
  end;
  print_indent buf;
  bprintf buf "if (t1) {\n    t0 = t1;\n";
  let (non_orderby_bindings, orderby_bindings) =
    List.partition (is_bindingTerm_at_pos orderby) bindings in
    (* now bind those variables other than the orderby field*)
    add_binding buf non_orderby_bindings;
    (* bind the orderby field *) 
    begin
      match orderby_bindings with
          [(orderby_varname, orderby, vtype)] ->
            begin match otype with
                M_ORDER_MIN ->
                  add_one_binding buf ( add_agg_modifier_to_varname M_AGG_MIN orderby_varname,
                                        orderby, vtype);
              | M_ORDER_MAX ->
                  add_one_binding buf ( add_agg_modifier_to_varname M_AGG_MAX orderby_varname,
                                        orderby, vtype);
            end
        | _ -> merror Internal_error "no or more than 1 order by bindings??";
    end;
    print_indent buf;
    let action_arg_str = string_of_exprs actArgs in
      bprintf buf "  t_%s.insert( new Tuple_%s(%s));\n" actName actName action_arg_str;
      print_indent buf;
      bprintf buf "} else {\n";
      print_indent buf; 
      bprintf buf "  Tuple_%s* t2 = new Tuple_%s();\n" actName actName;
      let keylist = List.combine groupbyHead groupby in 
      List.iter (fun (i,j) -> 
                   (
                     print_indent buf;
                     bprintf buf "  t2->get<%d>()=t0->get<%d>();\n" i j
                   ) )
        keylist;
      print_indent buf;
      bprintf buf "  t_%s.eraseByKey( t2 );\n" actName;
      print_indent buf;
      bprintf buf "}\n";;


let print_stmt buf (event, body, action, _) =
  init_var_binding ();
  let ev_type, event_name, ev_args, bindings, evloc= event in
  let n = get_handler_name ev_type event_name (-1) in
    bprintf buf "//# %d \"%s\"\n" evloc.lineno evloc.filename;
    level := 0;
    bprintf buf "void %s(Tuple* ms_t){\n" n;
    print_indent buf;
    bprintf buf "Tuple_%s *t0 = static_cast<Tuple_%s*>(ms_t);\n" event_name event_name;
    let (actType, actName, actArgs, actLoc) = action in begin
        match actType with
            ACT_ORDERBY(groupby, orderby,groupbyHead,otype) ->
              print_orderby_rule (ev_type, event_name, ev_args, bindings, evloc) body
                (groupby, orderby, groupbyHead, otype) actName actArgs actLoc buf
          | _ ->
              add_binding buf bindings;
              print_stmt_terms body action buf;
      end;
      begin
        match ev_type with 
            EV_PERIODIC( pinfo ) -> 
              if int_of_string pinfo.repeat >0 then begin
                bprintf buf "  if (boost::get<0>(*t0) >= %s) return;\n" pinfo.repeat;
              end;
              bprintf buf "  boost::get<0>(*t0)++ ;\n";
              bprintf buf "  Event _e(Event::RECV, t0);\n";
              bprintf buf "  callLater(%s, boost::bind(demux, _e), boost::get<1>(*t0));\n" pinfo.interval
          | _ -> ()
      end;
      bprintf buf "}\n\n"

let ev_type_to_string = function
  | EV_INSERTED -> "INSERTED"
  | EV_RECV | EV_PERIODIC(_)-> "RECV"
  | EV_DELETED -> "DELETED";;

let print_demux_handler buf (ev_type, name) count =
  bprintf buf "  else if ( (e.type & Event::%s) && (e.t->type_id==ID_Tuple_%s) ){\n"
    (ev_type_to_string ev_type) name;
  for i = 0 to count do
    bprintf buf "    %s(e.t);\n" (get_handler_name ev_type name i)
  done;
  bprintf buf "  }\n";;

let print_demux buf = 
  bprintf buf "void demux_handler(Event e){\n  if (false) {}\n";
  Hashtbl.iter (print_demux_handler buf) event_handlers;
  bprintf buf "}\n\n";;

let print_include buf inc =
  match inc with
      MFact(("include",args,_,_)) -> begin
        match args with
           [ MConstant(M_CONST_STRING(name))]
              -> bprintf buf "#include \"%s\"\n" name
          | _ -> debug "wrong usage of table include()"; raise Internal_error 
      end
    | _-> ()


let print_fact buf fact =
  match fact with
      MFact(("include",_,_,_)) -> ()
    | MFact((name, args, false, loc)) ->
	let args_str = string_of_exprs args in
	bprintf buf "  t_%s.insert( new Tuple_%s( %s ) );\n" name name args_str
    | MFact(name, args,true, loc) ->
        mlocerror Compile_error 
          (sprintf "fact `%s' cannot be a negative term." name) loc
    | _ -> ();;

let print_facts buf program =
  bprintf buf "void facts(){\n";
  List.iter (print_fact buf) (snd program);
  bprintf buf "}\n";;

(* CSV related *)
exception UnsupportedCSVType

let string_of_csv_field i (f_name, f_type) =  
  sprintf "from_string_cast<%s>(tokens.at(%d).c_str())" 
    (string_of_primitive_type f_type) (i+1);;

(*
match f_type with
    MTstring | MUserType("String") 
      -> sprintf "tokens.at(%d)" (i+1)
  | MTint | MTlong | MTdouble 
  | MUserType("uint64_t") | MUserType("uint32_t")
  | MUserType("int64_t") | MUserType("int32_t") ->
      sprintf "boost::lexical_cast<%s>(tokens.at(%d))" (string_of_primitive_type f_type) (i+1)
  | MUserType("Integer") ->
      sprintf "boost::lexical_cast<int>(tokens.at(%d))" (i+1)
  | MUserType("UDPAddress") ->
      sprintf "UDPAddress::fromString(tokens.at(%d))" (i+1)
  | _ -> 
      debug (sprintf "warning: %s is not supported by csv" (string_of_primitive_type f_type) );
      raise UnsupportedCSVType
*)
      
let string_of_csv_fields tuple_type=
  String.concat ", " 
    (Array.to_list (Array.mapi string_of_csv_field (Array.of_list tuple_type) ) )
let print_csv_insert buf name def =
  match def with
      MDef( MTableType(tupletype, _,_,_), name, _) -> begin
        bprintf buf "      if (tokens.at(0)==\"%s\"){\n" name;
        (try
          let conversion = string_of_csv_fields tupletype in
            bprintf buf "        t_%s.insert( new Tuple_%s( %s ) );\n" name name conversion
        with UnsupportedCSVType -> 
          bprintf buf "        throw std::runtime_error(\"unsupported type in %s\");\n" name;
        );
        bprintf buf "      }\n"
      end
    | _ -> ()

let print_csv_facts buf =
  bprintf buf "void csv_facts(const char* fn){\n";
  bprintf buf "  CSVLoader l; if (l.open(fn)) {\n";
  bprintf buf "    while (!l.eof()) {\n";
  bprintf buf "      string_vector tokens = l.next();\n";
  bprintf buf "      if ( tokens.size()<2 ) continue;\n";
  Hashtbl.iter (print_csv_insert buf) symbols;
  bprintf buf "    }\n";
  bprintf buf "  } else std::cerr << \"Cannot open CSV file \" << fn << std::endl;\n";
  bprintf buf "}\n"

let print_periodic_init buf index info =
  bprintf buf "  {\n";
  bprintf buf "    static boost::asio::deadline_timer tm(iosv);\n";
  bprintf buf "    static Tuple_ms_periodic%d t%d(1, &tm);\n" index index;
  bprintf buf "    Event e(Event::RECV, &t%d);\n" index;
  bprintf buf "    callLater(%s, boost::bind(demux, e),&tm);\n" info.delay;
  bprintf buf "  }\n"

let print_periodic_inits buf =
  bprintf buf "void register_periodics(){\n";
  Hashtbl.iter (print_periodic_init buf) periodics;
  bprintf buf "}\n\n";;

let print_tuple_s11n_init buf name t = 
  match t with
     MDef(MEventType(_),name,_)->
       let isperiodic = 
         if String.length name >= 11 then 
           if String.compare (String.sub name 0 11) "ms_periodic"==0 then
             true
           else false
         else false
       in
         if not isperiodic then
           bprintf buf "  REGISTER_TUPLE_ARCHIVE(%s,NetIArchive,NetOArchive)\n" name
    | _ -> ()

let print_tuple_s11n_inits buf =
  bprintf buf "void registerAllTuples(){\n";
  Hashtbl.iter (print_tuple_s11n_init buf) symbols;
  bprintf buf "}\n\n";;


