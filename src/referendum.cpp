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

/*TODO Ensure no one can alter referendum contract tables */
/*TODO when contract ended, finsh / tidy up tables */
/*TODO Check only correct people can call contract function */
/*TODO Only self can count votes */
/*TODO Double check the on( functions only count EOS. */
/*TODO Allow Multiple Vote */
void referendum::init(account_name const publisher)
{
    require_auth(publisher);

    /* referendum can only be initialised once */
    eosio_assert(!referendum_config.exists(), "vote has already been initialised");

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
    ref_info.t_votes_yes = 0;
    ref_info.t_votes_no = 0;
    referendum_results.set(ref_info, _self);

    /* votes will be counted 24 hours from initalisation */
    push_countvotes_transaction(TIME_DAY);
}


//@abi action
void referendum::vote(account_name const voter_name, uint8_t const vote_side) {
    require_auth(voter_name);

    /* check vote has been initialised */
    eosio_assert(referendum_config.exists(), "vote has not been initialised");

    /* check if vote is active */
    eosio_assert(referendum_results.get().vote_active, "voting has finished");

    /* if user has staked, their will be a voter entry */
    auto const voter = voter_info.find(voter_name);
    eosio_assert(voter == voter_info.end(), "user must stake before they can vote");

    /* vote side yes = 1 or no = 0 */
    eosio_assert(validate_side(vote_side), "vote side is invalid");

    /* have they already voted */
    auto const registered_voter = registered_voters.find(voter_name);
    eosio_assert(registered_voter == registered_voters.end(), "user has already voted");

    /* register vote */
    registered_voters.emplace(_self, [&](auto &voter_rec) {
        voter_rec.name = voter_name;
        voter_rec.vote_side = vote_side;
        voter_rec.total_votes = voter->staked;
    });

    /* tally the voter, any change in bw will update the tally automatically */
    update_tally(voter->staked, vote_side);

}

//@abi action
void referendum::unvote(account_name const voter_name) {
    require_auth(voter_name);

    /* check vote has been initalised*/
    eosio_assert(referendum_config.exists(), "vote has not been initialised");

    /* check if vote is active */
    eosio_assert(referendum_results.get().vote_active, "voting has finished");

    /* have they voted */
    auto registered_voter = registered_voters.find(voter_name);
    eosio_assert(registered_voter != registered_voters.end(), "user has not voted");

    /* find the voter record to check how much is staked */
    auto const voter = voter_info.find(voter_name);

    /* update the tally */
    update_tally(-voter->staked, registered_voter->vote_side);

    /* remove the user */
    registered_voters.erase(registered_voter);
}


void referendum::update_tally(uint64_t const delta_qty, uint8_t const vote_side)
{
    refinfo ref_info;

    auto const itr = referendum_results.get();

    ref_info.t_votes_yes = itr.t_votes_yes;
    ref_info.t_votes_no = itr.t_votes_no;

    switch(vote_side)
    {
    case VOTE_SIDE_YES:
        ref_info.t_votes_yes += delta_qty;
        break;
    case VOTE_SIDE_NO:
        ref_info.t_votes_no += delta_qty;
        break;
    default:
        break;
    }

    referendum_results.set(ref_info, _self);
}



