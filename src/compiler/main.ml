(* $Id: main.ml 212 2009-04-14 22:17:25Z maoy $ *)

open Cabs
open Printf

type target_type = CPP | Alloy | Debug;;

let target = ref CPP;;
let filename = ref "";;

let print_target str =
  match str with
      "alloy" -> target := Alloy
    | "cpp" -> target := CPP
    | "debug" -> target := Debug
    | _ -> let msg = sprintf "Unknown target:%s\n" str in
        raise (Arg.Bad msg)


let compile_file fn =
  Util.debug ("mosaicc on " ^ fn);
  filename := fn;;

let parse_args () =
  let spec = [("-t", Arg.String(print_target), "target: cpp(default) | debug | alloy")] in
    Arg.parse_argv Sys.argv spec compile_file "usage: mosaicc [options] file" 

let cpp () =
  let program = Frontm.parse_to_cabs !filename in
    Symbol.save_symbols program;
    Util.debug "start type checking..";
    Types.check_program program;
    Util.debug "type checking done.";
    let buf = Buffer.create 1000 in
    let stmts = Compile.program2stmt program in
      List.iter (Printcpp.print_include buf) (snd program);
      Printcpp.print_defs buf;
      List.iter (Printcpp.print_stmt buf) stmts;
      Printcpp.print_demux buf;
      Printcpp.print_facts buf program;
      Printcpp.print_csv_facts buf;
      Printcpp.print_periodic_inits buf;
      Printcpp.print_tuple_s11n_inits buf;
      Buffer.output_buffer stdout buf;;

let debug () =
  let program = Frontm.parse_to_cabs !filename in
    Symbol.save_symbols program;
    let buf = Buffer.create 1000 in
    let stmts = Compile.program2stmt program in
      Printcpp.print_defs buf;
      List.iter (Printstmt.print_stmt buf) stmts;
      Buffer.output_buffer stdout buf;;

let alloy () =
  let program = Frontm.parse_to_cabs !filename in
    Symbol.save_symbols program;
    let buf = Buffer.create 1000 in
      Util.debug "start type checking";
      Types.check_program program;
      Util.debug "start alloy printing";
      Alloy.print_defs buf;
      Alloy.print_clauses buf program;
      Buffer.output_buffer stdout buf;;
    (*let stmts = Compile.program2stmt program in
      List.iter (Alloy.print_stmt buf) stmts;
    *)
let main () = 
  let _ = parse_args () in 
    match !target with 
        CPP -> cpp ()
      | Alloy -> alloy ()
      | Debug -> debug ()



let _ = Printexc.print main ()
