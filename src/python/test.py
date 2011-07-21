import netconf
from pingtable import PingTable
from table import MemTable
from db import *
import time

"""
n = netconf.NetConf("10.128.1.1", 22, "root", 'notplan9!')

if (n.connected):
	n.start()
	print "thread started!"
	print n.ping( {'dst':'10.1.1.1', 'src'='10.128.1.1'} )
"""

"""
routerTable = MemTable(('RouterID'))

t1 = Tuple("entry",
	RouterID = '10.128.1.1',
	ManagementIP = '10.128.1.1',
	ManagementPort = 22,
	ManagementUser = 'root',
	ManagementPass = 'notplan9!')
routerTable.insert(t1)

pt = PingTable(routerTable)

for i in range(10):
	iter = pt.where(
		RouterID='10.128.1.1', 
		Dest='10.1.1.1',
		SrcIP='10.128.1.1')

	for res in iter:
		print res

	time.sleep(5)
"""

n = netconf.NetConf("10.128.1.1", 22, "root", 'notplan9!')
if (n.connected):
    n.start()
    print n.bgpStatus({"neighbor-address": '192.168.2.254'}) 

