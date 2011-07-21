import paramiko
import time
from threading import Lock
import threading
import pdb
from lxml import etree

class MyEvent:
    def __init__(self):
        self.evt = threading.Event()

    def set(self, msg):
        self.msg = msg
        self.evt.set()

    def wait(self):
        self.evt.wait()
        return self.msg


class NetConf( threading.Thread ):
    
    def __init__(self, host, port, user, pw):
        self.host = host
        self.port = port
        self.user = user
        self.pw = pw

        self.queue = []
        self.queueLock = Lock()

        self.connected = self.connect()
        threading.Thread.__init__(self)


    def _read_msg(self):
        out = ''

        while(1):
            x = self.chan.recv(1024)
            out += x
            if out.find(']]>]]>') != -1:
                break
        return out

    def _send_msg(self, msg):
        while len(msg) > 0: 
            n = self.chan.send(msg)
            if n <= 0:
                raise EOFError()
            if n == len(msg):
                return
            msg = msg[n:]
        return
        

    def connect(self):
        self.transport = paramiko.Transport((self.host, self.port))
        self.transport.connect(username=self.user, password=self.pw)

        self.chan = self.transport.open_session()
        if (self.chan is None):
            return False
        self.chan.invoke_subsystem("netconf")
        
        msg = self._read_msg()
#        print "message received from device:\n", msg
        return True


    def _query(self, name, kw):
        event = MyEvent()
        self.queueLock.acquire()
        self.queue.append( [event, name, kw] )
        self.queueLock.release()
        return event.wait().replace(']]>]]>', '')


    def ping(self, kw):
        res = self._query('ping', kw)
        xres = etree.fromstring(res)

        nodes = \
            xres.xpath('/g:rpc-reply/h:ping-results/h:probe-results-summary',
                namespaces={
                'g':'urn:ietf:params:xml:ns:netconf:base:1.0', 
                'h':'http://xml.juniper.net/junos/9.0R2/junos-probe-tests'})
        
        if (len(nodes) == 0):
            return []
        
        sent = 0
        back = 0
        rtt = 0
        for child in nodes[0].getchildren():
            if (child.tag.endswith('probes-sent')):
                sent = int(child.text.strip())
            if (child.tag.endswith('responses-received')):
                back = int(child.text.strip())
            if (child.tag.endswith('rtt-average')):
                rtt = int(child.text.strip())    

        return [sent, back, rtt]

    def bgpStatus(self, kw):
        res = self._query('bgp', kw)
        xres = etree.fromstring(res)
        
        nodes = xres.xpath(
            '/g:rpc-reply/h:bgp-information/h:bgp-peer',
            namespaces = {
                'g' : 'urn:ietf:params:xml:ns:netconf:base:1.0',
                'h' : 'http://xml.juniper.net/junos/9.0R2/junos-routing',
                } )
        if (len(nodes) == 0):
            return []

        hash = {}
        for child in nodes[0].getchildren():
            if (child.tag.endswith('peer-address')):
                hash['peer-address'] = child.text.strip()
            if (child.tag.endswith('peer-state')):
                hash['peer-state'] = child.text.strip()
            if (child.tag.endswith('peer-type')):
                hash['peer-type'] = child.text.strip()
            if (child.tag.endswith('peer-as')):
                hash['peer-as'] = child.text.strip()

        return hash
    
    def run(self):
        while(1):
            self.queueLock.acquire()
            if (len(self.queue) == 0):
                self.queueLock.release()
                time.sleep(0.05)
            else:
                todo = self.queue.pop(0)
                self.queueLock.release()
                ev = todo.pop(0)
                ev.set(self.handler(todo))

    
    def _ping(self, kw):
#        pdb.set_trace()
        try:
            if (kw.has_key('lr')): 
                self._send_msg("""
                <rpc>
                    <ping>
                        <count>5</count>
                        <rapid/>
                        <no-resolve/>
                        <host>%s</host>
                        <source>%s</source>
                        <logical-system>%s</logical-system>
                    </ping>
                </rpc>
                ]]>]]>""" % (kw['Dest'], kw['SrcIP'], kw['lr']) )

            else:
                self._send_msg("""
                <rpc>
                    <ping>
                        <count>5</count>
                        <rapid/>
                        <no-resolve/>
                        <host>%s</host>
                        <source>%s</source>
                    </ping>
                </rpc>
                ]]>]]>""" % (kw['Dest'], kw['SrcIP']) )

            response = self._read_msg()
    #        print "response from device:\n", response
            return response

        except ex:
            print "error sending a ping"


    def _populate(self, element, fields, kw):
        for field in fields:
            if (kw.has_key(field)):
                e = etree.Element(field)
                e.text = kw[field]
                element.append(e)


    def _bgpStatus(self, kw):
        try:

            pdb.set_trace()
            
            fields = ["logical-system", "instance", "neighbor-address"]

            rpc = etree.Element("rpc")
            bgp = etree.Element("get-bgp-neighbor-information")
            rpc.append(bgp)

            self._populate(bgp, fields, kw)

            self._send_msg( etree.tostring(rpc, pretty_print=True) )
            response = self._read_msg()
    #        print "response from device:\n", response
            return response

        except ex:
            print "error sending a ping"
    

    
    def handler(self, todo):
        if (todo[0] == 'ping'):
#            pdb.set_trace()
            return self._ping(todo[1])
        elif (todo[0] == 'bgp'):
            return self._bgpStatus(todo[1])
        else:
            pass


