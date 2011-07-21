(* $Id: symbol.ml 212 2009-04-14 22:17:25Z maoy $ *)

open Util
open Cabs
open Printf

let symbols = Hashtbl.create 200
let init_symbols () =
  Hashtbl.clear symbols;;

let reserved_events = ["print";"dump";"exit";"periodic"];;
let reserved_tables = ["print";"groupBy";"orderBy"];;

let is_defined (name:string) :bool = 
  try
    if not ((List.mem name reserved_events) or (List.mem name reserved_tables))
    then 
      ignore ( Hashtbl.find symbols name);
    true
  with Not_found ->
    false;;

let get_primary_key_from_def (name:string):mkey =
  match Hashtbl.find symbols name with
      MDef(MTableType(_, k, _ , _), _, _) -> k
    | MDef(MViewType(_, k, _ , _), _, _) -> !k
    | _ -> merror Internal_error (name ^ " is not a table when getting pkey?")

let get_constant_type_from_def (name:string):mtype =
  match Hashtbl.find symbols name with
      MConst(t, name, _, _) -> t
    | _ -> 
        error_loc Compile_error (name ^ " is not a constant")

let set_view_primary_key name key =
  match Hashtbl.find symbols name with
    | MDef(MViewType(_, k, _ , _), _, _) -> 
        debug (sprintf "updating view `%s` primary key to [%s]" name (string_of_key key) );
        k:=key
    | _ -> merror Internal_error (name ^ " is not a view when setting pkey?")

let get_table_tuple_type name : mtype list =
  match Hashtbl.find symbols name with
      MDef(MTableType(t, _, _ , _), _, _) -> snd (List.split t)
    | MDef(MViewType(t, _, _ , _), _, _) -> snd (List.split t)
    | _ -> merror Internal_error (name ^ " is not a table when getting table tuple type?")

let get_tuple_type name : mtype list =
  try
    match Hashtbl.find symbols name with
        MDef(MTableType(t, _, _ , _), _, _) 
      | MDef(MViewType(t, _, _ , _), _, _) 
      | MDef(MEventType(t), _, _) -> snd (List.split t)
      | _ -> merror Internal_error (name ^ " is not a table/view/event when get tuple type?")
  with Not_found ->
      error_loc Compile_error (sprintf "%s is not defined (in get_tuple_type)" name)

let get_tuple_field_names name : string list =
  try
    match Hashtbl.find symbols name with
        MDef(MTableType(t, _, _ , _), _, _) 
      | MDef(MViewType(t, _, _ , _), _, _) 
      | MDef(MEventType(t), _, _) -> fst (List.split t)
      | _ -> merror Internal_error (name ^ " is not a table when get tuple type?")
  with Not_found ->
      error_loc Compile_error (sprintf "%s is not defined in get_tuple_field_names" name)

let get_table_field_type name pos =
  (List.nth (get_table_tuple_type name) pos);;

let get_field_type name pos =
  (List.nth (get_tuple_type name) pos);;

let get_field_name name pos =
  (List.nth (get_tuple_field_names name) pos);;

let add_event name etype:unit =
  if not (is_defined name) then
    let f n = ("",n) in
    let r = List.map f etype in
    Hashtbl.add symbols name
      (MDef(MEventType(r), name, default_loc))
  else
    merror Compile_error 
      (sprintf "event `%s' is already defined." name)

let is_table_primary_key name key = 
  compare key (get_primary_key_from_def name) == 0;;

let add_secondary_key name key =
  if key==[] then
    merror Internal_error "cannot add an empty key";
  match Hashtbl.find symbols name with
      MDef(MTableType(_, _, sks,_ ), _, _) 
    | MDef(MViewType(_, _, sks,_ ), _, _) 
      -> 
        if  not (List.mem key (!sks)) then begin
          sks := key::(!sks);
          debug (sprintf "Added secondary key [%s] for table %s, which has %d sec keys now." 
            (string_of_key key) name (List.length !sks))
        end
    | _ -> merror Internal_error (name ^ " is not a table when adding secondary key?")

