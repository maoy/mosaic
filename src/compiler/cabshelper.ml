(* $Id: cabshelper.ml 212 2009-04-14 22:17:25Z maoy $ *)


open Cabs

let nextident = ref 0
let getident () =
    nextident := !nextident + 1;
    !nextident

let currentLoc () = 
  let l, f, c = Errormsg.getPosition () in
  { lineno   = l; 
    filename = f; 
    byteno   = c;
    ident    = getident ();}

let cabslu = {lineno = -10; 
	      filename = "cabs loc unknown"; 
	      byteno = -10;
              ident = 0}

(* clexer puts comments here *)
let commentsGA = GrowArray.make 100 (GrowArray.Elem(cabslu,"",false))


(*********** HELPER FUNCTIONS **********)
let explodeStringToInts (s: string) : int64 list =  
  let rec allChars i acc = 
    if i < 0 then acc
    else allChars (i - 1) (Int64.of_int (Char.code (String.get s i)) :: acc)
  in
  allChars (-1 + String.length s) []

let valueOfDigit chr =
  let int_value = 
    match chr with
      '0'..'9' -> (Char.code chr) - (Char.code '0')
    | 'a'..'z' -> (Char.code chr) - (Char.code 'a') + 10
    | 'A'..'Z' -> (Char.code chr) - (Char.code 'A') + 10
    | _ -> Errormsg.s (Errormsg.bug "not a digit") in
  Int64.of_int int_value
  
    
(*open Pretty
let d_cabsloc () cl = 
  text cl.filename ++ text ":" ++ num cl.lineno
*)
