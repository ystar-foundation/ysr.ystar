#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>

#include <string>
using namespace eosio;

using std::string;
//using eosio::current_time_point;

#define YOTTA_MAX_TOKENS   (2 ^ 20)
#define YOTTA_MAX_RULES    (2 ^ 40)

/**
 * ystar.token contract defines the structures and actions that allow users to create, issue, and manage
 * tokens on eosio based blockchains.
 */
class [[eosio::contract("ystar.token")]] ystartoken : public contract {
   public:
      using contract::contract;

      //static constexpr symbol token_symbol = symbol(symbol_code(TOKEN_SYMBOL), 4);

      /**
       * Allows `issuer` account to create a token in supply of `maximum_supply`. If validation is successful a new entry in statstable for token symbol scope gets created.
       *
       * @param issuer - the account that creates the token,
       * @param ruler - the account that add the lock rule,
       * @param bigsetter - the account that manager the bigaccs,
       * @param locker - the account that lock asset,
       * @param freezer - the account that freeze asset,
       * @param unfreezer - the account that unfreeze asset,
       * @param symno - symbol number of this token
       * @param maximum_supply - the maximum supply set for the token created.
       *
       * @pre Token symbol has to be valid,
       * @pre Token symbol must not be already created,
       * @pre maximum_supply has to be smaller than the maximum supply allowed by the system: 1^62 - 1.
       * @pre Maximum supply must be positive;
       */
      [[eosio::action]]
      void create( const name&   issuer,
                   const name&   ruler,
                   const name&   bigsetter,
                   const name&   locker,
                   const name&   freezer,
                   const name&   unfreezer,
                   uint32_t      symno,
                   const asset&  maximum_supply );

      /**
       *  This action issues to `to` account a `quantity` of tokens.
       *
       * @param to - the account to issue tokens to, it must be the same as the issuer,
       * @param quntity - the amount of tokens to be issued,
       * @param memo - the memo string that accompanies the token issue transaction.
       */
      [[eosio::action]]
      void issue( const name& to, const asset& quantity, const string& memo );

      /**
       *  This action set exchanging time.
       *
       * @param time - the exchanging time,
       * @param value - in order to get the symbol of currency.
       */
      [[eosio::action]]
      void setextime( uint64_t time, const asset& value );
      
      /**
       * Create an account.
       *
       * @param owner - which account will be created,
       * @param value - in order to get the symbol of currency,
       * @param ram_payer - which account create.
       */
      [[eosio::action]]
      void open( const name&    owner,
                 const asset&   value,
                 const name&    ram_payer );
      
      /**
       * Delete an account.
       *
       * @param acc - transfer from which account,
       * @param value - in order to get the symbol of currency.
       */
      [[eosio::action]]
      void close( const name&    acc,
                  const asset&   value );
      
      /**
       * Allows `from` account to transfer to `to` account the `quantity` tokens.
       * One account is debited and the other is credited with quantity tokens.
       *
       * @param from - transfer from which account,
       * @param to - transfer to which account,
       * @param quantity - the quantity of tokens to be transferred,
       * @param bcreate - create acc or not,
       * @param memo - the memo string to accompany the transaction.
       */
      [[eosio::action]]
      void transfer( const name&    from,
                     const name&    to,
                     const asset&   quantity,
                     bool bcreate,
                     const string&  memo );

      /**
       * This action will freeze an account.
       *
       * @param acc - the account to be frozen,
       * @param value - in order to get the symbol of currency,
       * @param time - freeze until this time.
       */
      [[eosio::action]]
      void freezeacc( const name&    acc,
                      const asset&   value,
                      uint64_t       time );

      /**
       * This action will unfreeze an account.
       *
       * @param acc - the account to be unfrozen,
       * @param value - in order to get the symbol of currency.
       */
      [[eosio::action]]
      void unfreezeacc( const name&    acc,
                        const asset&   value );

      /**
       * This action will add acc to accbig.
       *
       * @param user - which account,
       * @param value - which asset.
       */
      [[eosio::action]]
      void addaccbig( const name&   user,
                      const asset&  value );

      /**
       * This action will remove acc from accbig.
       *
       * @param user - which account,
       * @param value - which asset.
       */
      [[eosio::action]]
      void rmvaccbig( const name&  user,
                      const asset& value );

      /**
       * This action will add rule for lock.
       *
       * @param lockruleid - id of the lock rule,
       * @param times - which account,
       * @param pcts - percentage's numerator,
       * @param base - percentage's denominator,
       * @param period - unlock period,
       * @param value - in order to get the ruler,
       * @param desc - the desc.
       */
      [[eosio::action]]
      void addrule(  uint32_t lockruleid,
                     const std::vector<uint64_t>& times,
                     const std::vector<uint16_t>& pcts,
                     uint32_t base,
                     uint32_t period,
                     const asset& value,
                     const string& desc );

      /**
       * This action will transfer a batch of asset.
       *
       * @param from - transfer from which account,
       * @param accs - transfer to which accounts,
       * @param amounts - transfer how many to every account,
       * @param value - in order to get the symbol,
       * @param memo - the memo.
       */
      [[eosio::action]]
      void batchtrans( const name&   from,
                       const std::vector<name>& accs,
                       const std::vector<int64_t>& amounts,
                       const asset& value,
                       const string& memo );