let output_location fd loc = 
  match loc with
      MDef(_,_,l) ->
	output_string fd (l.filename ^ ":" ^ (string_of_int l.lineno))
    | _ -> merror Internal_error "in output_location";;

let save_def clause =
  match clause with
      MDef( _, name, loc) | MConst(_, name, _, loc) ->
	begin
	try 
	  let previousdef = Hashtbl.find symbols name in
            ignore (previousdef);
            mlocerror Compile_error 
              (sprintf "variable %s redefined" name) loc;
            (*output_location stderr previousdef;
	      output_string stderr "\n";
	      flush stderr;
	      raise Compile_error*)
	with Not_found ->
	  Hashtbl.add symbols name clause
	end
    | _ -> ()

let save_symbols program = 
  List.iter save_def (snd program)

let is_table_assert name loc =
  try
    if not (List.mem name reserved_tables) then
      match Hashtbl.find symbols name with
          MDef( MTableType( _, _, _, _), _, _) -> ()
        | MDef( _,_,defloc) | MConst(_,_,_,defloc)
            -> mlocerror Compile_error
            (sprintf "%s is used as a table, but is defined differently at file '%s', line: %d"
               name defloc.filename defloc.lineno
            ) loc
        | _ -> merror Internal_error "symbol table has strange stuff"
  with Not_found ->
    mlocerror Compile_error (sprintf "%s is supposed to be a table, but not defined." name) loc;;

let is_view_assert name loc =
  try
    if not (List.mem name reserved_tables) then
      match Hashtbl.find symbols name with
          MDef( MViewType( _, _, _, _), _, _) -> ()
        | MDef( _,_,defloc) | MConst(_,_,_,defloc)
            -> mlocerror Compile_error
            (sprintf "%s is used as a view, but is defined differently at file '%s', line: %d"
               name defloc.filename defloc.lineno
            ) loc
        | _ -> merror Internal_error "symbol table has strange stuff"
  with Not_found ->
    mlocerror Compile_error (sprintf "%s is supposed to be a view, but not defined." name) loc;;
  
let is_tableorview_assert name loc =
  try
    if not (List.mem name reserved_tables) then
      match Hashtbl.find symbols name with
          MDef( MViewType( _, _, _, _), _, _) -> ()
        | MDef( MTableType( _, _, _, _), _, _) -> ()
        | MDef( _,_,defloc) | MConst(_,_,_,defloc)
            -> mlocerror Compile_error
            (sprintf "%s is used as a view/table, but is defined differently at file '%s', line: %d"
               name defloc.filename defloc.lineno
            ) loc
        | _ -> merror Internal_error "symbol table has strange stuff"
  with Not_found ->
    mlocerror Compile_error (sprintf "%s is supposed to be a view/table, but not defined." name) loc;;
  
let is_event_assert name loc =
  try
    if not (List.mem name reserved_events) then
      match Hashtbl.find symbols name with
          MDef(MEventType(_), _, _) -> ()
        | _ -> mlocerror Compile_error 
            (sprintf "%s is not an event." name) loc
  with Not_found ->
    mlocerror Compile_error 
      (sprintf "`%s` is not defined as event." name) loc;;

let disable_table_refcount hn = 
  match Hashtbl.find symbols hn with
      MDef( MTableType(_,_,_,MRefTable(refCount)),_,_) ->
        refCount:=false;
    | _ -> merror Internal_error "In updating table refcount"

let disable_view_refcount hn = 
  match Hashtbl.find symbols hn with
      MDef( MViewType(_,_,_,MRefTable(refCount)),_,_) ->
        refCount:=false;
    | _ -> merror Internal_error "In updating view refcount"

(* periodic stuff *)

let periodics: (int,periodic_type) Hashtbl.t = Hashtbl.create 20
let periodic_count = ref 0

