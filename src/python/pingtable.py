from table import MemTable
from db import *
from table import MDBException
import threading
from netconf import *
import time

class PingTable(MemTable):
	connections = {}

	def __init__(self, routerTable):
		self.routerTable = routerTable
		super(PingTable, self).__init__(['RouterID', 'Dest', 'SrcIP'])

	def where(self, **kw):
		# all fields except for result should be bounded
		if ('RouterID' in kw.keys() and
				'Dest' in kw.keys() and
				'SrcIP' in kw.keys() and
				not 'Result' in kw.keys()):
			if (not PingTable.connections.has_key(kw['RouterID'])):
				for tp in self.routerTable.where( RouterID=kw['RouterID'] ):
					manIP = tp['ManagementIP']
					manPort = tp['ManagementPort']
					manUser = tp['ManagementUser']
					manPass = tp['ManagementPass']
					break
				PingTable.connections[kw['RouterID']] = NetConf(
					manIP, manPort, manUser, manPass)
				PingTable.connections[kw['RouterID']].start()

			res = PingTable.connections[kw['RouterID']].ping(kw)

			t = Tuple('router', 
				RouterID = kw['RouterID'],
				Dest = kw['Dest'],
				SrcIP = kw['SrcIP'],
				SuccessRate = res[1] * 1.0 / res[0],
				RTT = res[2])
			yield t
				
		else:
			pass
				
