/* Licence: https://github.com/eos-referendum/master/LICENCE.md */

/*
 * File:   referendum.cpp
 * Author: Michael Fletcher | EOS42
 *
 * Created on 20 June 2018, 17:26
*/

#include "referendum.hpp"

namespace referendum {

void referendum::vote(account_name voter_name, uint8_t vote_side){
  require_auth(voter_name);

  /* TODO Check if vote is active */

  /* if they've staked, their will be a voter entry */
  auto voter = voter_info.find(voter_name);
  eosio_assert(voter == voter_info.end(), "user must stake before they can vote.");

  /* vote side yes = 1 or no = 0 */
  eosio_assert(validate_side(vote_side), "vote side is invalid");

  /* have they already voted */
  auto registered_voter = registered_voters.find(voter_name);
  eosio_assert(registered_voter == registered_voters.end(), "user has already voted"); 

  /* register vote */
  registered_voters.emplace(_self, [&](auto &voter_rec){
    voter_rec.name = voter_name;
  });
   
}

void referendum::unvote(account_name voter_name){
  require_auth(voter_name);

  /*TODO Check if vote is active */

   /* have they voted */
  auto registered_voter = registered_voters.find(voter_name);
  eosio_assert(registered_voter != registered_voters.end(), "user has not voted");

  /* remove the user */
  registered_voters.erase(registered_voter);
}

void referendum::countvotes(account_name publisher){
  require_auth(publisher);

  /* check the vote is active */

  
  /* count the votes */
  uint64_t total_votes = 0;

  /* if vote_count / total_eos * 100 > MINIMUM_VOTE_PARTICIPATION_PERCENTAGE, this periods vote is succesful */

  /* if vote_count_no > vote_count_yes - 10%, it's a no vote */

  /* if not, check if there's enough time left in this period to finish the vote */

  /* if true, start again at 0 */

  /* if false, finish the vote */
 
  /* increment total days passed */ 
}

bool referendum::validate_side(uint8_t vote_side){

  switch(vote_side){
    case VOTE_SIDE_YES:
    case VOTE_SIDE_NO:
	return true;

    default:
	return false;
  }

}

}
