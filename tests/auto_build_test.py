#!/usr/bin/python
import os, sys

mosaic_dir = '/home/mosaictest/mosaic' #will be reset in build()

def pr(s):
    print s
    sys.stdout.flush()

def exec_cmd(cmd, retv = 0):
    ret = os.system(cmd)
    if ret!=0: 
        if retv==0: sys.exit(ret)
        sys.exit(retv)

def build_lib():
    pr("=================build lib ==============")
    home_dir = os.environ["HOME"]
    pr("<svn>")
    cmd = "svn co https://mosaic.maoy.net/svn/trunk/mosaic"
    exec_cmd(cmd)
    pr("</svn>")
    mosaic_dir = os.path.join(home_dir, "mosaic")
    os.chdir(mosaic_dir)
    pr("<build_lib>")
    cmd = "./setup"
    exec_cmd(cmd)
    cmd = "make"
    exec_cmd(cmd)
    cmd = "make -C build && make -C release"
    exec_cmd(cmd)
    pr("</build_lib>")

def build_mos():
    pr("============ build mos ================")
    pr("<build_mos>")
    os.chdir(os.path.join(mosaic_dir, "src/mos"))
    exec_cmd("make")
    os.chdir(os.path.join(mosaic_dir, "src/tests"))
    exec_cmd("make")
    os.chdir(os.path.join(mosaic_dir, "src/benchmarks"))
    exec_cmd("make")
    pr("</build_mos>")

def clean_build():
    home_dir = os.environ["HOME"]
    if not ("mosaictest" in home_dir):
        # this is to prevent from acidentally deleting $HOME/mosaic
        print "not using the right account mosaictest for testing.. exit"
        sys.exit(1)
    os.chdir(home_dir)
    pr("removing old mosaic..")
    cmd = "rm -rf /home/mosaictest/mosaic/"
    exec_cmd(cmd)
    build_lib()
    build_mos()
    
def tests():
    pr("===============unit tests and tests ==========")
    pr('<unitTests target="build">')
    os.chdir(os.path.join(mosaic_dir, "build/unitTests"))
    exec_cmd("./unitTest")
    pr('</unitTests>')
    pr('<unitTests target="release">')
    pr("start unit tests in release")
    os.chdir(os.path.join(mosaic_dir, "release/unitTests"))
    exec_cmd("./unitTest")
    pr('</unitTests>')
    os.chdir(os.path.join(mosaic_dir, "src/tests"))
    pr('<tests>')
    exec_cmd("./tests.sh")
    pr('</tests>')

def bench():
    pr("================= benchmarking ===========")
    exec_cmd("uname -a")
    exec_cmd("cat /proc/cpuinfo")
    os.chdir(os.path.join(mosaic_dir, "release/demo"))
    pr("<db_bench>")
    exec_cmd("./db_bench")
    pr("</db_bench>")
    os.chdir(os.path.join(mosaic_dir, "src/benchmarks/dv"))
    pr("<dv_bench>")
    exec_cmd("./run.sh")
    pr("</dv_bench>")
    pr("<ping>")
    os.chdir(os.path.join(mosaic_dir, "src/benchmarks/ping"))
    exec_cmd("./run.sh")
    pr("</ping>")

clean_build()
tests()
bench()
