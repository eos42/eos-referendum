/* Licence: https://github.com/eos-referendum/master/LICENCE.md */

/*
 * File:   referendum.cpp
 * Author: Michael Fletcher | EOS42
 *
 * Created on 20 June 2018, 17:26
*/

#include "referendum.hpp"

/*TODO User can vote, unstake, move tokens and revote. We need to understand how this is dealt with when voting for BP's and how we can resolve */

namespace referendum {

void referendum::init(account_name self){

  require_auth(self);

  /* initalise tables */
  results.emplace(self, [&](auto &result_rec){
    result_rec.id = 0; //no vote
  });

  results.emplace(self, [&](auto &result_rec){
    result_rec.id = 1; //yes vote
  });
  
}

void referendum::vote(account_name voter_name, uint8_t vote_side){
  require_auth(voter_name);

  auto voter = voters.find(voter_name);

  /* if they've staked, their will be a voter entry */
  eosio_assert(voter != voters.end(), "user must stake before they can vote.");

  /* vote side yes = 1 or no = 0 */
  eosio_assert(validate_side(vote_side), "vote side is invalid");

   /* if a vote has been cast previously, check if the new vote is voting for the same side. If so - update the quantity */   
  if(voter->existing_vote_side != NULL_VOTE){
    eosio_assert(voter->existing_vote_side == vote_side, "cannot vote both yes and no at the same time.");
  }

   /* update the quantity if they've staked more, or if this is the users first time staking */
  double update_delta_quantity = voter->staked - voter->existing_vote_quantity;

  eosio_assert(update_delta_quantity == 0, "user has already voted their staked tokens");
 
   /* apply delta qty */   
  voters.modify(voter, _self, [&](auto &vote_rec){
      vote_rec.existing_vote_quantity = update_delta_quantity;
  });


  /* first time voting */
  if(voter->existing_vote_side != NULL_VOTE){
    voters.modify(voter, _self, [&](auto &vote_rec){
        vote_rec.existing_vote_side = vote_side;
      });
  }

  /* get the vote side we're looking for */
  auto result = results.find(vote_side);
  eosio_assert(result != results.end(), "contract has not been initialised");

  /* apply delta quantity */
  results.modify(result, _self, [&](auto &result_rec){
        result_rec.total_votes += update_delta_quantity;
      });
}


void referendum::unvote(account_name voter_name){
  require_auth(voter_name);

  /* find the voter and check they've voted */
  auto voter = voters.find(voter_name);
  eosio_assert(voter->existing_vote_quantity != 0, "user must vote before they can unvote");

  /* find the side they voted for */
  auto result = results.find(voter->existing_vote_side);
  eosio_assert(result != results.end(), "contract has not been initialised");

  /* reduce the votes */
  results.modify(result, _self, [&](auto &result_rec){
        result_rec.total_votes -= voter->existing_vote_quantity;
      });

  /* null the voter */
  voters.modify(voter, _self, [&](auto &vote_rec){
      vote_rec.existing_vote_quantity = 0;
      vote_rec.existing_vote_side = NULL_VOTE;
  });

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
