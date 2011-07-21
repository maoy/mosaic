#!/usr/bin/python
modules = "pretty errormsg lexerhack longarray growArray cabs cabshelper util symbol intern eca alloy printcpp whitetrack mparser mlexer frontm compile"
s = ' '.join([m+'.cmo' for m in modules.split()])
cmd = "ocaml %s" % s
print cmd
