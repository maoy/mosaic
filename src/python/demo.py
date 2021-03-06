import itertools
#from pysqlite2 import dbapi2 as sqlite
import sqlite3 as sqlite

class Tuple(object):
    """
    Tuple is the class for tuple with named attributes
    """
    name = None
    value = {}
    def __init__(self, *w, **kw):
        self.name = w[0]
        self.value = kw
    def project(self, pk):
        return tuple([self.value[field] for field in pk])
    def __repr__(self):
        return "%s(%s)" % (self.name, str(self.value))
    def __getattr__(self,name):
        return self.value[name]
    def __getitem__(self, key):
        return self.value[key]
    def __setitem__(self, key, value):
        self.value[key] = value
    def __eq__(self, other):
        return self.name==other.name and self.value==other.value
    def __ne__(self, other):
        return not self.__eq__(other)
    def append(self, t):
        r = Tuple("",{})
        r.value.update(self.value)
        r.value.update(t.value)
        return r
    def keys(self):
        return self.value.keys()
    def values(self):
        return self.value.values()
    def has_key(self, key):
        return self.value.has_key(key)
    def to_tuple(self):
        """
        convert it to a python tuple (so that it is hashable).
        the attribute names are omitted, and the values are ordered
        based on the attribute names.
        """
        return tuple([self.value[field] 
                      for field in sorted(self.value.keys())]
                     )

def tuple_factory(cursor, row):
    t = Tuple("sqlite")
    for idx, col in enumerate(cursor.description):
        t[col[0]] = row[idx]
    return t



class TransactionContext(object):
    def __init__(self):
        self.log = []
        self.db_conn = {}
        self.state = "BEGIN"
    def getDBConnection(dbstr, dbFactory):
        if self.db_conn.has_key(dbstr):
            return self.db_conn[dbstr]
        conn = dbFactory.createConn(dbstr)
        self.db_conn[dbstr] = conn
        return conn

    def log(self, *record ):
        self.log.append( record )
    def commit(self):
        # TODO: check deferred constraints
        
        # assuming commit always succeed
        self.state = "COMMITTING"
        for k, conn in self.db.conn.items():
            conn.commit()
            conn.close()
            del self.db_conn[k]
        self.state = "COMMITTED"
    def rollback(self):
        self.state = "ABORTING"
        for rec in reversed(self.log):
            rec[0].undo(rec)
        
        for k, conn in self.db.conn.items():
            conn.rollback()
            conn.close()
            del self.db_conn[k]
        self.state = "ABORTED"

class BaseTable(object):
    pkf = None
    def __init__(self, *pkeys):
        self.pkf = tuple(sorted(pkeys))
    def phyInsertRow(self, t):
        """
        t is a Tuple type
        """
        raise NotImplementedError
    def phyRemoveRowByKey(self, pk):
        raise NotImplementedError
    def phyUpdateRow(self, oldt, newt):
        raise NotImplemetnedError

    def insertRow(self, t, stmt):
        stmt.insertQ.append( (self,t) )
        
    def removeRowByKey(self, pk, stmt):
        stmt.removeQ.append( (self,pk) )
        
    def updateRow(self, oldt, newt):
        stmt.updateQ.append( (self,oldt,newt) )
    


class Table(object):
    data = {}
    pkf = None #primary key field names
    def __init__(self, *pkeys):
        self.pkf = tuple(sorted(pkeys))
    def insertTuple(self, t):
        key = t.project(self.pkf)
        if self.data.has_key(key):
            if self.data[key]==t:
                return False # already have it
            else:
                self.data[key] = t
                return True
        else:
            self.data[key] = t
            return True
    def removeTuple(self, t):
        key = t.project(self.pkf)
        if self.data.has_key(key):
            del self.data[key]
            return True
        return False
    def where(self, **kw):
        for t in self.data.itervalues():
            match = True
            for k,v in kw.items():
                if t[k]!=v: 
                    match = False
                    break
            if match: 
                yield t
    def iter(self, **kw):
        for t in self.data.itervalues():
            r = Tuple("noname");
            for field,var in kw.items():
                assert(not r.value.has_key(var))
                r[var] = t[field]
            yield r

