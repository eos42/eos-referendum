/* Licence: https://github.com/eos-referendum/master/LICENCE.md */

/*
 * File:   referendum.hpp
 * Author: Michael Fletcher | EOS42
 *
 * Created on 20 June 2018, 17:26
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosio.system/eosio.system.hpp>
#include "referendum_declarations.hpp"

namespace referendum {


/*TODO Change scope to point to system voters table? */
typedef eosio::multi_index<N(voters), eosiosystem::voter_info> voter_info_table;


struct referendum_info{
   uint64_t total_days = 0; // total days passed
   uint64_t total_c_days = 0; // total consecutive days vote has passed
   bool	    vote_active = true; // false when the vote has finished

   EOSLIB_SERIALIZE(referendum_info, (total_days)(total_c_days))
};
typedef eosio::singleton<N(referendum), referendum_info> referendum_results_table;

 
struct registered_voters {

    account_name name;
    
    uint64_t primary_key() const {
        return name;
    }

    EOSLIB_SERIALIZE(registered_voters, (name));
};
typedef eosio::multi_index<N(refvoters), registered_voters>  registered_voters_table;


class referendum : public eosio::contract {
public:
    referendum(account_name self):contract(self), registered_voters(self, self), voter_info(self, self), referendum_results(self, self) {}
  
    void vote(account_name voter, uint8_t vote_side);
    void unvote(account_name voter);
    void countvotes(account_name publisher);

private:
    registered_voters_table    registered_voters;
    voter_info_table	       voter_info;
    referendum_results_table   referendum_results;
 
    bool validate_side(uint8_t vote_side);
};

EOSIO_ABI(referendum, (vote)(unvote))

}
