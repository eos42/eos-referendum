/* Licence: https://github.com/eos-referendum/master/LICENCE.md */

/*
 * File:   referendum.hpp
 * Author: Michael Fletcher | EOS42
 *
 * Created on 20 June 2018, 17:26
 */

#include <eosiolib/eosio.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/transaction.hpp>
#include <eosio.system/eosio.system.hpp>
#include <referendum_declarations.hpp>
#include <string>
#include <eosiolib/asset.hpp>

namespace referendum {

typedef eosio::multi_index<N(voters), eosiosystem::voter_info> voter_info_table;

struct undelegatebw {
    account_name from;
    account_name receiver;
    eosio::asset unstake_net_quantity;
    eosio::asset unstake_cpu_quantity;

    EOSLIB_SERIALIZE(undelegatebw, (from)(receiver)(unstake_net_quantity)(unstake_cpu_quantity)) //TODO Remove?
};

struct delegatebw {
    account_name from;
    account_name receiver;
    eosio::asset stake_net_quantity;
    eosio::asset stake_cpu_quantity;
    bool transfer;

    EOSLIB_SERIALIZE(delegatebw, (from)(receiver)(stake_net_quantity)(stake_cpu_quantity)(transfer)) //TODO Remove?
};


//@abi table
struct refconfig {
    uint64_t min_part_p; // min vote percent
    uint8_t vote_period_d;// total vote period
    uint8_t sust_vote_d; // total daily votes passed
    uint8_t yes_vote_w; // yes weight
    std::string name; // vote name
    std::string description; //vote desc

    EOSLIB_SERIALIZE(refconfig, (min_part_p)(vote_period_d)(sust_vote_d)(yes_vote_w)(name)(description))
};
typedef eosio::singleton<N(refconfig), refconfig> referendum_config_table;


//@abi table
struct refinfo {
    uint64_t total_days;  // total days passed
    uint8_t total_c_days; // total consecutive days vote has passed
    bool     vote_active; // false when the vote has finished
    uint64_t t_votes_yes; //total yes
    uint32_t t_votes_no;  //total no

    EOSLIB_SERIALIZE(refinfo, (total_days)(total_c_days)(vote_active)(t_votes_yes)(t_votes_no))
};
typedef eosio::singleton<N(refinfo), refinfo> referendum_results_table;


//@abi table
struct regvoters {
    account_name name;
    uint8_t	 vote_side;
    uint8_t	 total_votes;

    uint64_t primary_key() const {
        return name;
    }

    EOSLIB_SERIALIZE(regvoters, (name)(vote_side));
};
typedef eosio::multi_index<N(regvoters), regvoters>  registered_voters_table;



class referendum : public eosio::contract {
public:
    referendum(account_name self):contract(self),
        registered_voters(self, self),
        voter_info(N(eosio), N(eosio)),
        referendum_results(self,self),
        referendum_config(self, self) {}

    void init(account_name publisher);
    void vote(account_name voter, uint8_t vote_side);
    void unvote(account_name voter);
    void apply(account_name contract, account_name act);
    void countvotes(account_name publisher);
 
private:
    registered_voters_table    registered_voters;
    voter_info_table	       voter_info;
    referendum_results_table   referendum_results;
    referendum_config_table    referendum_config;

    void push_countvotes_transaction(uint64_t delay_sec);
    void update_tally(uint64_t delta_qty, uint8_t vote_side);
    void on(const undelegatebw &u);
    void on(const delegatebw &u);
    bool validate_side(uint8_t vote_side);
};

}
