/* Licence: https://github.com/eos-referendum/master/LICENCE.md */

/*
 * File:   referendum.cpp
 * Author: Michael Fletcher | EOS42
 *
 * Created on 20 June 2018, 17:26
*/

#include "referendum.hpp"
#include <eosiolib/print.hpp>

namespace referendum {

/*TODO when contract ended, finsh / tidy up tables */
/*TODO Check only correct people can call contract function */
/*TODO Only self can count votes */
/* check processing time < 30 ms + do recursive transactions */

void referendum::init(account_name publisher)
{
    require_auth(publisher);

    /* referendum can only be initialised once */
    //eosio_assert(!referendum_config.exists(), "vote has already been initialised");

    /* init config */
    refconfig ref_config;

    ref_config.min_part_p = MINIMUM_VOTE_PARTICIPATION_PERCENTAGE;
    ref_config.vote_period_d = REFERENDUM_VOTE_PERIOD_DAYS;
    ref_config.sust_vote_d = SUSTAINED_VOTE_PERIOD_DAYS;
    ref_config.yes_vote_w = YES_LEADING_VOTE_PERCENTAGE;
    ref_config.name = VOTE_NAME;
    ref_config.description = VOTE_DESCRIPTION;

    referendum_config.set(ref_config, _self);

    /* init info */
    refinfo ref_info;

    ref_info.total_days = 0;
    ref_info.total_c_days = 0;
    ref_info.vote_active = true;

    referendum_results.set(ref_info, _self);

    /* init counts */
    refcount ref_count;

    ref_count.bookmark = 0;
    ref_count.locked = false;
    ref_count.t_votes_yes = 0;
    ref_count.t_votes_no = 0;

    referendum_count.set(ref_count, _self);

        /*continue counting vote*/
        push_countvotes_transaction(6);
      /*continue counting vote*/
        push_countvotes_transaction(9);
    /*continue counting vote*/
        push_countvotes_transaction(12);
    /*continue counting vote*/
        push_countvotes_transaction(15);
    /*continue counting vote*/
        push_countvotes_transaction(18);

    /* votes will be counted 24 hours from initalisation */
 // TODO   push_countvotes_transaction(86400);
}


//@abi action
void referendum::vote(account_name voter_name, uint8_t vote_side) {
    require_auth(voter_name);

    /* check vote has been initialised */
    eosio_assert(referendum_config.exists(), "vote has not been initialised");

    /* check the votes are not being counted */
    eosio_assert(!referendum_count.get().locked, "todays vote is currently being counted, try again shortly");

    /* check if vote is active */
    eosio_assert(referendum_results.get().vote_active, "voting has finished");

    /* if they've staked, their will be a voter entry */
    auto voter = voter_info.find(voter_name);
    eosio_assert(voter == voter_info.end(), "user must stake before they can vote");

    /* vote side yes = 1 or no = 0 */
    eosio_assert(validate_side(vote_side), "vote side is invalid");

    /* have they already voted */
    auto registered_voter = registered_voters.find(voter_name);
    eosio_assert(registered_voter == registered_voters.end(), "user has already voted");

    /* register vote */
    registered_voters.emplace(_self, [&](auto &voter_rec) {
        voter_rec.name = voter_name;
        voter_rec.vote_side = vote_side;
    });

}

//@abi action
void referendum::unvote(account_name voter_name) {
    require_auth(voter_name);

    /* check vote has been initalised*/
    eosio_assert(referendum_config.exists(), "vote has not been initialised");

    /* check the votes are not being counted */
    eosio_assert(!referendum_count.get().locked, "todays vote is currently being counted, try again shortly");

    /* check if vote is active */
    eosio_assert(referendum_results.get().vote_active, "voting has finished");

    /* have they voted */
    auto registered_voter = registered_voters.find(voter_name);
    eosio_assert(registered_voter != registered_voters.end(), "user has not voted");

    /* remove the user */
    registered_voters.erase(registered_voter);
}


void referendum::countvotes(account_name publisher) {
    require_auth(publisher);

    eosio::print("1");

    /* check if vote is active */
    eosio_assert(referendum_results.get().vote_active, "voting has finished");

    /* if we're starting a new iteration, and record count > max. store bookmarks */
    refcount ref_count;

    /* if it's locked, we're mid count - continue where we left off */
    auto itr = registered_voters.begin();
    if(referendum_count.get().locked) {
        itr = registered_voters.find(referendum_count.get().bookmark);

        ref_count.t_votes_yes = referendum_count.get().t_votes_yes;
        ref_count.t_votes_no = referendum_count.get().t_votes_no;
        ref_count.locked = false;
    }

    /* check all the registered voters */
    for(int s = 0; itr != registered_voters.end(); itr++, s++ )
    {

        /* limit how many iterations we can count */
        if(s >= COUNT_BATCH_SIZE )
        {
            ref_count.locked = true; // keep locked to continue counting
            ref_count.bookmark = itr->name; // the last vote we processed
            break;
        }

        auto user_votes = voter_info.find(itr->name);
        if(user_votes == voter_info.end()) {
         //TODO   continue;  // user has not staked
        }

        /* count each side */
        switch(itr->vote_side)
        {
        case VOTE_SIDE_YES:
            ref_count.t_votes_yes += 50000000;//TODO user_votes->staked;
            break;

        case VOTE_SIDE_NO:
            ref_count.t_votes_no += 25000000;//TODOuser_votes->staked;
            break;

        default:
            continue;
            break;
        }

    }

	eosio::print("2");
    /* vote isn't locked, calclate the results */
    if(!ref_count.locked) {

	eosio::print("3");
        double total_votes = ref_count.t_votes_yes + ref_count.t_votes_no;

        /* TODO -> we can make this dynamic by looking up how many EOS currently exist. it will do for now */
        double total_network_vote_percentage = total_votes / TOTAL_AVAILABLE_EOS  * 100; //todo check how system contract reads max token supply

        /* calculate vote percentages */
        double yes_vote_percentage = ref_count.t_votes_yes / total_votes * 100;
        double no_vote_percentage = ref_count.t_votes_no / total_votes * 100;

        bool vote_period_passed = false;

        /* is it greater than the minimum pariticpation i.e 15%? */
        if(total_network_vote_percentage >= MINIMUM_VOTE_PARTICIPATION_PERCENTAGE)
        {
            /* Do we have more yes votes than no */
            if(ref_count.t_votes_yes > (ref_count.t_votes_no + YES_LEADING_VOTE_PERCENTAGE))
            {
                vote_period_passed = true;
            }
        }

        /* how many days have passed since the vote started + how many consecutive days has the vote been succesful */
        uint64_t total_days = referendum_results.get().total_days;
        uint64_t total_c_days = referendum_results.get().total_c_days;

        /* todays vote has passed */
        refinfo new_referendum_info;
        if(vote_period_passed) {
            new_referendum_info.total_days = ++total_days;
            new_referendum_info.total_c_days = ++total_c_days;
            new_referendum_info.vote_active = true;

        } else {
            /* todays vote has failed, start again */
            new_referendum_info.total_days = ++total_days;
            new_referendum_info.total_c_days = 0;

            /* do we have enough time left within the vote period to complete a succesful vote if we start again? */
            if(new_referendum_info.total_days + SUSTAINED_VOTE_PERIOD_DAYS > REFERENDUM_VOTE_PERIOD_DAYS)
            {
                new_referendum_info.vote_active = false;
            } else {
                new_referendum_info.vote_active = true;
            }

        }

        /* initalise the ref counts, as the day has been counted */
        ref_count.t_votes_yes = 0;
        ref_count.t_votes_no = 0;
        ref_count.bookmark = 0;

        /* have we reached the minimum succesful consecutive day threshold? */
        if(new_referendum_info.total_c_days >= SUSTAINED_VOTE_PERIOD_DAYS )
        {
            new_referendum_info.vote_active = false; // the vote has passed!
        }

        /*submit transaction to count vote again in 24 hours if there's no more votes to count and it's still active*/
        if(new_referendum_info.vote_active) {
          //TODO  push_countvotes_transaction(86400);
        }

        /* Update the singleton storing referendum data */
        referendum_results.set(new_referendum_info, _self);


    } else {
  
	eosio::print("4");
  
      /*continue counting vote*/
//        push_countvotes_transaction(6);

    }

    /*Update the refendum count */
    referendum_count.set(ref_count, _self);
}

void referendum::push_countvotes_transaction(uint64_t delay_sec) {
    /*submit transaction to count vote again in 24 hours*/
    eosio::transaction out;
 
    out.actions.emplace_back( eosio::permission_level{ _self, N(active) }, _self, N(countvotes), _self );

    out.delay_sec = delay_sec;
    out.send(_self, _self, true);
}

bool referendum::validate_side(uint8_t vote_side) {
    switch(vote_side) {
    case VOTE_SIDE_YES:
    case VOTE_SIDE_NO:
        return true;

    default:
        return false;
    }
}

}
