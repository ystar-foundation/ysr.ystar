#include <ystar.token.hpp>

void ystartoken::create( const name& issuer, const name& ruler, const name& bigsetter, const name& locker,
                         const name& freezer, const name& unfreezer, uint32_t symno, const asset&  maximum_supply )
{
   require_auth( get_self() );
   //require_auth( issuer );

   auto sym = maximum_supply.symbol;
   check( sym.is_valid(), "invalid symbol name" );
   check( maximum_supply.is_valid(), "invalid supply");
   check( maximum_supply.amount > 0, "max-supply must be positive");

   stats statstable( get_self(), sym.code().raw() );
   auto existing = statstable.find( sym.code().raw() );
   check( existing == statstable.end(), "token with symbol already exists" );
   //check( symno < (2 ^ 30), "the value of symno is too big" );
   assetkinds symkind( get_self(), get_self().value );
   auto existno = symkind.find( symno );
   check( existno == symkind.end(), "this symno has already existed" );

   statstable.emplace(get_self(), [&]( auto& s ) {
      s.supply.symbol = maximum_supply.symbol;
      s.max_supply    = maximum_supply;
      s.issuer        = issuer;
      s.ruler         = ruler;
      s.bigsetter     = bigsetter;
      s.locker        = locker;
      s.freezer       = freezer;
      s.unfreezer     = unfreezer;
      s.symno         = symno;
   });
   symkind.emplace(get_self(), [&]( auto& row ) {
      row.symno = symno;
   });
}

void ystartoken::issue( const name& to, const asset& quantity, const string& memo )
{
   auto sym = quantity.symbol;
   check( sym.is_valid(), "invalid symbol" );
   check( memo.size() <= 256, "memo has more than 256 bytes" );

   stats statstable( get_self(), sym.code().raw() );
   auto existing = statstable.find( sym.code().raw() );
   check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
   const auto& st = *existing;
   //check( to == st.issuer, "tokens can only be issued to issuer account" );

   require_auth( st.issuer );
   check( quantity.is_valid(), "invalid quantity" );
   check( quantity.amount > 0, "must issue positive quantity" );
   check( quantity.symbol == st.supply.symbol, "symbol or precision mismatch" );
   check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

   statstable.modify( st, same_payer, [&]( auto& s ) {
      s.supply += quantity;
   });

   add_balance( to.value, sym.code().raw(), quantity, st.issuer, true );
}

void ystartoken::setextime( uint64_t time, const asset& value )
{
   auto sym = value.symbol;
   check( sym.is_valid(), "invalid symbol" );
   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "This token is not existed when setextime." );

   check( st.time == 0, "The exchanging time has already been setted." );
   require_auth( st.issuer );

   statstable.modify( st, same_payer, [&]( auto& s ) {
      s.time = time;
   });
}

void ystartoken::open( const name& owner, const asset& value, const name& ram_payer )
{
   require_auth( ram_payer );
   check( is_account( owner ), "The account does not exist");
   accounts new_acnts( get_self(), owner.value );
   auto sym = value.symbol;
   check( sym.is_valid(), "invalid symbol when newacc" );
   auto newacc = new_acnts.find( sym.code().raw() );
   check( newacc == new_acnts.end(), "This token has already been created." );
   asset accasset( 0, sym );
   new_acnts.emplace( ram_payer, [&]( auto& a ){
      a.balance = accasset;
   });
}

void ystartoken::close( const name& acc, const asset& value )
{
   require_auth( acc );
   check( is_account( acc ), "The account does not exist");
   accounts del_acnts( get_self(), acc.value );
   auto sym = value.symbol;
   check( sym.is_valid(), "invalid symbol when delacc" );
   auto delacc = del_acnts.find( sym.code().raw() );
   check( delacc != del_acnts.end(), "This token doesn't exist." );
   check( delacc->balance.amount == 0, "The balance is not zero");
   del_acnts.erase( delacc );
}

