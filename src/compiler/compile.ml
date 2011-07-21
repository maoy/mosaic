(* $Id: compile.ml 212 2009-04-14 22:17:25Z maoy $ *)

open Cabs
open Printf
open Util
open Symbol

let var_binding :var_binding_t = Hashtbl.create 100

let init_var_binding () =
  Hashtbl.clear var_binding;;

let unnamed_var_count = ref 0;;
let create_unnamed_variable () = 
  unnamed_var_count := !unnamed_var_count +1;
  sprintf "ms_unnamed_%d" !unnamed_var_count;;

let get_join_and_binding arg pos (tablename:string) : joinTerm list * bindingTerm list =
  match arg with
      MLocspec( MVariable(varname) ) | MVariable(varname) -> begin
        match varname with
            "_" -> ( [], [] )
          | _ -> begin
              if Symbol.is_defined varname then
                (* join a global variable. e.g. a constant variable *)
                ( [(pos, arg)],[] )
              else
                try
                  ignore( Hashtbl.find var_binding varname );
                  (* found in rule context, it's a join term *)
                  ( [(pos, arg)],[])
                with Not_found ->
                  (* a binding term *)
                  let vartype = get_field_type tablename pos in
                    Hashtbl.add var_binding varname (pos, vartype);
                    ( [], [(varname, pos, vartype)] )
            end
      end
    | _ -> (* not a variable to bind, then it's a join term*)
        ([ (pos, arg) ], [] )
    
let rec get_join_and_bindings_nocheck ?(pos=0) args tablename = 
    match args with
      | [] -> ( [], [] )
      | head_arg::tail_args ->
          let (j1,b1) = get_join_and_binding head_arg pos tablename in
          let (j2,b2) = get_join_and_bindings_nocheck ~pos:(pos+1) tail_args tablename in
            ( List.append j1 j2, List.append b1 b2)

let get_join_and_bindings args tablename =
  let tuple_type = get_tuple_type tablename in
  let l1 = List.length tuple_type in
  let l2 = List.length args in
    if (l1<l2) then
      error_loc Compile_error 
        (sprintf "The query term has more fields (%d) than table `%s' has (%d)." 
           l2 tablename l1)
    else if (l1>l2) then
      warning_loc 
        (sprintf "The query term has less fields (%d) than table `%s' has (%d)." 
           l2 tablename l1);
    get_join_and_bindings_nocheck args tablename;;


let join2filter (jt:joinTerm): mexpression*bindingTerm =
  let v = create_unnamed_variable() in
  let new_bt = (v, fst jt, MUnknownType) in
  let filter = MBinary( M_EQ, snd jt, MVariable(v) ) in
    (filter, new_bt);;

(* at the situation where the table is not available to query the 
   join keys, bound those positions to automatic named variables,
   and create filter expressions.
*)
let joins2filters (jts: joinTerm list) (bts:bindingTerm list) 
    :mexpression list * bindingTerm list =
  let filters, new_bts = List.split( List.map join2filter jts ) in
    (filters, List.append bts new_bts);;

let ecaterm2stmtterms (t:mterm) :stmtTerm list = match t with
    MAssign(v, (e:mexpression) ,loc) ->
      [MAssignment(v,e,loc)]
  | MExpr(expr) ->
      [ MFilter([expr]) ]
  | MFunctor( name, args, negate, loc) ->
      current_loc := loc;
      let (jt, bt) = get_join_and_bindings args name in
      let (key, _) = List.split jt in
      let pkey = Symbol.get_primary_key_from_def name in
      let is_pjoin = (list_subset pkey key) && (List.length pkey>0) in (*primary key is subset of key?*)
        if is_pjoin then begin
          let extra = list_diff key pkey in
          let join_exps = List.map (List.nth args) pkey in
          let new_jts = List.combine pkey join_exps in
          let filters_exp =  List.map (List.nth args) extra in
          let extra_jts = List.combine extra filters_exp in
          let filters, new_bts = joins2filters extra_jts bt in
            if not negate then begin  (*primary key, regular*)
              match filters with 
                  [] -> [MPrimaryJoin(name, new_jts, new_bts, loc)]
                | _ -> [MPrimaryJoin(name, new_jts, new_bts, loc); 
                        MFilter(filters) ]
            end
            else begin(*primary key, negate*)
              if (bt != []) then
                mlocerror Compile_error "negative query cannot have unbounded terms." loc
              else
                match filters with 
                    [] -> [MNegatePrimaryJoin(name, new_jts, new_bts, loc)]
                  | _ -> [MNegatePrimaryJoin(name, new_jts, new_bts, loc); 
                          MNegFilter(filters)]
            end
        end
        else if key==[] then begin
          if negate then
            mlocerror Compile_error "negative query has no join key." loc;
          (* cartesian product, regular *)
          [MQuery(name, bt,loc)]
        end
        else begin
          if not negate then begin
            Symbol.add_secondary_key name key;
            [MSecondaryJoin(name, key, jt, bt, loc)]
          end
          else
            if (List.length bt >0) then
              mlocerror Compile_error "negative query cannot have unbounded terms." loc
            else begin
              Symbol.add_secondary_key name key;
              [MNegateSecondaryJoin(name,key, jt, bt, loc)]
            end
        end;;

let get_variable_at_pos tablename tableargs pos =
  let arg = List.nth tableargs pos in
  let varname = match arg with
        MVariable(v) -> v
      | _ -> merror Internal_error "can't find group by key variable"
  in
  let tp = List.nth (get_table_tuple_type tablename) pos in
    (varname, pos, tp)

let rewrite_action actType aname aargs aloc :stmtTerm list * stmtAction =
  ([],(actType,aname,aargs, aloc));;

let regular2stmt event body action: mstmt =
  let (actType, (aname, aargs, _, aloc) ) = action in
  let (ev_type, (ename, eargs, _, loc) ) = event in
    (* first bound variables in event *)
  let (joinTerms, bindingTerms) = get_join_and_bindings eargs ename 
  in
    (* when event has constants, rewrite them to filter expressions *)
  let filters, new_bts = joins2filters joinTerms bindingTerms in
  let sevent:stmtEvent = (ev_type, ename, eargs, new_bts, loc) in
    (* translate each term in the body *)
  let terms = List.concat (List.map ecaterm2stmtterms body) in
  let saction = (actType, aname, aargs, aloc) in
    (sevent,
     (if joinTerms==[] then terms (* event doesn't have join terms *)
      else MFilter(filters)::terms
     ),
     saction,
    Hashtbl.copy var_binding);;

let generate_aggterm tablename gkey (aggtype, varname) =
  let pos = match aggtype, varname with
      (M_AGG_COUNT, "*") -> 0
    | _ -> try
        fst( Hashtbl.find var_binding varname )
      with Not_found ->
        error_loc Compile_error 
          (sprintf "Variable %s is not bounded in aggregation." varname)
  in
    MAggTerm(tablename, gkey, pos, varname, aggtype)


let aggview2stmt event body action gkey gkey_head :mstmt =
  let (actType, (aname, aargs, _, aloc) ) = action in 
  let (ev_type, (ename, eargs, _, loc) ) = event in
    (* first bound variables in event *)
    (* in aggview: no join term, only bind groupBy position*)
  let (joinTerms, binding_terms) = get_join_and_bindings eargs ename in
    (*init_var_binding (); (* erase the binding in hashtbl *) *)
    if (List.length joinTerms) > 0 then
      mlocerror Compile_error "in aggview there is join term" loc;
    let groupby_bindings = List.map (get_variable_at_pos ename eargs) gkey in
    let sevent:stmtEvent = (ev_type, ename, eargs, groupby_bindings, loc) in
    let saction = (actType, aname, aargs, aloc) in
    let agglist = expr_list_has_agg aargs in
    let aggterms = List.map (generate_aggterm ename gkey) agglist in
      (sevent, aggterms, saction, Hashtbl.copy var_binding)
    (* now generate aggregates *)


let eca2stmt ((event:ecaEvent), body, (action:ecaAction) ) :mstmt =
  init_var_binding (); 
  current_loc := eca_event_loc event;
  let (actType, _ ) = action in match actType with
      ACT_LOCAL_SEND | ACT_LOCAL_INSERT | ACT_LOCAL_DELETE
    | ACT_REMOTE_SEND(_)
    | ACT_ORDERBY(_) ->
        regular2stmt event body action
    | ACT_AGGVIEW(gkey, gkey_head) ->
        aggview2stmt event body action gkey gkey_head
    | ACT_INSERT | ACT_SEND | ACT_DELETE ->
        merror Internal_error "actions should not appear after eca";;

let eca2stmts e :mstmt list =
  List.map eca2stmt e

let program2stmt prog = 
  let clauses = snd prog in
    debug "string interning..";
    let clauses = List.map Intern.eliminate_string_consts clauses in
      debug "localizing..";
      let lclauses = Localize.localize_clauses clauses in
        debug "delta rewrites and aggregation rewrites";
        let ecaRules = View.clauses2ecas lclauses in
          debug "ECA rewrites";
          let ecaRules = Eca.eca2ecas ecaRules in
            debug "statement";
            let s = eca2stmts ecaRules in
              debug "finished!";
              s;;


let test () =
  let program = Frontm.parse_to_cabs "/home/maoy/mosaic/src/mos/bestpath.mos" in
    Symbol.save_symbols program;
    program2stmt program;;