def sql_iter(db, sql, param={} ):
    con = sqlite.connect(db)
    con.row_factory = tuple_factory
    con.text_factory = sqlite.OptimizedUnicode #ASCII is in str by default
    cur = con.cursor()
    cur.execute(sql, param)
    row = cur.fetchone()
    while row:
        yield row
        row = cur.fetchone()
    cur.close()
    con.close()
    
class SQLiteTable(object):
    def __init__(self, db, sql):
        self.db = db
        self.sql = sql
    def iter(self, **kw):
        assert(len(kw)==0)
        return sql_iter(self.db, self.sql)
    def where(self, **kw):
        if len(kw)==0:
            sql = self.sql
        else:
            sql = "SELECT * FROM (%s) WHERE " % (self.sql)
            cond = ["%s=?" % (field,) for field in kw.keys()]
            sql += ' AND '.join(cond)
            #print sql
        return sql_iter( self.db, sql, kw.values() )
        
def join(seq, tbl, **kw):
    """join variables that are already bounded in seq
    and select variables that are not bounded
    kw: field=var
    """
    for t in seq:
        join_field = {}
        sel_field = {}
        for field,var in kw.iteritems():
            if t.has_key(var): #is var already bounded?
                join_field[field] = t[var]
            else:
                sel_field[field] = var
        #print 'join_field=',join_field
        for t2 in tbl.where(**join_field):
            r = Tuple(t.name, **t.value)
            for field,var in sel_field.iteritems():
                r[var]=t2[field]
            yield r

def assign(seq, varname, f):
    for t in seq:
        assert(not t.has_key(varname))
        r = Tuple(t.name, **t.value)
        r[varname] = f(t)
        yield r

def projection(seq, fields, distinct = True):
    seen = {}
    for t in seq:
        r = Tuple(t.name)
        for f in fields:
            r[f] = t[f]
        if distinct:
            tr = r.to_tuple()
            if not seen.has_key(tr):
                seen[tr] = True
                yield r
        else:
            yield r

def selection(seq, pred):
    return itertools.ifilter(pred, seq)

def aggregation(seq, groupby_field, agg):
    (aggfunc, aggname, aggarg) = agg
    res = {}
    for t in seq:
        gk = t.project(groupby_field)
        if not res.has_key(gk):
            res[gk] = aggfunc()
        f = res[gk]
        f.step(aggarg(t))
    for pk,v in res.iteritems():
        r = Tuple("agg")
        for i in range(len(groupby_field)):
            r[ groupby_field[i] ] = gk[i]
        r[aggname] = v.finalize()
        yield r

class Aggsum:
    def __init__(self):
        self.sum = 0

    def step(self, value):
        self.sum += value

    def finalize(self):
        return self.sum

def query_test(link):
    seq = link.iter(src='X',dest='Y',cost='C1') #link(X,Y,C1)
    seq = join(seq, link, src='Y', dest="Z", cost='C2') #link(Y,Z,C2)
    seq = assign(seq, 'C', lambda x: x.C1+x.C2) #C:=C1+C2
    seq = selection(seq, lambda x:x.C<4) # C<4
    seq = projection(seq, ('X','Z','C')) # drop Y
    seq = aggregation( seq, ('X','Z'), (Aggsum, "sum_C",lambda x: x['C']) )
    for i in seq: print i

def tmp(link):
    s = SQLiteTable("/tmp/test.db", "SELECT * FROM test")
    seq = s.where(id=1,t='test1')
    seq = join(seq, link, cost='id', src='src',dest='dest')
    for i in seq: print i
    
def demo():
    txn = db.beginTxn()
    stmt = txn.startStatement()


if __name__ == '__main__':
    l1 = Tuple("link",src="A",dest="B",cost=1)
    link = Table('src','dest')
    link.insertTuple(l1)
    l2 = Tuple("link",src="B",dest="C",cost=2)
    link.insertTuple(l2)
    l2 = Tuple("link",src="D",dest="C",cost=1)
    link.insertTuple(l2)
    l2 = Tuple("link",src="A",dest="D",cost=1)
    link.insertTuple(l2)
    query_test(link)
    tmp(link)



"""
for link in tLink.objects.all():
    for link2 in tLink.objects.filter(Src=link.dest):
        if link2["Dest"]!=link["Src"]:
            Tuple(link.src, link.dest)
"""
