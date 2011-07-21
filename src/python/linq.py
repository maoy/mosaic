"""
$Id: linq.py 265 2009-07-16 19:24:16Z maoy $

Linq-like operators for python
"""

# pylint: disable-msg=C0103

import itertools

def identity(x):
    return x

def op_where(seq, func):
    """
    op_where: a' list -> (a' -> bool) -> a' list
    Return those items of sequence for which function(item) is true. 
    """
    return itertools.ifilter(func, seq)

def op_select(seq, func):
    """
    op_select: a' list -> (a'->b') -> b' list
    """
    return itertools.imap(func, seq)

def op_selectMany(seq, func):
    """
    op_selectMany: a' list -> (a' -> b' list) -> b' list
    """
    for i in seq:
        fseq = func(i)
        for j in fseq:
            yield j

def op_take(seq, count):
    taken = 0
    it = iter(seq)
    while taken < count:
        taken += 1
        yield it.next()

def _group_lambda_func(t):
    return GroupedQuery( t[0], t[1] )

def op_groupByHash(iterable, key):
    """
    use a hash table to group by keys
    """
    if key is None:
        key = identity
    data = {}
    for item in iterable:
        if data.has_key( key(item) ):
            data[ key(item) ].append( item )
        else:
            data[ key(item) ] = [item]
    return op_select(data.iteritems(), _group_lambda_func)

def op_groupBest(git, keyfunc):
    for group in git:
        best = None
        for value in group:
            if best is None or keyfunc(value) < keyfunc(best):
                best = value
        yield best

def op_groupWorst(git, keyfunc):
    for group in git:
        worst = None
        for value in group:
            if worst is None or keyfunc(value) > keyfunc(worst):
                worst = value
        yield worst

def op_join(inner, outer_name, outer, selector=None):
    """
    inner must be a query of NamedTuple
    """
    for inner_item in inner:
        for outer_item in outer:
            #print "inner=",inner_item, "outer=", outer_item
            res = inner_item.append(outer_name, outer_item)
            #print 'res=',res
            if selector is None or selector(res):
                yield res

def op_union(lhs, rhs):
    """
    union operator -- duplicated stuff is deleted
    """
    seen = set()
    for source in (lhs, rhs):
        for item in source:
            if item not in seen:
                seen.add( item )
                yield item

class NamedTuple(object):
    value = {}
    def __init__(self, value={}, **kw):
        if len(value) == 0:
            value = kw
        else:
            assert len(kw)==0, "wrong usage of Tuple"
        self.value = value
    def __getattr__(self, name):
        return self.value[name]
    def __repr__(self):
        return "%s" % (str(self.value))
    def __getitem__(self, key):
        return self.value[key]
    def __setitem__(self, key, value):
        self.value[key] = value
    def __eq__(self, other):
        return self.value==other.value
    def __ne__(self, other):
        return not self.__eq__(other)
    def append(self, name, value):
        res = NamedTuple({})
        res.value.update(self.value)
        res.value[name] = value
        return res
    def project(self, pk):
        return tuple([self.value[field] for field in pk])
    def keys(self):
        return self.value.keys()
    def values(self):
        return self.value.values()
    def has_key(self, key):
        return self.value.has_key(key)
    def to_tuple(self):
        """
        convert it to a python tuple (so that it is hashable).
        the attribute names are discarded, and the values are ordered
        based on the attribute names.
        """
        return tuple([self.value[field] 
                      for field in sorted(self.value.keys())]
                     )

class NamedSeq:
    def __init__(self, name,  seq):
        self.seq = seq
        self.name = name        
    def __iter__(self):
        for item in self.seq:
            value = {self.name:item}
            yield NamedTuple(value)

def record(**kw):
    return kw

class LinqQuery(object):
    """
    Representing a Linq Query
    """
    def __init__(self, query, named):
        self.named = named
        self.query = query
    def __iter__(self):
        return planner(self.query)
    @staticmethod
    def fromSeq(seq, name = None):
        if name is None:
            return LinqQuery([("Seq", seq)], False)
        else:
            return LinqQuery([("NamedSeq", seq, name)], True)
    def _append_query(self, *q):
        expr = self.query[:]
        expr.append(q)
        return LinqQuery(expr, self.named)
    def where(self, func):
        return self._append_query("Where", func)
    def select(self, func):
        return self._append_query("Select", func)
    def selectMany(self, func):
        return self._append_query("SelectMany", func)
    def join(self, outer_name, outer, selector):
        assert self.named, "must be a named query before join"
        if not isinstance(outer, LinqQuery):
            outer = LinqQuery.fromSeq(outer)
        return self._append_query("Join", outer_name, outer, selector)
    def crossProduct(self, outer_name, outer):
        return self.join(outer_name, outer, None)
    def take(self, count):
        return self._append_query("Take", count)
    def groupBy(self, keyfunc):
        return self._append_query("GroupBy", keyfunc)
    def orderBy(self, keyfunc=identity):
        return self._append_query("OrderBy", keyfunc)
    def union(self, rhs):
        return self._append_query("Union", rhs)
    def orderByDescending(self, keyfunc=identity):
        return self._append_query("OrderByDescending", keyfunc)
    # following are greedy operators
    def max(self, keyfunc=None):
        it = iter(self)
        if keyfunc is None:
            return max(it)
        else:
            return max(it, keyfunc)
    def min(self, keyfunc=None):
        it = iter(self)
        if keyfunc is None:
            return min(it)
        else:
            return min(it, keyfunc)
    def count(self, keyfunc=None):
        it = iter(self)
        if keyfunc is None:
            c = 0
            for i in it: 
                c += 1
            return c
        else:
            c = 0
            for i in it: 
                if keyfunc(i): 
                    c += 1
            return c
    def sum(self, keyfunc=None):
        it = iter(self)
        if keyfunc is None:
            return sum(it)
        else:
            return sum(op_select(it, keyfunc))
    def any(self, pred = None):
        it = iter(self)
        if pred == None:
            #no predicate, return true if there is anything
            try:
                it.next()
            except StopIteration:
                return False
            return True
        # there is a predicate
        for i in it:
            if pred(i): return True
        return False
    def all(self, pred):
        for i in iter(self):
            if not pred(i): return False
        return True #return True even if it is an empty sequence
    def contains(self, tp):
        for i in iter(self):
            if i==tp: return True
        return False
