from linq import NamedTuple, LinqQuery, fromEach

def named_tuple_factory(cursor, row):
    t = NamedTuple()
    for idx, col in enumerate(cursor.description):
        t[col[0]] = row[idx]
    return t
import sqlite3

def sqlite3ConnectionFactory(dbname):
    con = sqlite3.connect(dbname)
    con.row_factory = named_tuple_factory
    con.text_factory = sqlite.OptimizedUnicode #ASCII is in str by default
    return con

from table import BaseTable

class SQLSource(object):
    def __init__(self, txn, db, tablename):
        self.txn = txn
        self.db = db
        self.tablename = tablename

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
        
    def __iter__(self):
        cols = "*"
        sql = "SELECT %s FROM %s" % (cols, self.tablename)
        return self.sql_iter(sql)
    
    def where(self, **kw):
        if len(kw) == 0:
            sql = "SELECT * FROM %s" % (self.tablename)
        else:
            sql = "SELECT * FROM %s WHERE " % (self.tablename)
            cond = ["%s=?" % (field,) for field in kw.keys()]
            sql += ' AND '.join(cond)
            #print sql
        return self.sql_iter( sql, kw.values() )

if __name__ == '__main__':
    from db import *
    txn = TransactionContext()
    tLink = SQLSource(txn, "/home/maoy/test.db", "link")

    # from link1 in tLink, link2 in tLink
    # where link1.Dest == link2.Src
    # select new {Src=link1.Src, Dest=link2.Dest, Hop2Cost=link1.Cost+link2.Cost}

    x = fromEach( link1=tLink, link2=tLink ) \
        .where( lambda t: t.link1.Dest == t.link2.Src ) \
        .select( lambda t: \
                     NamedTuple( Src=t.link1.Src, \
                                     Dest=t.link2.Dest, \
                                     Hop2Cost=t.link1.Cost+t.link2.Cost) )
    for i in x:
        print i

