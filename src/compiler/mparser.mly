/*
(**
 borrowed many things from cil
*)
*/
%{
open Cabs
open Cabshelper
module E = Errormsg

let parse_error msg : unit =       (* sm: c++-mode highlight hack: -> ' <- *)
  E.parse_error msg

let print = print_string

(* unit -> string option *)
(*
let getComments () =
  match !comments with
    [] -> None
  | _ -> 
      let r = Some(String.concat "\n" (List.rev !comments)) in
      comments := [];
      r
*)

let cabslu = {lineno = -10; 
	      filename = "cabs loc unknown"; 
	      byteno = -10;
              ident = 0;}

(* cabsloc -> cabsloc *)
(*
let handleLoc l =
  l.clcomment <- getComments();
  l
*)

(*
** Expression building
*)


let int64_to_char value =
  if (compare value (Int64.of_int 255) > 0) || (compare value Int64.zero < 0) then
    begin
      let msg = Printf.sprintf "cparser:intlist_to_string: character 0x%Lx too big" value in
      parse_error msg;
      raise Parsing.Parse_error
    end
  else
    Char.chr (Int64.to_int value)

(* takes a not-nul-terminated list, and converts it to a string. *)
let rec intlist_to_string (str: int64 list):string =
  match str with
    [] -> ""  (* add nul-termination *)
  | value::rest ->
      let this_char = int64_to_char value in
      (String.make 1 this_char) ^ (intlist_to_string rest)

let fst3 (result, _, _) = result
let snd3 (_, result, _) = result
let trd3 (_, _, result) = result

let functorloc  = function
   (_,_,_,loc) -> loc;;


let term2functor t =
  match t with
    MFunctor( name, args, negate, loc ) -> (name, args, negate, loc)
  | _ -> 
      parse_error "term is not not a functor";
      raise Parsing.Parse_error;;

let to_mkey tuple (keys:mkeyfield list) : mkey =
  let field_names = Hashtbl.create 10 in
  let tuple_array = Array.of_list tuple in
  let size = Array.length tuple_array in
  let m idx v = match v with
    | ("",t) -> ()
    | (field_name, t) -> try
        ignore(Hashtbl.find field_names field_name);
        parse_error (field_name ^ " is already defined.")
      with Not_found ->
        Hashtbl.add field_names field_name idx
  in
    Array.iteri m tuple_array;
    let f = function 
      | MIndexedByNumber(i,loc) -> 
          if i < size then i
          else begin
            parse_error (Printf.sprintf "key index %d >= tuple size %d" i size);
            -1
          end
      | MIndexedByName(n,loc)-> try
          Hashtbl.find field_names n
        with Not_found ->
          parse_error (n ^ " is not defined.");
          -1
    in
      List.map f keys;;

%}

%token <string * Cabs.cabsloc> IDENT
%token <int64 list * Cabs.cabsloc> CST_CHAR
%token <int64 list * Cabs.cabsloc> CST_WCHAR
%token <string * Cabs.cabsloc> CST_INT
%token <string * Cabs.cabsloc> CST_FLOAT
%token <string * Cabs.cabsloc> NAMED_TYPE

/* Each character is its own list element, and the terminating nul is not
   included in this list. */
%token <int64 list * Cabs.cabsloc> CST_STRING   
%token <int64 list * Cabs.cabsloc> CST_WSTRING

%token IMPLY, AT_SIGN, COLON_COLON

%token EOF
%token<Cabs.cabsloc> CHAR INT DOUBLE FLOAT VOID INT64 INT32 
%token<Cabs.cabsloc> STRING TABLE VIEW EVENT KEYS ON INSERT DELETE AGG_MIN AGG_MAX AGG_COUNT
%token<Cabs.cabsloc> STATIC_CAST DYNAMIC_CAST
%token<Cabs.cabsloc> ENUM STRUCT TYPEDEF UNION
%token<Cabs.cabsloc> SIGNED UNSIGNED LONG SHORT
%token<Cabs.cabsloc> VOLATILE EXTERN STATIC CONST RESTRICT AUTO REGISTER
%token<Cabs.cabsloc> THREAD
%token<Cabs.cabsloc> SIZEOF ALIGNOF

%token EQ PLUS_EQ MINUS_EQ STAR_EQ SLASH_EQ PERCENT_EQ
%token AND_EQ PIPE_EQ CIRC_EQ INF_INF_EQ SUP_SUP_EQ
%token ARROW DOT
%token LEADSTO /*new*/
%token<Cabs.cabsloc> MODULE

%token EQ_EQ EXCLAM_EQ INF SUP INF_EQ SUP_EQ
%token<Cabs.cabsloc> PLUS MINUS STAR
%token SLASH PERCENT
%token<Cabs.cabsloc> TILDE AND
%token PIPE CIRC
%token<Cabs.cabsloc> EXCLAM AND_AND
%token PIPE_PIPE
%token INF_INF SUP_SUP
%token<Cabs.cabsloc> PLUS_PLUS MINUS_MINUS

%token RPAREN 
%token<Cabs.cabsloc> LPAREN RBRACE
%token<Cabs.cabsloc> LBRACE
%token LBRACKET RBRACKET
%token COLON
%token<Cabs.cabsloc> SEMICOLON
%token COMMA ELLIPSIS QUEST

%token<Cabs.cabsloc> BREAK CONTINUE GOTO RETURN
%token<Cabs.cabsloc> SWITCH CASE DEFAULT
%token<Cabs.cabsloc> WHILE DO FOR
%token<Cabs.cabsloc> IF TRY EXCEPT FINALLY
%token ELSE 

%token<Cabs.cabsloc> ATTRIBUTE INLINE ASM TYPEOF FUNCTION__ PRETTY_FUNCTION__
%token LABEL__
%token<Cabs.cabsloc> BUILTIN_VA_ARG ATTRIBUTE_USED
%token BUILTIN_VA_LIST
%token BLOCKATTRIBUTE 
%token<Cabs.cabsloc> BUILTIN_TYPES_COMPAT BUILTIN_OFFSETOF
%token<Cabs.cabsloc> DECLSPEC
%token<string * Cabs.cabsloc> MSASM MSATTR
%token<string * Cabs.cabsloc> PRAGMA_LINE
%token<Cabs.cabsloc> PRAGMA
%token PRAGMA_EOL

/* sm: cabs tree transformation specification keywords */
%token<Cabs.cabsloc> AT_TRANSFORM AT_TRANSFORMEXPR AT_SPECIFIER AT_EXPR
%token AT_NAME

/* operator precedence */
%nonassoc 	IF
%nonassoc 	ELSE


%left	COMMA
%right	EQ PLUS_EQ MINUS_EQ STAR_EQ SLASH_EQ PERCENT_EQ
                AND_EQ PIPE_EQ CIRC_EQ INF_INF_EQ SUP_SUP_EQ
%right	QUEST COLON
%left	PIPE_PIPE
%left	AND_AND
%left	PIPE
%left 	CIRC
%left	AND
%left	EQ_EQ EXCLAM_EQ
%left	INF SUP INF_EQ SUP_EQ
%left	INF_INF SUP_SUP
%left	PLUS MINUS
%left	STAR SLASH PERCENT CONST RESTRICT VOLATILE
%right	EXCLAM TILDE PLUS_PLUS MINUS_MINUS CAST RPAREN ADDROF SIZEOF ALIGNOF
%left 	LBRACKET
%left	DOT ARROW LPAREN LBRACE
%right  NAMED_TYPE     /* We'll use this to handle redefinitions of
                        * NAMED_TYPE as variables */
%left   IDENT

/* Non-terminals informations */
/* begin of mozlog stuff */

%start interpret file
%type <Cabs.mclause list> file interpret
%type <Cabs.mclause> mclause
%type <Cabs.mclause> mrule
%type <Cabs.mclause> mdefinition
%type <Cabs.mfunctor> mfunctor
/*%type <Cabs.mexpression> mexpr*/
%type <Cabs.mexpression * cabsloc> expression
%type <Cabs.mexpression list * cabsloc> comma_expression

/* end of mozlog stuff */
/*%type <Cabs.constant * cabsloc> constant*/
%type <string * cabsloc> string_constant
%type <int64 list Queue.t * cabsloc> string_list 
%type <int64 list * cabsloc> wstring_list

%%
/* mosaic stuff*/
interpret:
  file EOF				{$1}
;
file: mclauses				{$1}
;
mclauses:
  /* empty */ { [] }
| mclause mclauses  { $1 :: $2 }
| mmodule mclauses  { $1 @ $2 }
;

mclause:
  mrule { $1 }
| mecarule { $1 }
| mdefinition { $1 }
| mconst { $1 }
| mfact {$1 }
;

mmodule:
  MODULE IDENT LBRACE mclauses RBRACE { $4 }
mfact:
  mfunctor SEMICOLON { MFact $1 } 
;

mrule:
 mfunctor IMPLY mtermlist SEMICOLON { MViewRule( "", $1, $3, functorloc $1 ) }
;

mecarule:
   ON mevent_type mtermlist LEADSTO maction_type mfunctor SEMICOLON 
  { MEcaRule( ( ($2, term2functor (List.hd $3)),
	        List.tl $3,
	        ( $5, $6) )
                , $1 )
  }
;

mevent_type:
  /*empty*/ { EV_RECV }
| INSERT    { EV_INSERTED }
| DELETE    { EV_DELETED }
;

maction_type:
  /*empty*/ { ACT_SEND }
| INSERT    { ACT_INSERT }
| DELETE    { ACT_DELETE }
;

mfunctor:
  IDENT LPAREN mfunctorargs RPAREN { ( fst $1, $3, false, snd $1) }
| EXCLAM IDENT LPAREN mfunctorargs RPAREN 
      { ( fst $2, $4, true, snd $2) }
;

mfunctorargs:
  mfunctorarg             { [$1] }
| mfunctorarg COMMA mfunctorargs { $1 :: $3 }
;

mfunctorarg:
  expression                   { fst $1 }
;

mtermlist:
  mterm                    { [$1] }
| mterm COMMA mtermlist          { $1 :: $3 }
;

mterm:
| massign                  { $1 }
| expression              
   { try
       MFunctor(expr2functor $1)
     with Not_functor ->
       MExpr (fst $1)
   }
;

massign:
  IDENT EQ expression        { MAssign( fst $1, fst $3, snd $1 ) }
;
mconstant:
    CST_INT				{M_CONST_INT (fst $1), snd $1}
|   CST_FLOAT				{M_CONST_FLOAT (fst $1), snd $1}
|   CST_CHAR				{M_CONST_CHAR (fst $1), snd $1}
|   CST_WCHAR				{M_CONST_WCHAR (fst $1), snd $1}
|   string_constant		        {M_CONST_STRING (fst $1), snd $1}
|   wstring_list			{M_CONST_WSTRING (fst $1), snd $1}
;
mdefinition:
  mtype IDENT SEMICOLON {MDef( $1, fst $2, snd $2 ) }
;

mtype:
  INT  { MTint }
| STRING { MTstring }
| DOUBLE { MTdouble }
| LONG   { MTlong }
| IDENT                                              { MUserType(fst $1) }
| IDENT INF mcomma_typelist SUP
      { MTemplate (fst $1, $3) }
| TABLE INF INF mtuple SUP COMMA mkeytype SUP   
      { MTableType( $4, to_mkey $4 $7, ref [], MRefTable(ref false)) }
| TABLE INF INF mtuple SUP COMMA mkeytype COMMA CST_INT SUP   
      { MTableType( $4, to_mkey $4 $7, ref [], MSoftTable(int_of_string(fst $9),0)) }
| TABLE INF INF mtuple SUP COMMA mkeytype COMMA CST_INT COMMA CST_INT SUP
      { MTableType( $4, to_mkey $4 $7, ref [], MSoftTable(int_of_string(fst $9),int_of_string(fst $11))) }
| VIEW INF mtuple SUP 
      { MViewType( $3, ref [], ref [], MRefTable(ref true) ) 
              (* tuple_type, no_primary_key, no_secondary_keys, use ref counting*)
      }
| EVENT INF mtuple SUP    { MEventType( $3) }
;

mcomma_typelist:
   mtype       { [$1] }
|  mtype COMMA mcomma_typelist { $1::$3 }
;

mtuple_field:
  mtype { ("",$1) }
| IDENT COLON mtype { (fst $1, $3) }
;

mtuple:
   mtuple_field       { [ $1 ] }
|  mtuple_field COMMA mtuple { $1::$3 }
;

mkeytype:
   KEYS INF mkeylist SUP  { $3 }
;

mkeyfield:
  CST_INT { MIndexedByNumber(int_of_string(fst $1), snd $1) }
| IDENT   { MIndexedByName(fst $1, snd $1) }
;

mkeylist:
   /*empty*/       { [] }
|  mkeyfield { [ $1 ] }
|  mkeyfield COMMA mkeylist  {  $1 :: $3 }
;

mtypelist:
  /*empty*/         {  []  }
|  mtype mtypelist  { $1 :: $2 }
;

mconst:
  CONST mtype IDENT EQ expression SEMICOLON { MConst( $2, fst $3,fst $5, snd $3 ) }
;


/* *** Expressions *** */

primary_expression:                     /*(* 6.5.1. *)*/
| IDENT
   {MVariable (fst $1), snd $1}
| COLON_COLON IDENT
   {MVariable ("::"^fst $2), snd $2}
| mconstant
   {MConstant (fst $1), snd $1}
| IDENT COLON_COLON IDENT
   {MVariable ( (fst $1)^"::"^( fst $3) ), snd $1}
| LPAREN expression RPAREN
   { MParen( fst $2), snd $2 }
|  LBRACKET extended_expression RBRACKET   
     {MNewVector( fst $2 ), snd $2 }
| agg_expression
     {$1}
;
agg_expression:
| AGG_MIN INF IDENT SUP  
    { MAggregation( M_AGG_MIN, MVariable(fst $3) ), snd $3 }
| AGG_MAX INF IDENT SUP 
    { MAggregation( M_AGG_MAX, MVariable(fst $3) ), snd $3 }
| AGG_COUNT INF IDENT SUP  
    { MAggregation( M_AGG_COUNT, MVariable(fst $3) ), snd $3 }
| AGG_COUNT INF STAR SUP  
    { MAggregation( M_AGG_COUNT, MVariable("*") ),  $1 }
;

postfix_expression:                     /*(* 6.5.2 *)*/
|               primary_expression     
                        { $1 }
|		postfix_expression LPAREN arguments RPAREN
                        {MCall (fst $1, $3), snd $1}
|		postfix_expression DOT IDENT
		        {MMemberOf (fst $1, fst $3), snd $1}
|		postfix_expression ARROW IDENT   
		        {MMemberOfPtr (fst $1, fst $3), snd $1}
| postfix_expression LBRACKET extended_expression RBRACKET /* note: C allows comma expression. not here*/
                        { MIndex (fst $1, fst $3), snd $1 }
  
;

unary_expression:   /*(* 6.5.3 *)*/
|               postfix_expression
                        { $1 }
/*|		SIZEOF unary_expression
		        {EXPR_SIZEOF (fst $2), $1}
|	 	SIZEOF LPAREN type_name RPAREN
		        {let b, d = $3 in TYPE_SIZEOF (b, d), $1}*/
|		PLUS cast_expression
		        {MUnary (M_PLUS, fst $2), $1}
|		MINUS cast_expression
		        {MUnary (M_MINUS, fst $2), $1}
|		STAR cast_expression
		        {MUnary (M_MEMOF, fst $2), $1}
|		AND cast_expression				
		        {MUnary (M_ADDROF, fst $2), $1}
|		EXCLAM cast_expression
		        {MUnary (M_NOT, fst $2), $1}
|		TILDE cast_expression
		        {MUnary (M_BNOT, fst $2), $1}
;

cast_expression:   /*(* 6.5.4 *)*/
| unary_expression 
        { $1 }
| STATIC_CAST INF mtype SUP LPAREN cast_expression RPAREN
        { MStaticCast($3, (fst $6)), snd $6 }
| DYNAMIC_CAST INF mtype SUP LPAREN cast_expression RPAREN
        { MDynamicCast($3, (fst $6)), snd $6 }
;

multiplicative_expression:  /*(* 6.5.5 *)*/
|               cast_expression
                         { $1 }
|		multiplicative_expression STAR cast_expression
			{MBinary(M_MUL, fst $1, fst $3), snd $1}
|		multiplicative_expression SLASH cast_expression
			{MBinary(M_DIV, fst $1, fst $3), snd $1}
|		multiplicative_expression PERCENT cast_expression
			{MBinary(M_MOD, fst $1, fst $3), snd $1}
;

additive_expression:  /*(* 6.5.6 *)*/
|               multiplicative_expression
                        { $1 }
|		additive_expression PLUS multiplicative_expression
			{MBinary(M_ADD, fst $1, fst $3), snd $1}
|		additive_expression MINUS multiplicative_expression
			{MBinary(M_SUB, fst $1, fst $3), snd $1}
;

shift_expression:      /*(* 6.5.7 *)*/
|               additive_expression
                         { $1 }
|		shift_expression  INF_INF additive_expression
			{MBinary(M_SHL, fst $1, fst $3), snd $1}
|		shift_expression  SUP_SUP additive_expression
			{MBinary(M_SHR, fst $1, fst $3), snd $1}
;


relational_expression:   /*(* 6.5.8 *)*/
|               shift_expression
                        { $1 }
|		relational_expression INF shift_expression
			{MBinary(M_LT, fst $1, fst $3), snd $1}
|		relational_expression SUP shift_expression
			{MBinary(M_GT, fst $1, fst $3), snd $1}
|		relational_expression INF_EQ shift_expression
			{MBinary(M_LE, fst $1, fst $3), snd $1}
|		relational_expression SUP_EQ shift_expression
			{MBinary(M_GE, fst $1, fst $3), snd $1}
;

equality_expression:   /*(* 6.5.9 *)*/
|              relational_expression
                        { $1 }
|		equality_expression EQ_EQ relational_expression
			{MBinary(M_EQ, fst $1, fst $3), snd $1}
|		equality_expression EXCLAM_EQ relational_expression
			{MBinary(M_NE, fst $1, fst $3), snd $1}
;


bitwise_and_expression:   /*(* 6.5.10 *)*/
|               equality_expression
                       { $1 }
|		bitwise_and_expression AND equality_expression
			{MBinary(M_BAND, fst $1, fst $3), snd $1}
;

bitwise_xor_expression:   /*(* 6.5.11 *)*/
|               bitwise_and_expression
                       { $1 }
|		bitwise_xor_expression CIRC bitwise_and_expression
			{MBinary(M_XOR, fst $1, fst $3), snd $1}
;

bitwise_or_expression:   /*(* 6.5.12 *)*/
|               bitwise_xor_expression
                        { $1 } 
|		bitwise_or_expression PIPE bitwise_xor_expression
			{MBinary(M_BOR, fst $1, fst $3), snd $1}
;

logical_and_expression:   /*(* 6.5.13 *)*/
|               bitwise_or_expression
                        { $1 }
|		logical_and_expression AND_AND bitwise_or_expression
			{MBinary(M_AND, fst $1, fst $3), snd $1}
;

logical_or_expression:   /*(* 6.5.14 *)*/
|               logical_and_expression
                        { $1 }
|		logical_or_expression PIPE_PIPE logical_and_expression
			{MBinary(M_OR, fst $1, fst $3), snd $1}
;

conditional_expression:    /*(* 6.5.15 *)*/
|               logical_or_expression
                         { $1 }
| logical_or_expression QUEST expression COLON conditional_expression
    {MQuestion (fst $1, fst $3, fst $5), snd $1}
;

extended_expression:
|  conditional_expression
     { $1 }
;

at_expression:
|    extended_expression
       { $1 }
|    AT_SIGN extended_expression
       {MLocspec(fst $2), snd $2}
;

expression:           /*(* 6.5.17 *)*/
                at_expression
                        { $1 }
;

arguments: 
                /* empty */         { [] }
|               comma_expression    { fst $1 }
;

comma_expression:
	        expression                        {[fst $1], snd $1}
|               expression COMMA comma_expression { fst $1 :: fst $3, snd $1 }
|               error COMMA comma_expression      { $3 }
;


/*** C stuff ******/
                            
string_constant:
/* Now that we know this constant isn't part of a wstring, convert it
   back to a string for easy viewing. */
    string_list                         {
     let queue, location = $1 in
     let buffer = Buffer.create (Queue.length queue) in
     Queue.iter
       (List.iter
	  (fun value ->
	    let char = int64_to_char value in
	    Buffer.add_char buffer char))
       queue;
     Buffer.contents buffer, location
   }
;
one_string_constant:
/* Don't concat multiple strings.  For asm templates. */
    CST_STRING                          {intlist_to_string (fst $1) }
;
string_list:
    one_string                          {
      let queue = Queue.create () in
      Queue.add (fst $1) queue;
      queue, snd $1
    }
|   string_list one_string              {
      Queue.add (fst $2) (fst $1);
      $1
    }
;

wstring_list:
    CST_WSTRING                         { $1 }
|   wstring_list one_string             { (fst $1) @ (fst $2), snd $1 }
|   wstring_list CST_WSTRING            { (fst $1) @ (fst $2), snd $1 }
/* Only the first string in the list needs an L, so L"a" "b" is the same
 * as L"ab" or L"a" L"b". */

one_string: 
    CST_STRING				{$1}
;    
  
%%



