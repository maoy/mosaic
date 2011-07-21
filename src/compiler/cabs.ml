(* $Id: cabs.ml 212 2009-04-14 22:17:25Z maoy $ *)

(*
** Types
*)

(*
  runtime system constants
*)
let max_key_len = 5;;
let max_tuple_len = 9;;
let max_secondary_keys = 4;;
(*
** Types
*)

type cabsloc = {
 lineno : int;
 filename: string;
 byteno: int;
 ident : int;
}

let default_loc = {lineno=(-1);filename="";byteno=0;ident=0};;
let current_loc = ref default_loc;;
  
(*******************Mosaic stuff ***************)
type periodic_type = {interval:string; repeat:string; delay:string}

type ecaRule =
  ecaEvent * mterm list * ecaAction
and ecaEvent = ecaEventType *mfunctor
and ecaAction = ecaActionType * mfunctor
and ecaEventType = EV_INSERTED | EV_RECV | EV_DELETED 
                   | EV_PERIODIC of periodic_type
and ecaActionType = ACT_INSERT | ACT_SEND | ACT_DELETE 
                    | ACT_LOCAL_SEND 
                    | ACT_LOCAL_INSERT
                    | ACT_LOCAL_DELETE
                    | ACT_REMOTE_SEND of mexpression
                    | ACT_ORDERBY of mkey * int * mkey * morderbytype
                        (*groupBy key and orderby position in rule body, 
                          and groupBy key found in rule head*)
                    | ACT_AGGVIEW of mkey * mkey (*groupby key in body, head*)

and mclause =
    MViewRule of string * mfunctor * mterm list * cabsloc
  | MEcaRule of ecaRule * cabsloc
  | MDef of mtype * string * cabsloc
  | MConst of mtype * string * mexpression * cabsloc
  | MFact of mfunctor
and mtype =
  | MTint
  | MTstring
  | MTlong
  | MTdouble
  | MUserType of string
  | MTemplate of string * mtype list
  | MUnknownType (* type cannot be inferred from tuple def. used in code generation only*)
  | MTableType of mtupletype * mkey * mkey list ref * mtablespecific
      (* tuple type, primary key, secondary keys, whether use ref counting*)
  | MViewType of mtupletype * mkey ref * mkey list ref * mtablespecific
      (* tuple type, primary key, secondary keys, whether use ref counting*)
  | MEventType of mtupletype
and mtupletype = ( string * mtype ) list
and mkeyfield = 
    MIndexedByNumber of int * cabsloc
  | MIndexedByName of string * cabsloc
and mtablespecific =
    MRefTable of bool ref
  | MSoftTable of int * int (* expiration seconds, maxSize *)
and mkey =
    int list
and mfunctor = string * mexpression list * bool * cabsloc
and mterm = 
    MFunctor of mfunctor
  | MAssign of string * mexpression * cabsloc
  | MExpr of mexpression

and mbinary_operator =
    M_ADD | M_SUB | M_MUL | M_DIV | M_MOD
  | M_AND | M_OR
  | M_BAND | M_BOR | M_XOR | M_SHL | M_SHR 
  | M_EQ | M_NE | M_LT | M_GT | M_LE | M_GE

and munary_operator =
    M_MINUS | M_PLUS | M_NOT | M_MEMOF | M_ADDROF | M_BNOT
  (*
  | PREINCR | PREDECR | POSINCR | POSDECR*)

and mexpression =
    MUnary of munary_operator * mexpression
  | MBinary of mbinary_operator * mexpression * mexpression
  | MQuestion of mexpression * mexpression * mexpression
  | MConstant of mconstant
  | MVariable of string
  | MMemberOf of mexpression * string
  | MMemberOfPtr of mexpression * string
  | MParen of mexpression
  | MCall of mexpression * mexpression list
  | MAggregation of maggtype * mexpression
  | MLocspec of mexpression
  | MNewVector of mexpression
  | MStaticCast of mtype * mexpression
  | MDynamicCast of mtype * mexpression
  | MIndex of mexpression * mexpression
and maggtype = 
    M_AGG_MIN 
  | M_AGG_MAX
  | M_AGG_COUNT
and morderbytype =
    M_ORDER_MIN
  | M_ORDER_MAX
and mconstant =
  | M_CONST_INT of string   (* the textual representation *)
  | M_CONST_FLOAT of string (* the textual representaton *)
  | M_CONST_CHAR of int64 list
  | M_CONST_WCHAR of int64 list
  | M_CONST_STRING of string
  | M_CONST_WSTRING of int64 list 

type var_binding_t = (string, int*mtype) Hashtbl.t

type mstmt =
    stmtEvent * stmtTerm list * stmtAction * var_binding_t
and stmtEvent = ecaEventType * string * mexpression list * bindingTerm list * cabsloc
and stmtTerm = MQuery of string * bindingTerm list * cabsloc
               | MPrimaryJoin of string * joinTerm list * bindingTerm list * cabsloc
               | MSecondaryJoin of string * mkey * joinTerm list * bindingTerm list * cabsloc
               | MNegatePrimaryJoin of string * joinTerm list * bindingTerm list * cabsloc 
               | MNegateSecondaryJoin of string * mkey * joinTerm list * bindingTerm list * cabsloc
               | MFilter of mexpression list
               | MNegFilter of mexpression list
               | MAssignment of string * mexpression * cabsloc
               | MAggTerm of string * mkey * int * string * maggtype
                   (* tablename, groupby key, position, the variable to bind to, and type *)
and stmtAction = ecaActionType * string * mexpression list * cabsloc
and joinTerm = int * mexpression (* key position, value *)
and bindingTerm = string * int * mtype (* after query returns, bind the variable name to the corresponding position*)

exception Internal_error

let action_type_to_string = function
  | ACT_LOCAL_INSERT -> "ins"
  | ACT_LOCAL_DELETE -> "del"
  | ACT_INSERT -> "insert"
  | ACT_DELETE -> "delete"
  | ACT_LOCAL_SEND | ACT_REMOTE_SEND(_) -> "send"
  | ACT_ORDERBY(_) -> "orderby"
  | ACT_AGGVIEW(_) -> "aggview"
  | ACT_SEND -> "unknwon_send"

exception Not_functor

let expr2functor ((e:mexpression), loc) :mfunctor =
  let negate, expr = match e with
      MUnary(M_NOT, uexpr) ->
        (true,uexpr)
    | _ -> (false, e)
  in match expr with
      MCall(MVariable(name),args) ->
        if not (String.contains name ':') then
          (* treat this as a table name*)
          (name, args, negate, loc)
        else
          raise Not_functor
    | _ -> raise Not_functor