def fromEach(*k, **kw):
    """
    a shortcut to create a linq query
    """
    if len(k)==1 and len(kw)==0:
        #unamed sequence
        return LinqQuery.fromSeq(k[0])
    elif len(k)==0 and len(kw)>=1:
        #named sequence. i.e. the FROM clause
        res = None
        for name, seq in kw.items():
            if res is None:
                #first sequence
                res = LinqQuery.fromSeq( seq, name )
            else:
                res = res.crossProduct( name, seq )
        return res
    raise RuntimeError, "wrong usage of linq %s, %s" %( k, kw )
class GroupedQuery(LinqQuery):
    """
    After groupBy, each group has a key, and is a sequence of LinqQuery
    """
    def __init__(self, key, seq):
        self.key = key
        super(GroupedQuery, self).__init__( [("Seq", seq)], False )

def planner(query):
    """
    decide which planner to use
    """
    assert len(query)>=1
    return simple_planner(query)

def simple_planner(query):
    """
    the most straightforward planning algorithm
    """
    it = None
    grouped = False
    for term in query:
        if term[0] == "Seq":
            grouped = False
            it = iter(term[1])
        elif term[0] == "NamedSeq":
            grouped = False
            it = NamedSeq( term[2], term[1] )
        elif term[0] == "Where":
            it = op_where(it, term[1])
        elif term[0] == "Join":
            it = op_join(it, term[1], term[2], term[3])
        elif term[0] == "Select":
            grouped = False
            it = op_select(it, term[1])
        elif term[0] == "SelectMany":
            grouped = False
            it = op_selectMany(it, term[1])
        elif term[0] == "OrderBy":
            if not grouped:
                it = iter( sorted(it, key=term[1]) )
            else:
                it = op_groupBest(it, term[1])
        elif term[0] == "OrderByDescending":
            if not grouped:
                it = iter( sorted(it, key=term[1], reverse=True) )
            else:
                it = op_groupWorst(it, term[1])
        elif term[0] == "Take":
            it = op_take(it, term[1])
        elif term[0] == "GroupBy":
            it = op_groupByHash(it, term[1])
            grouped = True
        elif term[0] == "Union":
            it = op_union(it, term[1])
        else:
            raise RuntimeError("unknown query term %s" % (term,) )
    return it

def test():
    x = NamedSeq('a', [1, 2, 3])
    y = NamedSeq('b', [2, 3, 4])
    tmp = op_where(x, lambda t: t.a > 1)
    tmp = op_join(tmp, 'c', [2, 3, 4], lambda t: t.a == t.c)
    for i in tmp: 
        print i

def test3():
    print "test3"
    x = LinqQuery.fromSeq([1, 2, 3],'a') \
        .where(lambda t:t.a > 1) \
        .join( 'b', [2, 3, 4], lambda t: t.a==t.b)
    print "query is", x.query
    for i in x: print i

    x = LinqQuery.fromSeq([5, 4, 1, 3, 9, 8, 6, 7, 2, 0]) \
        .groupBy(lambda x: x%5) \
        .orderByDescending(lambda g: g)
    print x
    for i in x:
        print "remainder:", i
        #for n in i: print n

    x = LinqQuery.fromSeq([5, 4, 1, 3, 9, 8, 6, 7, 2, 0]) \
        .groupBy(lambda x: x%5) \
        .select(lambda g: (g.key, g.orderBy(identity).take(1)))
    for i in x:
        print ":", i

    words = [ "blueberry", "chimpanzee", "abacus", "banana", "apple", "cheese" ]
    wordGroups = LinqQuery.fromSeq( words, 'w' ) \
        .groupBy( lambda t: t.w[0] )
    for g in wordGroups:
        print "starting letter:", g.key
        for i in g: print i.w
        
if __name__=='__main__':
    test3()