void ystartoken::transfer( const name&    from,
                         const name&    to,
                         const asset&   quantity,
                         bool bcreate,
                         const string&  memo )
{
    check( from != to, "cannot transfer to self" );
    require_auth( from );
    check( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol;
    check( sym.is_valid(), "invalid symbol when transfer" );
    stats statstable( get_self(), sym.code().raw() );
    const auto& st = statstable.get( sym.code().raw(), "token is not existed." );

    require_recipient( from );
    require_recipient( to );

    check( quantity.is_valid(), "invalid quantity" );
    check( quantity.amount > 0, "must transfer positive quantity" );
    check( quantity.symbol == st.supply.symbol, "symbol or precision mismatch" );
    check( memo.size() <= 256, "memo has more than 256 bytes" );

    sub_balance( from, quantity );
    add_balance( to.value, sym.code().raw(), quantity, from, bcreate );
}

void ystartoken::sub_balance( const name& owner, const asset& value ) {
   accounts from_acnts( get_self(), owner.value );

   const auto& from = from_acnts.get( value.symbol.code().raw(), "Payer's token is not existed" );
   //check( from.balance.amount >= value.amount, "overdrawn balance" );
   check( !is_frozen(owner, value), "Payer's account is frozen." );

   auto lock_asset = get_lock_asset(owner, value);
   check( lock_asset.symbol == value.symbol, "symbol or precision mismatch" );
   check( from.balance.amount - lock_asset.amount >= value.amount, "overdrawn balance" );

   from_acnts.modify( from, owner, [&]( auto& a ) {
      a.balance -= value;
   });
}

void ystartoken::add_balance( uint64_t namevalue, uint64_t symbol, const asset& value, const name& ram_payer, bool bcreate )
{
   accounts to_acnts( get_self(), namevalue );
   auto to = to_acnts.find( symbol );
   if( to != to_acnts.end() ){
      to_acnts.modify( to, same_payer, [&]( auto& a ) {
         a.balance.amount += value.amount;
      });
   } else if( bcreate ){
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      check( false, "Payee's token is not existed" );
   }
}

void ystartoken::freezeacc( const name& acc, const asset& value, uint64_t time )
{
   auto sym = value.symbol;
   check( sym.is_valid(), "invalid symbol when freezeacc" );
   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "token is not existed when freezeacc" );
   require_auth( st.freezer );
   check( is_account( acc ), "Account does not exist when freezeacc");
   accounts _acnts( get_self(), acc.value );
   const auto& to = _acnts.get( sym.code().raw(),  "Account does not have this token when freezeacc");

   accfrozens _accfrozen( get_self(), sym.code().raw() );
   auto it = _accfrozen.find( acc.value );
   if( it == _accfrozen.end() ) {
      _accfrozen.emplace(st.freezer, [&](auto &row) {
         row.user = acc;
         row.time = time;
      });
   } else {
      _accfrozen.modify(it, st.freezer, [&](auto &row) {
         row.time = time;
      });
   }
}

void ystartoken::unfreezeacc( const name& acc, const asset& value )
{
   auto sym = value.symbol;
   check( sym.is_valid(), "invalid symbol when unfreezeacc" );
   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "token is not existed when unfreezeacc" );
   require_auth( st.unfreezer );
   check( is_account( acc ), "Account does not exist when unfreezeacc");
   accounts _acnts( get_self(), acc.value );
   const auto& to = _acnts.get( sym.code().raw(), "Account does not have this token when unfreezeacc" );

   accfrozens _accfrozen( get_self(), sym.code().raw() );
   const auto& it = _accfrozen.get( acc.value, "about this token, account isn't frozen" );
   _accfrozen.erase( it );
}

void ystartoken::addaccbig( const name& user, const asset& value ) {
   auto sym = value.symbol;
   check( sym.is_valid(), "invalid symbol when addaccbig" );
   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "token is not existed when addaccbig" );
   require_auth( st.bigsetter );

   check( is_account( user ), "Account does not exist when addaccbig");
   accbigs _accbig( get_self(), sym.code().raw() );
   auto bigacc = _accbig.find( user.value );
   check( bigacc == _accbig.end(), "user has already registered as big account" );  

   _accbig.emplace(st.bigsetter, [&](auto &row) {
      row.user = user;
   });
}

void ystartoken::rmvaccbig( const name& user, const asset& value ) {
   auto sym = value.symbol;
   check( sym.is_valid(), "invalid symbol when rmvaccbig" );
   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "token is not existed when rmvaccbig" );
   require_auth( st.bigsetter );

   check( is_account( user ), "Account does not exist when rmvaccbig");
   accbigs _accbig( get_self(), sym.code().raw() );
   const auto& bigacc = _accbig.get( user.value,  "is not a big account");
   _accbig.erase(bigacc);
}

