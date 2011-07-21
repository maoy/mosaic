(* $Id: eca.ml 212 2009-04-14 22:17:25Z maoy $ *)

open Cabs
open Printf
open Util
open Symbol

(* rewrite special ECA rules to regular ECA rules:
   periodic, aggeca, aggorderby;
   and set remote location field;
   rewrite remote DB action to an extra remote event + local db action;
*)

let set_location (action:ecaAction) :ecaAction = 
  let (atype, afunc ) = action in
    match atype with 
        ACT_SEND -> begin
          try
            let (_,args,_,_)= afunc in
              (ACT_REMOTE_SEND(find_locspec args), afunc)
          with Not_found
              -> (ACT_LOCAL_SEND, afunc)
        end
      | _ -> action;;



(*when insert/delete action is on a remote node, rewrite it to
 a send action to a new event: ms_[ins/del]_table, and generate
the extra rule ins_table => insert table; del_table => delete table 
if haven't already*)
let extraRemoteDBRules :(ecaActionType*string, string) Hashtbl.t 
    = Hashtbl.create 10;;

let localizeDBAction = function
  | ACT_INSERT -> ACT_LOCAL_INSERT
  | ACT_DELETE -> ACT_LOCAL_DELETE
  | _ -> merror Internal_error "not db action";;


(*generate n variables with name MS_varN-1, .. MS_var0.*)
let rec generate_var_list n :mexpression list =
  match n with
      0 -> []
    | _ -> 
        let varname = sprintf "MS_var%d" (n-1) in
          MVariable(varname)::(generate_var_list (n-1) );;

let remoteDBAction act_type act_func: ecaAction*ecaRule list =
  let (name, args, negate, loc) = act_func in
    try let addr = find_locspec args in
      try let newname = Hashtbl.find extraRemoteDBRules (act_type, name) in
        ((ACT_REMOTE_SEND(addr), (newname, args, negate, loc) )
           , [])
      with Not_found ->
        let newname = sprintf "ms_%s_%s" (action_type_to_string act_type) name in
          Hashtbl.add extraRemoteDBRules (act_type,name) newname;
          (*add definition of the new event*)
          let tuple_type = (get_table_tuple_type name) in
            add_event newname tuple_type;
            let gen_args = generate_var_list (List.length tuple_type) in
              ((ACT_REMOTE_SEND(addr), (newname, args, negate, loc) )
                 , [( (EV_RECV, (newname, gen_args, false, loc) ), (*rule head*)
                      [], 
                      (localizeDBAction act_type, (name, gen_args, false, loc) )
                    ) ]
              )
    with Not_found -> (* not remote DB Action, do nothing*)
      let local_type = localizeDBAction act_type in
        ((local_type,act_func), []);;

let rec eca2ecas ecaRules :ecaRule list =  
  List.concat (List.map eca2eca ecaRules)
and periodic_rewrite ((ev_type, (pname, args, negate, loc) ), body,action) =
(* format: periodic([Sequence,] interval, repeat=0, delay=0) *)
  if ev_type != EV_RECV then 
    mlocerror Compile_error 
      "you can only use periodic with receive event" 
      loc
  else if negate then
    mlocerror Compile_error 
      "the periodic predicate cannot be negative" 
      loc
  else begin
    (* extract interval, repeat and delay *)
    let var, newargs = match args with
        MVariable( var )::rest -> (var, rest)
      | _ -> ("_", args) 
    in
       let periodic_info =
         match newargs with
             [ MConstant( M_CONST_INT (intv) )] 
           | [ MConstant( M_CONST_FLOAT (intv) )] 
             ->
               {interval=intv;repeat="0";delay="0"}
           | [ MConstant( M_CONST_INT (intv) );
               MConstant( M_CONST_INT (rep) )] 
           | [ MConstant( M_CONST_FLOAT (intv) );
               MConstant( M_CONST_INT (rep) )] 
             ->
               {interval=intv;repeat=rep;delay="0"}
           | [ MConstant( M_CONST_INT (intv) );
               MConstant( M_CONST_INT (rep) );
               MConstant( M_CONST_INT (delay) )] 
           | [ MConstant( M_CONST_FLOAT (intv) );
               MConstant( M_CONST_INT (rep) );
               MConstant( M_CONST_INT (delay) )] 
             ->
               {interval=intv;repeat=rep;delay=delay}
           | _ -> mlocerror Compile_error "wrong usage of periodic" loc
       in
      Hashtbl.add periodics (!periodic_count) periodic_info;
      (* define a periodic tuple *)
      let newname = sprintf "ms_periodic%d" !periodic_count in
        Hashtbl.add Symbol.symbols newname 
          (MDef(MEventType([("seq",MTint);("timer",MUserType("boost::asio::deadline_timer*"))]), newname, loc));
        let ev_p = (EV_PERIODIC( periodic_info), (newname, [MVariable(var);MVariable("_")], false, loc)) in
          periodic_count := !periodic_count +1;
          let newrule = (ev_p, body, action) in
            eca2eca newrule
  end (*end of periodics *)
and eca2eca (eca_rule:ecaRule) : ecaRule list = 
  let ((ev_type, func), body, (act_type, act_func) ) = eca_rule in
    match functorname func with
        "periodic" -> periodic_rewrite eca_rule
      | _ -> begin  match act_type with
            ACT_SEND -> (* convert it to LOCAL_SEND or REMOTE_SEND *)
              let (act_name, _, _, act_loc) = act_func in
                is_event_assert act_name act_loc;
                eca2eca 
                  ( (ev_type, func), body, set_location (act_type,act_func) )
          | ACT_INSERT | ACT_DELETE ->
              let (act_name, _, _, act_loc) = act_func in
                is_tableorview_assert act_name act_loc;
                let (newAction, extraRules) = remoteDBAction act_type act_func in
                let newrule = ((ev_type,func), body, newAction) in
                  (eca2eca newrule) @ extraRules
          | ACT_ORDERBY (_) | ACT_AGGVIEW(_) -> [eca_rule]
          | ACT_LOCAL_INSERT | ACT_LOCAL_DELETE
          | ACT_REMOTE_SEND _ | ACT_LOCAL_SEND ->
              let (actName, actArgs, _, actLoc) = act_func in
                match expr_list_has_agg actArgs with
                    [] -> [eca_rule]
                  | agglist -> 
                      mlocerror Internal_error 
                        "not support eventagg yet" actLoc  (*FIXME*)
        end
