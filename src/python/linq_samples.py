"""
$Id: linq_samples.py 265 2009-07-16 19:24:16Z maoy $
illustrate some of the 101 LINQ Samples
http://msdn.microsoft.com/en-us/vcsharp/aa336746.aspx
"""
from linq import LinqQuery, NamedTuple, fromEach

def linq1():
    """where simple 1
    """
    numbers = [ 5, 4, 1, 3, 9, 8, 6, 7, 2, 0 ]
    lowNums = fromEach(n=numbers) \
        .where( lambda t: t.n < 5 ) \
        .select( lambda t: t.n )
    print "Numbers < 5:"
    for x in lowNums:
        print x

def linq28():
    """
    OrderBy - Simple 1

    This sample prints an alphabetically sorted version of an input
    string array. The sample uses orderby to perform the sort.
    """
    words = ["cherry", "apple", "blueberry"]
    sortedWords = fromEach(words) \
        .orderBy()
    print "The sorted list of words:"
    for w in sortedWords:
        print w

def linq29():
    """
    OrderBy - Simple 2

    This sample prints a version of an input string array sorted by the
    length each element. The sample uses orderby to sort the words.
    """
    words = ["cherry", "apple", "blueberry"]
    sortedWords = fromEach(words) \
        .orderBy(lambda w: len(w))
    print "The sorted list of words (by length):"
    for w in sortedWords:
        print w
    
    
def linq40():
    """
    GroupBy simple 1
    """
    numbers = [ 5, 4, 1, 3, 9, 8, 6, 7, 2, 0 ]
    numberGroups = fromEach( n=numbers ) \
        .groupBy(lambda t: t.n%5) \
        .select(lambda g: NamedTuple(remainder=g.key, numbers=g) )
    for g in numberGroups:
        print "Numbers with a remainder of %d when divided by 5:" % (g.remainder)
        for i in g.numbers:
            print i.n

def linq41():
    """
    GroupBy simple 2
    """
    words = [ "blueberry", "chimpanzee", "abacus", "banana", "apple", "cheese" ]
    wordGroups = fromEach( w=words) \
        .groupBy( lambda t: t.w[0] )
    for g in wordGroups:
        print "Words that start with the letter '%s':" %(g.key,)
        for i in g: 
            print i.w

def linq48():
    """
    Union - 1 
    
    This sample prints the unique elements of two integer arrays. The
    sample uses the Union method to create a sequence that is a union
    of the two integer arrays with duplicate elements removed.

    """
    numbersA = [ 0, 2, 4, 5, 6, 8, 9 ]
    numbersB = [ 1, 3, 5, 7, 8 ]
    
    uniqueNumbers = fromEach(numbersA) \
        .union( numbersB )

    print "Unique numbers from both arrays:"
    for n in uniqueNumbers:
        print n

def linq67():
    """
    Any - Simple

    This sample determines whether any words in a string array contain
    the character sequence 'ei'. It uses Any, passing a lambda
    expression that uses the Contains method to perform the check.
    """
    words = [ "believe", "relief", "receipt", "field" ]
    iAfterE = fromEach(words).any(lambda w: w.find('ei')>=0)
    print "There is a word that contains in the list that contains 'ei':", iAfterE

def linq70():
    """
    All - Simple

    This sample uses All to determine whether an array contains only
    odd numbers.
    """
    numbers = [1, 11, 3, 19, 41, 65, 19]
    onlyOdd = fromEach(numbers).all( lambda n: n%2==1 )
    print "The list contains only odd numbers:", onlyOdd

def linq78():
    """
    Sum - Simple

    This sample used Sum to find the total of all of the numbers in an integer array.
    """
    numbers = [ 5, 4, 1, 3, 9, 8, 6, 7, 2, 0 ]
    numSum = fromEach(numbers).sum()
    print "The sum of the numbers is", numSum

def linq79():
    """
    Sum - Projection

    This sample uses Sum to find the total number of characters in all
    of the words in a string array.
    """
    words = [ "cherry", "apple", "blueberry"]
    totalChars = fromEach(words) \
        .sum(lambda w: len(w))
    print "There are a total of %d characters in these words." % (totalChars,)

if __name__ == '__main__':
    linq1()
    linq28()
    linq29()
    linq40()
    linq41()
    linq48()
    linq67()
    linq70()
    linq78()
    linq79()