      /**
       * This action will transfer the locked asset.
       *
       * @param lockruleid - which lock rule,
       * @param from - transfer from which account,
       * @param to - transfer to which account,
       * @param quantity - quantity,
       * @param memo - the memo.
       */
      [[eosio::action]]
      void locktransfer( uint32_t      lockruleid,
                         const name&   from,
                         const name&   to,
                         const asset&  quantity,
                         const string& memo );

      /**
       * This action will lock the asset of an account.
       *
       * @param acc - the account to be locked,
       * @param value - the asset of the account,
       * @param memo - the memo.
       */
      [[eosio::action]]
      void lockasset( const name&    acc,
                      const asset&   value,
                      const string&  memo );
      
      static asset get_supply( const name& token_contract_account, const symbol_code& sym_code )
      {
         stats statstable( token_contract_account, sym_code.raw() );
         const auto& st = statstable.get( sym_code.raw() );
         return st.supply;
      }

      static asset get_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
      {
         accounts accountstable( token_contract_account, owner.value );
         const auto& ac = accountstable.get( sym_code.raw() );
         return ac.balance;
      }

      using create_action = eosio::action_wrapper<"create"_n, &ystartoken::create>;
      using issue_action = eosio::action_wrapper<"issue"_n, &ystartoken::issue>;
      using setextime_action = eosio::action_wrapper<"setextime"_n, &ystartoken::setextime>;
      using open_action = eosio::action_wrapper<"open"_n, &ystartoken::open>;
      using close_action = eosio::action_wrapper<"close"_n, &ystartoken::close>;
      using transfer_action = eosio::action_wrapper<"transfer"_n, &ystartoken::transfer>;
      using freezeacc_action = eosio::action_wrapper<"freezeacc"_n, &ystartoken::freezeacc>;
      using unfreezeacc_action = eosio::action_wrapper<"unfreezeacc"_n, &ystartoken::unfreezeacc>;
      using addaccbig_action = eosio::action_wrapper<"addaccbig"_n, &ystartoken::addaccbig>;
      using rmvaccbig_action = eosio::action_wrapper<"rmvaccbig"_n, &ystartoken::rmvaccbig>;
      using addrule_action = eosio::action_wrapper<"addrule"_n, &ystartoken::addrule>;
      using batchtrans_action = eosio::action_wrapper<"batchtrans"_n, &ystartoken::batchtrans>;
      using locktransfer_action = eosio::action_wrapper<"locktransfer"_n, &ystartoken::locktransfer>;
      using lockasset_action = eosio::action_wrapper<"lockasset"_n, &ystartoken::lockasset>;

   private:
      struct [[eosio::table]] account {
         asset    balance;

         uint64_t primary_key()const { return balance.symbol.code().raw(); }
      };

      struct [[eosio::table]] accfrozen {
         name     user;
         uint64_t time; //frozen deadline

         uint64_t primary_key()const { return user.value; }
      };

      struct [[eosio::table]] accbig {
         name     user;

         uint64_t primary_key()const { return user.value; }
      };

      struct [[eosio::table]] currency_stat {
         asset    supply;
         asset    max_supply;
         name     issuer;
         name     ruler;
         name     bigsetter;
         name     locker;
         name     freezer;
         name     unfreezer;
         uint64_t time = 0; //exchanging time
         uint32_t symno; //symbol number in this contract

         uint64_t primary_key()const { return supply.symbol.code().raw(); }
      };

      struct [[eosio::table]] assetkind {
         uint32_t symno; //symbol number in this contract

         uint32_t primary_key()const { return symno; }
      };

      struct [[eosio::table]] lockrule {
         uint32_t                lockruleid;
         std::vector<uint64_t>   times;
         std::vector<uint16_t>   pcts; //lock percentage's numerator
         uint32_t                base; //lock percentage's denominator
         uint32_t                period;
         string                  desc;

         uint32_t                primary_key()const { return lockruleid; }
      };

      struct [[eosio::table]] acclock {
         uint64_t        symruleid;
         //uint32_t        lockruleid;  
         int64_t         quantity;
         name            user;
         name            from;
         uint64_t        time;

         uint32_t        primary_key()const { return symruleid; }
      };

      struct [[eosio::table]] numlock {
         name            user;
         asset           quantity;

         uint64_t        primary_key()const { return user.value; }
      };

      typedef eosio::multi_index< "accounts"_n, account > accounts;
      typedef eosio::multi_index< "accfrozen"_n, accfrozen> accfrozens;
      typedef eosio::multi_index< "accbig"_n, accbig> accbigs;
      typedef eosio::multi_index< "stat"_n, currency_stat > stats;
      typedef eosio::multi_index< "assetkind"_n, assetkind > assetkinds;
      typedef eosio::multi_index< "lockrule"_n, lockrule> lockrules;
      typedef eosio::multi_index< "acclock"_n, acclock> acclocks;
      typedef eosio::multi_index< "numlock"_n, numlock> numlocks;

      void sub_balance( const name& owner, const asset& value );
      void add_balance( uint64_t namevalue, uint64_t symbol, const asset& value, const name& ram_payer, bool bcreate );
      bool is_frozen( const name&  user, const asset& value );
      asset get_lock_asset( const name& user, const asset& value );
};
