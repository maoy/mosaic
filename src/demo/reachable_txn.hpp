/* $Id: reachable_txn.hpp 225 2009-06-16 20:49:02Z maoy $ */

enum {
  ID_unused = TUPLE_RESERVED
  ,ID_Tuple_reachable
  ,ID_Tuple_link
  ,ID_Tuple_ms_periodic0
  ,ID_Tuple_ms_periodic1
  ,ID_Tuple_ms_periodic2
  ,ID_Tuple_ms_periodic3
  ,ID_Tuple_ms_periodic4
};
typedef tuple< gc_string, gc_string > type_tuple_reachable;
DEFINE_TUPLE( reachable )
typedef RefTable< Tuple_reachable,Keys<  >, Keys< 0 > > Table_reachable;
//Table_reachable t_reachable(&taskQ, true);
typedef tuple< gc_string, gc_string > type_tuple_link;
DEFINE_TUPLE( link )
typedef RefTable< Tuple_link,Keys< 0,1 >, Keys< 1 > > Table_link;
//Table_link t_link(&taskQ, false);
const gc_string _ms_str_const_0 = gc_string("reachable");
const gc_string _ms_str_const_1 = gc_string("C");
const gc_string _ms_str_const_2 = gc_string("B");
const gc_string _ms_str_const_3 = gc_string("A");
typedef tuple< int32_t, boost::asio::deadline_timer* > type_tuple_ms_periodic0;
DEFINE_TUPLE( ms_periodic0 )
typedef tuple< int32_t, boost::asio::deadline_timer* > type_tuple_ms_periodic1;
DEFINE_TUPLE( ms_periodic1 )
typedef tuple< int32_t, boost::asio::deadline_timer* > type_tuple_ms_periodic2;
DEFINE_TUPLE( ms_periodic2 )
typedef tuple< int32_t, boost::asio::deadline_timer* > type_tuple_ms_periodic3;
DEFINE_TUPLE( ms_periodic3 )
typedef tuple< int32_t, boost::asio::deadline_timer* > type_tuple_ms_periodic4;
DEFINE_TUPLE( ms_periodic4 )

#define WR_TABLE_IN_WSP(name) static_cast<Table_##name *>(ctx.wsp->getWriteTable(ID_Tuple_##name))
#define RD_TABLE_IN_WSP(name) static_cast<Table_##name *>(ctx.wsp->getReadTable(ID_Tuple_##name))


