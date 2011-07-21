(* $Id: frontm.ml 212 2009-04-14 22:17:25Z maoy $ *)

module E = Errormsg

(* Output management *)
let out : out_channel option ref = ref None
let close_me = ref false

let close_output _ =
  match !out with
    None -> ()
  | Some o -> begin
      flush o;
      if !close_me then close_out o else ();
      close_me := false
  end

let set_output filename =
  close_output ();
  let out_chan = try open_out filename 
    with Sys_error msg -> 
    (output_string stderr ("Error while opening output: " ^ msg); exit 1) in
  out := Some out_chan;
  Whitetrack.setOutput out_chan;
  close_me := true

exception ParseError of string
exception CabsOnly


let mlexer lexbuf =
    Mlexer.clear_white ();
    Mlexer.clear_lexeme ();
    let token = Mlexer.initial lexbuf in
    let white = Mlexer.get_white () in
    let cabsloc = Mlexer.currentLoc () in
    let lexeme = Mlexer.get_extra_lexeme () ^ Lexing.lexeme lexbuf in
    white,lexeme,token,cabsloc;;

(* just parse *)
let parse_to_cabs (fname : string) =
  try
    if !E.verboseFlag then ignore (E.log "Frontc is parsing %s\n" fname);
    flush !E.logChannel;
    E.hadErrors := false;
    let lexbuf = Mlexer.init fname in
    let cabs = (Mparser.interpret (Whitetrack.wraplexer mlexer)) lexbuf in
    Whitetrack.setFinalWhite (Mlexer.get_white ());
    Mlexer.finish ();
    (fname, cabs)
  with (Sys_error msg) -> begin
    ignore (E.log "Cannot open %s : %s\n" fname msg);
    Mlexer.finish ();
    close_output ();
    raise (ParseError("Cannot open " ^ fname ^ ": " ^ msg ^ "\n"))
  end
  | Parsing.Parse_error -> begin
      ignore (E.log "Parsing error\n");
      Mlexer.finish ();
      close_output ();
      raise (ParseError("Parse error"))
  end
  | e -> begin
      ignore (E.log "Caught %s while parsing\n" (Printexc.to_string e));
      Mlexer.finish ();
      raise e
  end

