/* Licence: https://github.com/eos-referendum/master/LICENCE.md */

/*
 * File:   referendum.hpp
 * Author: Michael Fletcher | EOS42
 *
 * Created on 16 May 2018, 17:26
 */

#include <eosiolib/eosio.hpp>

namespace referendum {

class referendum : public eosio::contract {
public:
    referendum(account_name publisher):contract(publisher){}

    void vote(account_name voter, bool vote_yes);
    void unvote(account_name voter);
    
private:

};

EOSIO_ABI(referendum, (vote)(unvote))

}