void ystartoken::addrule( uint32_t lockruleid, const std::vector<uint64_t>& times, const std::vector<uint16_t>& pcts,
                          uint32_t base, uint32_t period, const asset& value, const string& desc ) 
{
   auto sym = value.symbol;
   check( sym.is_valid(), "invalid symbol when addrule" );
   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "token is not existed when addrule" );
   require_auth( st.ruler );
   //check( st.time > 0, "must exchange before addrule" ); 

   //check( times.size() >= 2, "invalidate size of times array" );
   check( times.size() >= 1, "invalidate size of times array" ); //when equals one, lock by period
   check( times.size() == pcts.size(), "times and percentage in different size." );
   check( desc.size() <= 256, "desc has more than 256 bytes" );
   //check( lockruleid < 1000000000, "lockruleid is too big" );
   if ( times.size() == 1 ) {
      check( period > 0, "period must be a positive number" );
   }

   lockrules _lockrule( get_self(), sym.code().raw() );
   auto itrule = _lockrule.find(lockruleid);
   check( itrule == _lockrule.end(), "the id already existed in rule table" ); 

   for( size_t i = 0; i < times.size(); i++ ) {
      if( i == 0 ){
         //check( times[i] >= st.time, "exchanging time should be earlier" );
         check( pcts[i] >= 0 && pcts[i] <= base, "invalidate lock percentage" );
      } else {
         check( times[i] > times[i-1], "times vector error" );
         check( pcts[i] > pcts[i-1] && pcts[i] <= base, "lock percentage vector error" );
      }
   }

   _lockrule.emplace(st.ruler, [&](auto &row) {
      row.lockruleid   = lockruleid;
      row.times        = times;
      row.pcts         = pcts;
      row.base         = base;
      row.period       = period;
      row.desc         = desc;
   });
}

void ystartoken::batchtrans(const name& from, const std::vector<name>& accs,
                            const std::vector<int64_t>& amounts, const asset& value, const string& memo)
{
   require_auth( from );

   check( memo.size() <= 256, "memo has more than 256 bytes" );
   check( accs.size() == amounts.size(), "accounts and quantities in different size" );
   auto sym = value.symbol;
   check( sym.is_valid(), "invalid symbol when batchtrans" );
   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "token is not existed when batchtrans." );
   check( sym == st.supply.symbol, "symbol or precision mismatch" );

   int64_t all_amount = 0;
   for(size_t no = 0; no < amounts.size(); no++) {
      //check( from != accs[no], "cannot transfer to self" );
      //check( amounts[no] > 0, "must transfer positive quantity" );
      //check( is_account( accs[no] ), "is not an account");
      accounts to_acnts( get_self(), accs[no].value );
      auto to = to_acnts.find( sym.code().raw() );
      if( to != to_acnts.end() && amounts[no] > 0 && is_account( accs[no]) ) {
         to_acnts.modify( to, same_payer, [&]( auto& a ) {
            a.balance.amount += amounts[no];
         });
         all_amount += amounts[no];
      }
   }
   asset subasset( all_amount, sym );
   sub_balance( from, subasset );
}

void ystartoken::locktransfer(uint32_t lockruleid, const name& from, const name& to, const asset& quantity, const string& memo) 
{
   require_auth( from );
   auto sym = quantity.symbol;
   check( sym.is_valid(), "invalid symbol when locktransfer" );
   check( quantity.amount > 0, "must locktransfer positive quantity" );
   check( memo.size() <= 256, "memo has more than 256 bytes" );
   accbigs _accbig( get_self(), sym.code().raw() );
   const auto& bigacc = _accbig.get( from.value, "only bigacc can locktransfer" );
   lockrules _lockrule( get_self(), sym.code().raw() );
   const auto& itrule = _lockrule.get( lockruleid, "lockruleid not existed in rule table" );

   transfer( from, to, quantity, true, memo );

   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "token is not existed" );
   uint64_t symruleid = lockruleid + ((uint64_t)st.symno << 32);
   //acclocks _acclock( get_self(), to.value + ((uint64_t)(sym.code().raw() & 0xf) << 60) );
   acclocks _acclock( get_self(), to.value + (uint64_t)(sym.code().raw() & 0xf) );
   size_t rules_no = 0;
   for(auto it = _acclock.begin(); it != _acclock.end(); it++) {
      rules_no++;
      if (it->symruleid == symruleid) {
         _acclock.modify(it, same_payer, [&](auto &row) {
            row.time = current_time_point().sec_since_epoch();
            row.quantity += quantity.amount;
         });
         return;
      }
   }
   check( rules_no <= 30, "lock rules of account is too many" );
   auto itlc = _acclock.find(symruleid);
   if(itlc == _acclock.end()) {
      _acclock.emplace(from, [&](auto &row) {
         row.symruleid   = symruleid;
         row.quantity    = quantity.amount;
         //row.lockruleid  = lockruleid;
         row.user        = to;
         row.from        = from;
         row.time        = current_time_point().sec_since_epoch();
      });
   }
}

