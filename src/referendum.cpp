/* Licence: https://github.com/eos-referendum/master/LICENCE.md */

/*
 * File:   referendum.cpp
 * Author: Michael Fletcher | EOS42
 *
 * Created on 16 May 2018, 17:26
*/

#include "referendum.hpp"

namespace referendum {

void referendum::vote(account_name voter, bool vote_yes){
  require_auth(voter);
}

void referendum::unvote(account_name voter){
  require_auth(voter);
}

}
