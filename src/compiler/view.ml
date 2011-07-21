(* $Id: view.ml 212 2009-04-14 22:17:25Z maoy $ *)

open Cabs
open Printf
open Util
open Symbol

(*
rewrite MViewRule to MEcaRule
including delta_rewrite for regular rules,
and orderby_rewrite for min/max aggregation,
and agg_rewrite for other aggregations.
*)

(*let functor_has_agg (_,(_,args,_,_)) = 
  List.length (expr_list_has_agg args) >0;;
*)

(*find the position of the first aggregation field in the list*)
let rec find_first_agg_arg ?(pos=0) rlist : int = 
  match rlist with
      [] -> merror Internal_error "no agg field in a agg rule head?"
    | h::t -> 
        if List.length( expr_has_agg h) > 0 then pos
        else find_first_agg_arg ~pos:(pos+1) t

let rec find_first_agg_var rlist  = 
  match rlist with
      [] -> merror Internal_error "no agg field in a agg rule head?"
    | h::t -> 
        match expr_has_agg h with
            [] -> find_first_agg_var t
          | [(aggtype,expr)] -> expr
          | _ -> raise Compile_error

let delta_rewrite_one curr rest head = 
  match curr with
      MFunctor(tuple_name, args, negate, loc) -> 
        Symbol.is_tableorview_assert tuple_name loc; begin
        match negate with
            false -> 
              [ ((EV_INSERTED, term2functor curr), rest, (ACT_INSERT, head));
	        ((EV_DELETED, term2functor curr), rest, (ACT_DELETE, head)) ]
          | true -> 
              [ ((EV_INSERTED, term2functor curr), rest, (ACT_DELETE, head));
	        ((EV_DELETED, term2functor curr), rest, (ACT_INSERT, head)) ]
        end
    | _ -> []

let rec delta_rewrite body_front body_back head =
  match body_back with
      [] -> []
    | curr::rest ->
        List.append 
          (delta_rewrite_one curr (List.append body_front rest) head)
          (delta_rewrite (List.append body_front [curr]) rest head)

let rec find_idx ?(pos=0) (args:mexpression list) (varname:string) : int=
  match args with
      [] -> raise Not_found
    | MVariable(aname)::tl -> 
        if String.compare aname varname==0 then
          pos
        else
          find_idx ~pos:(pos+1) tl varname
    | _::tl -> find_idx ~pos:(pos+1) tl varname
          
(* check the following:
   term1 should be a functor as the aggregated table;
   term2 should be groupBy() functor indicating the groupBy key
   hn should be the result view name,
   hargs should be the result view arguments:
   extract the groupBy key in term1 sequence, 
   extract the primary key in headview sequence,
   compare them after binding to variables --- they should match. 
   if so, return the groupBy keys in both sequences
*)
let check_group_by term1 term2 (hn,hargs,hloc) :mkey*mkey = match (term1, term2) with
    ((body_name, body_args),  
     MFunctor("groupBy",groupbyVars, false, gloc)) ->
      let groupby_varnames = List.map variable2varname groupbyVars in
      current_loc := gloc;
      let group_by_key = 
        try List.map (find_idx body_args) groupby_varnames
        with Not_found ->
          mlocerror Compile_error 
            (sprintf "groupBy fields cannot be found in table %s" body_name) gloc 
      in
      let group_by_key_head = 
        try List.map (find_idx hargs) groupby_varnames
        with Not_found ->
          mlocerror Compile_error 
            (sprintf "groupBy fields cannot be found in table %s" hn) hloc 
      in
      (* let group_by_key2 = Symbol.get_primary_key_from_def hn in
        if compare (List.sort compare group_by_key_head) group_by_key2 !=0 then
          error_loc Compile_error "The groupBy term must match the primary key of the rule head.";
        (group_by_key, group_by_key_head) (*return value*)         
      *)
        set_view_primary_key hn group_by_key;
        (group_by_key, group_by_key_head)

  | _ ->  error_loc Compile_error
      "The rule body must be a single non-negate table query without filters, followed by a groupBy term indicating groupBy keys.";;
            
let orderby_rewrite (hn,hargs,hloc) otype body loc :ecaRule list= 
  match body with
      [MFunctor(body_name, body_args, false, bloc);
       groupby_term] -> begin
        current_loc := bloc;
        let group_by_key, group_by_key_head = 
          check_group_by (body_name,body_args) groupby_term (hn,hargs,hloc) in
        let order_by_key = find_idx body_args (find_first_agg_var hargs) in 
          (*debug "orderby rewrite add groupby key..";*)
          Symbol.add_secondary_key body_name group_by_key;
          begin
            (*order by rule head should not use ref count*)
            Symbol.disable_view_refcount hn;
          end;
          [ ((EV_INSERTED, (body_name,body_args,false,bloc) ), [], 
             (ACT_ORDERBY(group_by_key,order_by_key,group_by_key_head,otype), 
              (hn,hargs,false,hloc) ));
            ((EV_DELETED, (body_name,body_args,false,bloc) ), [], 
             (ACT_ORDERBY(group_by_key,order_by_key,group_by_key_head,otype),
              (hn,hargs,false,hloc) ))]
      end
    | _ -> 
        mlocerror Compile_error
          "The rule body must be a single non-negate table query without filters, followed by a groupBy term indicating groupBy keys." 
          loc;;

let aggview_rewrite (hn,hargs,hloc) body loc :ecaRule list = 
  match body with
      [MFunctor(body_name, body_args, false, bloc);
       groupby_term] -> begin
        current_loc := bloc;
        Symbol.disable_view_refcount hn;
        let group_by_key, group_by_key_head = 
          check_group_by (body_name,body_args) groupby_term (hn,hargs,hloc) in
          [ ((EV_INSERTED, (body_name,body_args,false,bloc) ), [], 
             (ACT_AGGVIEW(group_by_key, group_by_key_head), 
              (hn,hargs,false,hloc) ));
            ((EV_DELETED, (body_name,body_args,false,bloc) ), [], 
             (ACT_AGGVIEW(group_by_key, group_by_key_head),
              (hn,hargs,false,hloc) ))]
      end
    | _ -> 
        mlocerror Compile_error
          "The rule body must be a single non-negate table query without filters, followed by a groupBy term indicating groupBy keys." 
          loc;;



let rec clauses2ecas clauses : ecaRule list =
  List.concat (List.map clause2eca clauses)
and clause2eca clause: ecaRule list = match clause with
    MEcaRule(r,loc) -> [ r ]
  | MViewRule(rule_name, rule_head, rule_body, loc) -> begin
      (*FIXME: if there is more than one probe terms in rule body, 
        and this is an aggregation rule, create an auxillary rule first*)
      (*delta rewrite*)
      match rule_head with
          (_ , _ , true, hloc) ->
            mlocerror Compile_error "View rule head cannot be a negated table." hloc
        | ( hn, hargs, false, hloc) -> begin
            is_view_assert hn hloc;
            let agglist = expr_list_has_agg hargs in
              match agglist with
                  [] -> (*not aggregation rule*)
                    delta_rewrite [] rule_body (hn,hargs,false,hloc)
                | [(M_AGG_MIN,_)]  -> 
                    orderby_rewrite (hn,hargs,hloc) M_ORDER_MIN rule_body loc
                | [(M_AGG_MAX,_)]  -> 
                    orderby_rewrite (hn,hargs,hloc) M_ORDER_MAX rule_body loc
                | _ -> (*count, or more than 1 aggregations *)
                    aggview_rewrite (hn,hargs,hloc)  rule_body loc
          end
    end
  | _ -> [];;