void ystartoken::lockasset( const name& acc, const asset& value, const string& memo )
{
   auto sym = value.symbol;
   check( sym.is_valid(), "invalid symbol when lockasset" );
   check( memo.size() <= 256, "memo has more than 256 bytes" );
   check( value.amount >= 0, "cannot lock negative quantity" );
   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "token is not existed when lockasset" );
   require_auth( st.locker );

   accounts _acnts( get_self(), acc.value );
   auto to = _acnts.get( sym.code().raw(),  "Account does not have this token");

   numlocks _numlock( get_self(), sym.code().raw() );
   auto it = _numlock.find( acc.value );
   if( it == _numlock.end() ) {
      check( to.balance.amount - get_lock_asset(acc, value).amount >= value.amount, "lock overdrawn asset" );
      if ( value.amount > 0 ) {
         _numlock.emplace(st.locker, [&](auto &row) {
            row.user = acc;
            row.quantity = value;
         });
      }
   } else {
      check( it->quantity.amount > value.amount, "locking asset should less than before" );
      if ( value.amount == 0 ) {
         _numlock.erase( it );
      } else {
         _numlock.modify(it, st.locker, [&](auto &row) {
            row.quantity = value;
         });
      }
   }
}

bool ystartoken::is_frozen( const name& user, const asset& value )
{
   bool isFrozen = false;
   accfrozens _accfrozen( get_self(), value.symbol.code().raw() );
   auto it = _accfrozen.find( user.value );
   if(it != _accfrozen.end()) {
      uint64_t current_time = current_time_point().sec_since_epoch(); //seconds
      //print( " test time is ", current_time, " time!!! ", "\n" );
      if(current_time < it->time) {
         isFrozen = true;
      } else {
         _accfrozen.erase( it );
      }
   }
   return isFrozen;
}

asset ystartoken::get_lock_asset( const name& user, const asset& value ) {
   auto sym = value.symbol;
   asset lockasset( 0, sym );

   numlocks _numlock( get_self(), sym.code().raw() );
   auto il = _numlock.find( user.value );
   if( il != _numlock.end() )
      lockasset.amount = il->quantity.amount;

   stats statstable( get_self(), sym.code().raw() );
   const auto& st = statstable.get( sym.code().raw(), "token is not existed" );
   check( sym == st.supply.symbol, "symbol or precision mismatch" );
   acclocks _acclock( get_self(), user.value  + (uint64_t)(sym.code().raw() & 0xf) );
   uint64_t curtime = current_time_point().sec_since_epoch(); //seconds
   for(auto it = _acclock.begin(); it != _acclock.end(); it++) {
      int64_t amount = it->quantity;
      uint64_t extime = st.time; //exchanging time

      if ( (it->symruleid >> 32) != st.symno)
         continue;

      lockrules _lockrule( get_self(), sym.code().raw() );
      auto itrule = _lockrule.find(it->symruleid & 0xffffffff);
      //check(itrule != _lockrule.end(), "lockruleid not existed in rule table");
      if ( extime == 0 || itrule == _lockrule.end() || curtime <= extime) {
         lockasset.amount += amount;
      } else {
         uint32_t percent = 0;
         if ( itrule->times.size() == 1 ) { //lock by period
            //int64_t hour_seconds = 60*60;
            int64_t numerator = (int64_t)curtime - (int64_t)extime - (int64_t)itrule->times[0];
            int64_t periods = numerator / (int64_t)itrule->period;
            if (numerator > 0 && periods >= 1) {
               percent = itrule->pcts[0] * periods;
               if (percent < itrule->base) {
                  percent = itrule->base - percent;
                  lockasset.amount += (int64_t)(((double)amount / itrule->base)*percent);
               }
            } else {
               lockasset.amount += amount;
            }
         } else {
            size_t n = 0;
            for(auto itt = itrule->times.begin(); itt != itrule->times.end(); itt++) {
               if( extime + *itt > curtime ) {
                     break;
               }
               percent = itrule->pcts[n];
               n++;
            }
            //check( percent>=0 && percent<=itrule->base, "invalidate lock percentage" );
            percent = itrule->base - percent;
            lockasset.amount += (int64_t)(((double)amount / itrule->base)*percent);
         }
      }
   }
   //print(lockasset.amount);

   return lockasset;
}
