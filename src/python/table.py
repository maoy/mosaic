"""

  $Id: table.py 264 2009-07-15 17:44:27Z maoy $

"""
from db import *

class MDBException(Exception):
    pass
class DuplicatedKeyError(MDBException):
    pass
class NoKeyError(MDBException):
    pass
class ConstraintError(MDBException):
    pass

class BaseTable(object):
    """
    The abstraction of a DB table with only 1 index being
    the primary key

    all operations are row-based
    """

    pkf = None
    def __init__(self, *pkeys):
        self.pkf = tuple(sorted(pkeys))
    def insert(self, tp):
        """
        insert the tuple tp, and return (rowid, rows_affected, full_tuple)
        full_tuple has the missing fields that are automatically determined,
        e.g. a key with AUTOINCREMENT, and columns with default values
        """
        raise NotImplementedError
    def remove(self, tp):
        """
        Remove a tuple from the table, and return (None, rows_affected, full_tuple).
        Columns of tp must be a superset of what primary key contains.
        """
        raise NotImplementedError
    def update(self, tp):
        """
        Replace a tuple from the table, and return (rowid, rows_affected, old_tuple, full_tuple)
        Columns of tp must be a superset of what primary key contains.
        """
        raise NotImplementedError
    def upsert(self, tp):
        """
        Replace a tuple from the table, or insert a new one if old_tuple does not exist, 
        and return (rowid, rows_affected, old_tuple/None, full_tuple).
        Columns of tp must be a superset of what primary key contains.
        old_tuple is None when it is an insertion, otherwise it's an update
        """
        raise NotImplementedError
    def iter(self, **kw):
        raise NotImplementedError
    def lookup(self, cols, conditions):
        """
        This is equiv to SELECT ck1 as cv1, ck2 as cv2, ... FROM this WHERE condk1=condv1,...
        cols and conditions are two dicts
        if cols = {}, then it is assume to be SELECT *
        """
        raise NotImplementedError

    def undo(self,*record):
        print "undoing", record
        tbl = record[0]
        action = record[1]
        if action=="INSERT":
            tp = record[2]
            tbl.remove(tp)
        elif action=="UPDATE":
            old_tp = record[2]
            # new_tp = record[3]
            tbl.update(old_tp)
        elif action=="REMOVE":
            tp = record[2]
            tbl.insert(old_tp)

class MemTable(BaseTable):
    """
    an in-memory hash table implementation
    """
    data = {}
    def insert(self, tp):
        key = tp.project(self.pkf)
        if self.data.has_key(key):
            raise DuplicatedKeyError
        self.data[key] = tp
    
    def removeKey(self, key):
        if self.data.has_key(key):
            tp = self.data[key]
            del self.data[key]
            return tp
        else:
            raise NoKeyError
    def remove(self, tp):
        key = tp.project(self.pkf)
        if self.data.has_key(key):
            existing_tp = self.data[key]
            if existing_tp != tp:
                return tp
            del self.data[key]
            return tp
        else:
            raise NoKeyError

    def update(self, tp):
        key = tp.project(self.pkf)
        if self.data.has_key(key):
            old_tp = self.data[key]
        else:
            raise NoKeyError
        self.data[key] = tp
        return old_tp

    def upsert(self, tp):
        try:
            return self.update(tp)
        except NoKeyError:
            self.insert(tp)
            return None
        assert(False)

    def lookup(self, pk):
        if self.data.has_key(pk):
            return self.data[pk]
        raise NoKeyError

    def iter(self, **kw):
        for t in self.data.itervalues():
            r = Tuple("noname");
            if len(kw)==0:
                yield t
            else:
                for field,var in kw.items():
                    assert(not r.value.has_key(var))
                    r[var] = t[field]
                yield r
    def where(self, **kw):
        for t in self.data.itervalues():
            match = True
            for k,v in kw.items():
                if t[k]!=v: 
                    match = False
                    break
            if match: 
                yield t
        
import sqlite3
def sqlite3ConnectionFactory(dbname):
    con = sqlite3.connect(dbname)
    con.row_factory = tuple_factory
    con.text_factory = sqlite.OptimizedUnicode #ASCII is in str by default
    return con