#define WSP_TABLE_SECONDARY_LOOKUP(tablename,level,key,key_type) \
  typedef Table_##tablename::container_type::index< key_type >::type::const_iterator iter_type_##level; \
  iter_type_##level it_begin_##level, it_end_##level;			\
  key_type _key_inst_##level;						\
  tie(it_begin_##level,it_end_##level) =                                \
    static_cast<Table_##tablename*>(ctx.wsp->getReadTable(ID_Tuple_##tablename))->secondaryLookup(_key_inst_##level, key); \
  TABLE_ITER(tablename,level)

//# 4 "reachable.mos"
void ins_link_handler_0(Tuple* ms_t, TxnContext& ctx){
  Tuple_link *t0 = static_cast<Tuple_link*>(ms_t);
  gc_string const& S = boost::get<0>(*t0);
  gc_string const& D = boost::get<1>(*t0);
  //t_reachable.insert( new Tuple_reachable(S, D));
  WR_TABLE_IN_WSP(reachable)->insert( new Tuple_reachable(S, D), &ctx.deltaQ );
}

//# 4 "reachable.mos"
void del_link_handler_0(Tuple* ms_t, TxnContext& ctx){
  Tuple_link *t0 = static_cast<Tuple_link*>(ms_t);
  gc_string const& S = boost::get<0>(*t0);
  gc_string const& D = boost::get<1>(*t0);
  WR_TABLE_IN_WSP(reachable)->erase( new Tuple_reachable(S, D), &ctx.deltaQ );
}

//# 5 "reachable.mos"
void ins_link_handler_1(Tuple* ms_t, TxnContext& ctx){
  Tuple_link *t0 = static_cast<Tuple_link*>(ms_t);
  gc_string const& S = boost::get<0>(*t0);
  gc_string const& Z = boost::get<1>(*t0);
    typedef Keys<0> key_type_1;
    gc_string const&  ms_t1_key( Z );
    WSP_TABLE_SECONDARY_LOOKUP(reachable,1,&ms_t1_key,key_type_1){
    gc_string const& D = boost::get<1>(*t1);
    WR_TABLE_IN_WSP(reachable)->insert( new Tuple_reachable(S, D), &ctx.deltaQ );
    }
}

//# 5 "reachable.mos"
void del_link_handler_1(Tuple* ms_t, TxnContext& ctx){
  Tuple_link *t0 = static_cast<Tuple_link*>(ms_t);
  gc_string const& S = boost::get<0>(*t0);
  gc_string const& Z = boost::get<1>(*t0);
    typedef Keys<0> key_type_1;
    gc_string const&  ms_t1_key( Z );
    WSP_TABLE_SECONDARY_LOOKUP(reachable,1,&ms_t1_key,key_type_1){
    gc_string const& D = boost::get<1>(*t1);
    WR_TABLE_IN_WSP(reachable)->erase( new Tuple_reachable(S, D), &ctx.deltaQ );
    }
}

//# 5 "reachable.mos"
void ins_reachable_handler_0(Tuple* ms_t, TxnContext& ctx){
  Tuple_reachable *t0 = static_cast<Tuple_reachable*>(ms_t);
  gc_string const& Z = boost::get<0>(*t0);
  gc_string const& D = boost::get<1>(*t0);
    typedef Keys<1> key_type_1;
    gc_string const&  ms_t1_key( Z );
    WSP_TABLE_SECONDARY_LOOKUP(link,1,&ms_t1_key,key_type_1){
    gc_string const& S = boost::get<0>(*t1);
      WR_TABLE_IN_WSP(reachable)->insert( new Tuple_reachable(S, D), &ctx.deltaQ );
    }
}

//# 5 "reachable.mos"
void del_reachable_handler_0(Tuple* ms_t, TxnContext& ctx){
  Tuple_reachable *t0 = static_cast<Tuple_reachable*>(ms_t);
  gc_string const& Z = boost::get<0>(*t0);
  gc_string const& D = boost::get<1>(*t0);
    typedef Keys<1> key_type_1;
    gc_string const&  ms_t1_key( Z );
    WSP_TABLE_SECONDARY_LOOKUP(link,1,&ms_t1_key,key_type_1){
    gc_string const& S = boost::get<0>(*t1);
      WR_TABLE_IN_WSP(reachable)->erase( new Tuple_reachable(S, D), &ctx.deltaQ );
    }
}

//# 12 "reachable.mos"
void recv_ms_periodic0_handler_0(Tuple* ms_t, TxnContext& ctx){
  Tuple_ms_periodic0 *t0 = static_cast<Tuple_ms_periodic0*>(ms_t);
  RD_TABLE_IN_WSP(reachable)->dump();
  RD_TABLE_IN_WSP(link)->dump();
  if (boost::get<0>(*t0) >= 1) return;
  boost::get<0>(*t0)++ ;
  Event _e(Event::RECV, t0);
  callLater(1, boost::bind(demux, _e), boost::get<1>(*t0));
}

//# 14 "reachable.mos"
void ins_reachable_handler_1(Tuple* ms_t, TxnContext& ctx){
  Tuple_reachable *t0 = static_cast<Tuple_reachable*>(ms_t);
  gc_string const& X = boost::get<0>(*t0);
  gc_string const& Y = boost::get<1>(*t0);
    std::cout << "[ins]" << _ms_str_const_0<<' '<<X<<' '<<Y << std::endl;
}

//# 14 "reachable.mos"
void del_reachable_handler_1(Tuple* ms_t, TxnContext& ctx){
  Tuple_reachable *t0 = static_cast<Tuple_reachable*>(ms_t);
  gc_string const& X = boost::get<0>(*t0);
  gc_string const& Y = boost::get<1>(*t0);
    std::cout << "[del]" << _ms_str_const_0<<' '<<X<<' '<<Y << std::endl;
}

//# 16 "reachable.mos"
void recv_ms_periodic1_handler_0(Tuple* ms_t, TxnContext& ctx){
  Tuple_ms_periodic1 *t0 = static_cast<Tuple_ms_periodic1*>(ms_t);
  WR_TABLE_IN_WSP(link)->erase( new Tuple_link(_ms_str_const_1, _ms_str_const_2));
  if (boost::get<0>(*t0) >= 1) return;
  boost::get<0>(*t0)++ ;
  Event _e(Event::RECV, t0);
  callLater(1, boost::bind(demux, _e), boost::get<1>(*t0));
}

//# 17 "reachable.mos"
void recv_ms_periodic2_handler_0(Tuple* ms_t, TxnContext& ctx){
  Tuple_ms_periodic2 *t0 = static_cast<Tuple_ms_periodic2*>(ms_t);
  RD_TABLE_IN_WSP(reachable)->dump();
  RD_TABLE_IN_WSP(link)->dump();
  if (boost::get<0>(*t0) >= 1) return;
  boost::get<0>(*t0)++ ;
  Event _e(Event::RECV, t0);
  callLater(1, boost::bind(demux, _e), boost::get<1>(*t0));
}

//# 20 "reachable.mos"
void recv_ms_periodic3_handler_0(Tuple* ms_t, TxnContext& ctx){
  Tuple_ms_periodic3 *t0 = static_cast<Tuple_ms_periodic3*>(ms_t);
  WR_TABLE_IN_WSP(link)->erase( new Tuple_link(_ms_str_const_3, _ms_str_const_3));
  if (boost::get<0>(*t0) >= 1) return;
  boost::get<0>(*t0)++ ;
  Event _e(Event::RECV, t0);
  callLater(1, boost::bind(demux, _e), boost::get<1>(*t0));
}

//# 21 "reachable.mos"
void recv_ms_periodic4_handler_0(Tuple* ms_t, TxnContext& ctx){
  Tuple_ms_periodic4 *t0 = static_cast<Tuple_ms_periodic4*>(ms_t);
  RD_TABLE_IN_WSP(reachable)->dump();
  RD_TABLE_IN_WSP(link)->dump();
  if (boost::get<0>(*t0) >= 1) return;
  boost::get<0>(*t0)++ ;
  Event _e(Event::RECV, t0);
  callLater(1, boost::bind(demux, _e), boost::get<1>(*t0));
}

void view_handler(Event e, TxnContext & ctx){
  if (false) {}
  else if ( (e.type & Event::DELETED) && (e.t->type_id==ID_Tuple_link) ){
    del_link_handler_0(e.t, ctx);
    del_link_handler_1(e.t, ctx);
  }
  else if ( (e.type & Event::INSERTED) && (e.t->type_id==ID_Tuple_reachable) ){
    ins_reachable_handler_0(e.t, ctx);
    ins_reachable_handler_1(e.t, ctx);
  }
  else if ( (e.type & Event::DELETED) && (e.t->type_id==ID_Tuple_reachable) ){
    del_reachable_handler_0(e.t, ctx);
    del_reachable_handler_1(e.t, ctx);
  }
  else if ( (e.type & Event::INSERTED) && (e.t->type_id==ID_Tuple_link) ){
    ins_link_handler_0(e.t, ctx);
    ins_link_handler_1(e.t, ctx);
  }
}

void event_handler(Event e, TxnContext & ctx) {
  if (false) {}
  else if ( (e.type & Event::RECV) && (e.t->type_id==ID_Tuple_ms_periodic4) ){
    recv_ms_periodic4_handler_0(e.t, ctx);
  }
  else if ( (e.type & Event::RECV) && (e.t->type_id==ID_Tuple_ms_periodic1) ){
    recv_ms_periodic1_handler_0(e.t, ctx);
  }
  else if ( (e.type & Event::RECV) && (e.t->type_id==ID_Tuple_ms_periodic3) ){
    recv_ms_periodic3_handler_0(e.t, ctx);
  }
  else if ( (e.type & Event::RECV) && (e.t->type_id==ID_Tuple_ms_periodic0) ){
    recv_ms_periodic0_handler_0(e.t, ctx);
  }
  else if ( (e.type & Event::RECV) && (e.t->type_id==ID_Tuple_ms_periodic2) ){
    recv_ms_periodic2_handler_0(e.t, ctx);
  }
}

void trigger_handler(Event e, TxnContext & ctx) {
  if (false) {}
  else if ( (e.type & Event::INSERTED) && (e.t->type_id==ID_Tuple_reachable) ){
    std::cout << "trigger on insert reachable\n";
  }
}

void facts(TxnContext& ctx){
  WR_TABLE_IN_WSP(link)->insert( new Tuple_link( gc_string("A"), gc_string("B") ), &ctx.deltaQ );
  WR_TABLE_IN_WSP(link)->insert( new Tuple_link( gc_string("B"), gc_string("C") ), &ctx.deltaQ );
  WR_TABLE_IN_WSP(link)->insert( new Tuple_link( gc_string("C"), gc_string("A") ), &ctx.deltaQ );
  WR_TABLE_IN_WSP(link)->insert( new Tuple_link( gc_string("C"), gc_string("B") ), &ctx.deltaQ );
  WR_TABLE_IN_WSP(link)->insert( new Tuple_link( gc_string("A"), gc_string("A") ), &ctx.deltaQ );
}
void csv_facts(const char* fn, TxnContext& ctx){
  CSVLoader l; if (l.open(fn)) {
    while (!l.eof()) {
      string_vector tokens = l.next();
      if ( tokens.size()<2 ) continue;
      if (tokens.at(0)=="link"){
        WR_TABLE_IN_WSP(link)->insert( new Tuple_link( tokens.at(1), tokens.at(2) ), &ctx.deltaQ );
      }
    }
  } else std::cerr << "Cannot open CSV file " << fn << std::endl;
}
void register_periodics(){
  {
    static boost::asio::deadline_timer tm(iosv);
    static Tuple_ms_periodic0 t0(1, &tm);
    Event e(Event::RECV, &t0);
    callLater(0, boost::bind(demux, e),&tm);
  }
  {
    static boost::asio::deadline_timer tm(iosv);
    static Tuple_ms_periodic1 t1(1, &tm);
    Event e(Event::RECV, &t1);
    callLater(1, boost::bind(demux, e),&tm);
  }
  {
    static boost::asio::deadline_timer tm(iosv);
    static Tuple_ms_periodic2 t2(1, &tm);
    Event e(Event::RECV, &t2);
    callLater(2, boost::bind(demux, e),&tm);
  }
  {
    static boost::asio::deadline_timer tm(iosv);
    static Tuple_ms_periodic3 t3(1, &tm);
    Event e(Event::RECV, &t3);
    callLater(3, boost::bind(demux, e),&tm);
  }
  {
    static boost::asio::deadline_timer tm(iosv);
    static Tuple_ms_periodic4 t4(1, &tm);
    Event e(Event::RECV, &t4);
    callLater(4, boost::bind(demux, e),&tm);
  }
}

void registerAllTuples(){
}

void initWorkspace(WorkspacePtr wsp) {
  wsp->addTable( ID_Tuple_reachable, new Table_reachable(&taskQ, true) );
  wsp->addTable( ID_Tuple_link, new Table_link(&taskQ, true) );
}
