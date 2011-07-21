module alloy/test
open util/ordering[Time] as history
--open util/ordering[Num] as circle

/* ========================================================================
TEMPORAL STRUCTURE
   Note that if time has a scope of x, there will be exactly x times, not
x or fewer.
======================================================================== */

sig Time { }

abstract sig Event {
   pre:  Time,
   post: Time,
   cause: lone (Event-Null),
}
{ post = pre.next }

sig Null extends Event { } { no cause }

fact TemporalStructure1 {
   all t: Time - history/last |
      one e: Event | e.pre = t and e.post = t.next 
}

fact TemporalStructure2 { all t: history/last | no e: Event | e.pre = t }

--fact CauseHasSingleEffect { cause in Event lone -> Event }

fact CausePrecedesEffect {
   all e1, e2: Event | e1 in e2.(^cause) => lt[e1.pre,e2.pre] }

one sig initEvent extends Event {
 f_Loc: Node
}{no cause}

abstract sig UserEvent extends Event {
} {one cause}



pred show{}
run show for 1 but  1 Node, 4 Event, 5 Time
