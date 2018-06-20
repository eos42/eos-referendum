/* Licence: https://github.com/eos-referendum/master/LICENCE.md */

/*
 * File:   referendum.hpp
 * Author: Michael Fletcher | EOS42
 *
 * Created on 20 June 2018, 17:26
 */

#include <eosiolib/eosio.hpp>
#include <eosio.system/eosio.system.hpp>
#include "referendum_declarations.hpp"

namespace referendum {


struct extended_voter_info : public eosiosystem::voter_info {

  double existing_vote_quantity = 0;
  uint8_t existing_vote_side = NULL_VOTE; 

  EOSLIB_SERIALIZE_DERIVED(extended_voter_info, eosiosystem::voter_info, (existing_vote_quantity)(existing_vote_side))
};

typedef eosio::multi_index<N(voters), extended_voter_info> extended_voter_table;


struct result {

    int64_t id;
    double  total_votes = 0;

    uint64_t primary_key() const {
        return id;
    }

    double by_votes() const {
        return total_votes;
    }

    EOSLIB_SERIALIZE(result, (id)(total_votes));
};

typedef eosio::multi_index< N(results), result,
        eosio::indexed_by<N(totalvotes), eosio::const_mem_fun<result, double, &result::by_votes>  >
        >  results_table;


class referendum : public eosio::contract {

public:
    referendum(account_name self):contract(self), voters(self, self), results(self, self) {}
  
    void init(account_name self);
    void vote(account_name voter, uint8_t vote_side);
    void unvote(account_name voter);

private:
    extended_voter_table  voters;
    results_table	   results;

    bool validate_side(uint8_t vote_side);
};

EOSIO_ABI(referendum, (vote)(unvote))

}