class SQLTable(BaseTable):
    tablename = None
    def __init__(self, txn, db, tablename, *pkeys):
        self.txn = txn
        self.tablename = tablename
        self.db = db
        super(SQLTable,self).__init__(*pkeys)

    def _exec_sql(self, sql, params):
        con = self.txn.getDBConnection(self.db, sqlite3ConnectionFactory)
        cur = con.cursor()
        print "in _exec_sql", sql,params
        cur.execute(sql, params)
        cur.close()

    def sql_iter(self, sql, params={}):
        con = self.txn.getDBConnection(self.db, sqlite3ConnectionFactory)
        cur = con.cursor()
        print "in sql_iter", sql,params
        cur.execute(sql, params)
        row = cur.fetchone()
        while row:
            yield row
            row = cur.fetchone()
        cur.close()
        
    def insert(self, tp):
        columns = ",".join(tp.keys())
        place_holders = "?,"*(len(tp.keys())-1) + "?"
        sql = "INSERT INTO %s (%s) VALUES (%s)" % (self.tablename, columns, place_holders)
        return self._exec_sql(sql, tp.values())

    def remove(self, tp):
        sql = "DELETE FROM %s WHERE " % (self.tablename)
        cond = ["%s=?" % (field,) for field in tp.keys()]
        sql += ' AND '.join(cond)
        return self._exec_sql( sql.tp.values() )

    def update(self, tp):
        key = tp.project(self.pkf)        
        set_sql = ", ".join( ["%s=?" %(field,) for field in tp.keys()] )
        set_params = tp.values()
        where_sql = " AND ".join( ["%s=?" %(field,) for field in self.pkf] )
        where_params = key.values()
        sql = "UPDATE %s SET %s WHERE %s" % (self.tablename, set_sql, where_sql)
        return self._exec_sql(sql, set_params+where_params )

    def iter(self, **kw):
        if len(kw)==0:
            cols = "*"
        else:
            col_list = []
            for field,var in kw.items():
                col_list.append( "%s AS %s" %(field, var))
            cols = ','.join(col_list)
        sql = "SELECT %s FROM %s" % (cols, self.tablename)
        return self.sql_iter(sql)
    
    def where(self, **kw):
        if len(kw)==0:
            sql = "SELECT * FROM %s" % (self.tablename)
        else:
            sql = "SELECT * FROM %s WHERE " % (self.tablename)
            cond = ["%s=?" % (field,) for field in kw.keys()]
            sql += ' AND '.join(cond)
            #print sql
        return self.sql_iter( sql, kw.values() )
    def undo(self, *record):
        """
        do nothing because SQLite rollback will handle it
        """
        print "in undo: do nothing"
        print record

class InterfaceTable(SQLTable):
    def __init__(self, txn):
        db = "/home/maoy/test.db"
        tablename = "interface" #if_id, router_ip, status
        super(InterfaceTable, self).__init__(txn, db, tablename, 'oid')
    def update(self, tp):
        print "calling router function to do ifconfig...", tp
        import time
        time.sleep(1)
        print "ifconfig done"
        super(InterfaceTable,self).update(tp)
    def undo(self, *record):
        pass
        

class Statement(object):
    """
    handle INSERT, REMOVE, UPDATE and UPSERT
    """
    def __init__(self, txn, tbl):
        self.txn = txn
        self.tbl = tbl
        self.queue = []
        self.lastrowid = None

    def fetch_all(self, seq):
        for i in seq:
            self.queue.append(i)

    def insert(self):
        for tp in self.queue:
            self.tbl.insert(tp)
            self.txn.saveLog(self.tbl, "INSERT", tp)
        self.queue = []
    
    def remove(self):
        for pk in self.queue:
            tp = self.tbl.remove(pk)
            self.txn.saveLog(self.tbl, "REMOVE", tp)
        self.queue = []
    def update(self):
        for tp in self.queue:
            old_tp = self.tbl.update(tp)
            self.txn.saveLog(self.tbl, "UPDATE", old_tp, tp)
        self.queue = []
    def upsert(self):
        for tp in self.queue:
            old_tp = self.tbl.upsert(tp)
            if old_tp is None:
                self.txn.saveLog(self.tbl, "INSERT",tp)
            else:
                self.txn.saveLog(self.tbl, "UPDATE", old_tp, tp)

def sampleStmt(txn):
    tLink = SQLTable(txn,"/home/maoy/test.db","link")
    tHop2 = SQLTable(txn, "/home/maoy/test.db", "hop2")
    stmt = Statement(txn, tHop2)
    # SELECT INTO hop2 FROM link l1, link l2 WHERE l1.Dest=l2.Src
    seq = tLink.iter(Src='X',Dest='Y')
    seq = join(seq, tLink, Src='Y', Dest='Z')
    seq = selection(seq, lambda t: t.X!=t.Z)
    seq = projection(seq, {'X':'Src','Z':'Dest'})
    stmt.fetch_all(seq)
    stmt.insert()
    #now data are written to mem/disk async

def testTxn():
    txn = TransactionContext()
    try:
        sampleStmt(txn)
        #view maintainance

        #check non-deferred constraints
        #raise ConstraintError
        #call all triggers

        print "committing"
        txn.commit()
        print "commited"
    except MDBException, e:
        print "aborting", e
        
        txn.rollback()

if __name__=='__main__':
    testTxn()