void referendum::countvotes(account_name const publisher) {
    require_auth(publisher);

    /* check vote has been initalised*/
    eosio_assert(referendum_results.exists(), "vote has not been initialised");

    auto const results_itr = referendum_results.get();

    double const total_votes = results_itr.t_votes_yes + results_itr.t_votes_no;

    /* TODO -> we can make this dynamic by looking up how many EOS currently exist. it will do for now */
    double const total_network_vote_percentage = 100.0 * total_votes / TOTAL_AVAILABLE_EOS; //todo check how system contract reads max token supply

    /* calculate vote percentages */
    double const yes_vote_percentage = 100.0 * results_itr.t_votes_yes / total_votes;
    double const no_vote_percentage = 100.0 * results_itr.t_votes_no / total_votes;

    bool vote_period_passed = false;

    /* is it greater than or equal to the minimum pariticpation i.e 15%? */
    if(total_network_vote_percentage >= MINIMUM_VOTE_PARTICIPATION_PERCENTAGE)
    {
        /* Do we have more yes votes than no */
        if(results_itr.t_votes_yes > (results_itr.t_votes_no + YES_LEADING_VOTE_PERCENTAGE))
        {
            vote_period_passed = true;
        }
    }

    /* how many days have passed since the vote started + how many consecutive days has the vote been successful */
    uint64_t total_days = results_itr.total_days;
    uint64_t total_c_days = results_itr.total_c_days;

    refinfo new_referendum_info;
    if(vote_period_passed) 
    {
        /* todays vote has passed */
        total_days++;
        total_c_days++;
        new_referendum_info.total_days = total_days;
        new_referendum_info.total_c_days = total_c_days;
        new_referendum_info.vote_active = true;

    } 
    else 
    {
        /* todays vote has failed, start again */
        total_days++;
        new_referendum_info.total_days = total_days;
        new_referendum_info.total_c_days = 0;

        /* do we have enough time left within the vote period to complete a succesful vote if we start again? */
        if(new_referendum_info.total_days + SUSTAINED_VOTE_PERIOD_DAYS > REFERENDUM_VOTE_PERIOD_DAYS)
        {
            new_referendum_info.vote_active = false;
        } 
        else 
        {
            new_referendum_info.vote_active = true;
        }

    }

    /* have we reached the minimum succesful consecutive day threshold? */
    if(new_referendum_info.total_c_days >= SUSTAINED_VOTE_PERIOD_DAYS )
    {
        new_referendum_info.vote_active = false; // the vote has passed!
    }

    /* Update the singleton storing referendum data */
    referendum_results.set(new_referendum_info, _self);

    /* count the votes again in 24 hours */
    push_countvotes_transaction(TIME_DAY);
}

void referendum::push_countvotes_transaction(uint64_t const delay_sec) {
    eosio::transaction out;
    out.actions.emplace_back( eosio::permission_level{ _self, N(active) }, _self, N(countvotes), _self );
    out.delay_sec = delay_sec;
    out.send(_self, _self, true);
}

bool referendum::validate_side(uint8_t const vote_side) {
  return vote_side == VOTE_SIDE_YES || vote_side == VOTE_SIDE_NO;
}

void referendum::on(undelegatebw const & u) {

    /* check user is a voter */
    auto reg_voter = registered_voters.find(u.receiver);
    if(registered_voters.find(u.receiver) == registered_voters.end()) {
        return;
    }

    /* update the tally */
    eosio::asset total_update = u.unstake_net_quantity + u.unstake_cpu_quantity;
    update_tally(-total_update.amount, reg_voter->vote_side);

    /* update user votes */
    registered_voters.modify(reg_voter, _self, [&](auto &voter_rec) {
        voter_rec.total_votes -= total_update.amount;
    });
}

void referendum::on(delegatebw const & d) {

    /* check user is a voter */
    auto reg_voter = registered_voters.find(d.receiver);
    if(reg_voter == registered_voters.end()) {
        return;
    }

    /* update the tally */
    eosio::asset total_update = d.stake_net_quantity + d.stake_cpu_quantity;
    update_tally(total_update.amount, reg_voter->vote_side);

    /* update user votes */
    registered_voters.modify(reg_voter, _self, [&](auto &voter_rec) {
        voter_rec.total_votes += total_update.amount;
    });
}


void referendum::apply(account_name contract, account_name const act)
{
    /* listens for delegate / undelegate actions of users who have voted to adjust the tallies */
    if(contract == N(system))
    {
        switch(act) {
        case N(undelegatebw):
            on(eosio::unpack_action_data<undelegatebw>());
            return;
        case N(delegatebw):
            on(eosio::unpack_action_data<delegatebw>());
            return;

        default:
            break;

        }
    }

    if(contract == _self) {
        auto& thiscontract = *this; 
        switch(act) {
            EOSIO_API(referendum, (init)(vote)(unvote)(countvotes));
        }

    }

}

extern "C" {
    [[noreturn]] void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
        referendum  ref(receiver);
        ref.apply( code, action );
        eosio_exit(0);
    }
}

}
