/*
   Boneh-Gentry-Waters
   Collusion Resistant Broadcast Encryption With Short Ciphertexts and Private Keys
   Implemented on Type-1 pairing

   Compile with modules as specified below

        For MR_PAIRING_SSP curves
        cl /O2 /GX bgw.cpp ssp_pair.cpp ecn.cpp zzn2.cpp zzn.cpp big.cpp miracl.lib

        For MR_PAIRING_SS2 curves
    cl /O2 /GX bgw.cpp ss2_pair.cpp ec2.cpp gf2m4x.cpp gf2m.cpp big.cpp miracl.lib

        or of course

    g++ -O2 bgw.cpp ss2_pair.cpp ec2.cpp gf2m4x.cpp gf2m.cpp big.cpp miracl.a -o bgw

   See http://eprint.iacr.org/2005/018.pdf
   Section 3.1

*/

#include "FNIAGKA/pki.h"
#include "FNIAGKA/proto.h"
#include "log.h"
#include "metric.h"
#include "singleton.h"

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


extern int kFastBenchmarkThreshold;
constexpr int kMergeSplitEventThreshold = 10;
constexpr int kPNNum = 1;

bool
UseFastBenchmarkMode(int eta)
{
    return eta >= kFastBenchmarkThreshold;
}

//********* CHOOSE JUST ONE OF THESE **********
// #define MR_PAIRING_SS2  // AES-80 or AES-128 security GF(2^m) curve
// #define AES_SECURITY 80 // OR
// #define AES_SECURITY 128

#define MR_PAIRING_SSP  // AES-80 or AES-128 security GF(p) curve
#define AES_SECURITY 80 // OR

// #define AES_SECURITY 128
//*********************************************

void
TestEncapDecap(int eta,
               std::shared_ptr<FullParameter> omega,
               std::shared_ptr<GroupInfo> group_info,
               std::vector<std::shared_ptr<FNIAGKA::User>>& users,
               EncryptionKey& ek,
               std::vector<DecryptionKey>& dks,
               const std::vector<int64_t>& selected) // selected indices (not uids)
{
    // check inputs
    if (users.size() != dks.size())
    {
        FATAL_ERROR("TestEncapDecap, Bad inputs, users.size() != dks.size()");
    }

    std::unordered_set<int64_t> group_members;
    for (int i = 0; i < group_info->membership_.size(); i++)
    {
        if (group_info->membership_[i] != -1)
        {
            group_members.insert(group_info->membership_[i]);
        }
    }

    std::vector<int64_t> receiver_uids;
    for (auto i : selected)
    {
        if (group_members.find(users[i]->uid_) == group_members.end())
        {
            FATAL_ERROR("TestEncapDecap, Bad inputs, User "
                        << users[i]->uid_ << " is not a member of group " << group_info->gid_);
        }
        receiver_uids.push_back(users[i]->uid_);
    }

    auto& metric = Singleton<Metric>::GetInstance();

    auto key = metric.GenerateStatKey(EmitType::kComputeEncap);
    metric.Emit(EmitType::kComputeEncap, key);
    auto encap_result = FNIAGKA::Encap(group_info->eta_, omega, group_info, receiver_uids, ek);
    metric.Emit(EmitType::kComputeEncap, key);

    for (auto i : selected)
    {
        key = metric.GenerateStatKey(EmitType::kComputeDecap);
        metric.Emit(EmitType::kComputeDecap, key);
        auto decap_result = FNIAGKA::Decap(group_info->eta_,
                                           omega,
                                           group_info,
                                           users[i],
                                           receiver_uids,
                                           dks[i],
                                           encap_result.second);

        metric.Emit(EmitType::kComputeDecap, key);
        if (!decap_result.second)
        {
            FATAL_ERROR("TestEncapDecap failed!");
        }
        else
        {
            break;
        }
    }

    HIGHLIGHT("TestEncapDecap passed");
}

void
TestEncapDecap(int eta,
               std::shared_ptr<FullParameter> omega,
               std::shared_ptr<GroupInfo> group_info,
               std::unordered_map<int64_t, std::shared_ptr<FNIAGKA::User>>& users,
               EncryptionKey& ek,
               std::unordered_map<int64_t, DecryptionKey>& dks,
               const std::vector<int64_t>& selected_uids)
{
    // check inputs
    std::unordered_set<int64_t> group_members;
    for (int i = 0; i < group_info->membership_.size(); i++)
    {
        if (group_info->membership_[i] != -1)
        {
            group_members.insert(group_info->membership_[i]);
        }
    }

    std::vector<int64_t> receiver_uids;
    for (auto uid : selected_uids)
    {
        if (group_members.find(uid) == group_members.end())
        {
            FATAL_ERROR("TestEncapDecap, Bad inputs, User " << uid << " is not a member of group "
                                                            << group_info->gid_);
        }
        receiver_uids.push_back(uid);
    }

    auto& metric = Singleton<Metric>::GetInstance();
    auto key = metric.GenerateStatKey(EmitType::kComputeEncap);
    metric.Emit(EmitType::kComputeEncap, key);
    auto encap_result = FNIAGKA::Encap(eta, omega, group_info, receiver_uids, ek);
    metric.Emit(EmitType::kComputeEncap, key);

    for (auto uid : selected_uids)
    {
        key = metric.GenerateStatKey(EmitType::kComputeDecap);
        metric.Emit(EmitType::kComputeDecap, key);
        auto decap_result = FNIAGKA::Decap(eta,
                                           omega,
                                           group_info,
                                           users[uid],
                                           receiver_uids,
                                           dks[uid],
                                           encap_result.second);
        metric.Emit(EmitType::kComputeDecap, key);

        if (!decap_result.second)
        {
            FATAL_ERROR("TestEncapDecap failed!");
        }
        else
        {
            break;
        }
    }

    HIGHLIGHT("TestEncapDecap passed");
}

void
TestSplitMergeGroup(int eta,
                    std::shared_ptr<FullParameter> omega,
                    int split_group_num,
                    std::unordered_map<int64_t, std::shared_ptr<FNIAGKA::User>> users,
                    std::unordered_map<int64_t, EncryptionKey>& encryption_keys,
                    std::unordered_map<int64_t, DecryptionKey>& decryption_keys)
{
    // initialize
    auto& pki = Singleton<PKI>::GetInstance();
    auto group_info = GroupInfo::NewGroupInfo(eta);
    auto& metric = Singleton<Metric>::GetInstance();

    for (auto& pair : users)
    {
        group_info.Occupy(pair.first);
    }

    TestEncapDecap(eta,
                   omega,
                   std::make_shared<GroupInfo>(group_info),
                   users,
                   encryption_keys.begin()->second,
                   decryption_keys,
                   group_info.GetMembers());

    // split
    std::vector<GroupInfo> group_infos_after_split;
    for (int i = 0; i < split_group_num; i++)
    {
        group_infos_after_split.push_back(GroupInfo::NewGroupInfo(eta));
    }

    int c = 0;
    for (int i = 0; i < group_info.membership_.size(); i++)
    {
        if (group_info.membership_[i] == -1)
        {
            continue;
        }
        int idx = c % split_group_num;
        group_infos_after_split[idx].Occupy(group_info.membership_[i]);
        ++c;
    }

    for (auto& pair : users)
    {
        EmitType et = (EmitType)((int)EmitType::kComputeSplit_2 + (split_group_num - 2));
        if (metric.GetEventCount(et) < kMergeSplitEventThreshold)
        {
            auto key = metric.GenerateStatKey(et);
            metric.Emit(et, key);
            FNIAGKA::SplitGroup(eta,
                                omega,
                                pair.second,
                                std::make_shared<GroupInfo>(group_info),
                                group_infos_after_split,
                                encryption_keys[pair.first],
                                decryption_keys[pair.first]);
            metric.Emit(et, key);
        }
    }

    for (int i = 0; i < group_infos_after_split.size(); i++)
    {
        auto tmp_uid = group_infos_after_split[i].GetMembers()[0];
        TestEncapDecap(eta,
                       omega,
                       std::make_shared<GroupInfo>(group_infos_after_split[i]),
                       users,
                       encryption_keys[tmp_uid],
                       decryption_keys,
                       group_infos_after_split[i].GetMembers());
    }

    // merge
    for (auto& user : users)
    {
        std::vector<EncryptionKey> cur_eks;
        for (int j = 0; j < split_group_num; j++)
        {
            cur_eks.push_back(encryption_keys[group_infos_after_split[j].GetMembers()[0]]);
        }

        EmitType et = (EmitType)((int)EmitType::kComputeMergeStandard_2 + (split_group_num - 2));
        if (metric.GetEventCount(et) < kMergeSplitEventThreshold)
        {
            auto key = metric.GenerateStatKey(et);
            metric.Emit(et, key);
            std::vector<EncryptionKey> new_eks = FNIAGKA::MergeGroup(eta,
                                                                    omega,
                                                                    user.second,
                                                                    cur_eks,
                                                                    decryption_keys[user.first],
                                                                    FNIAGKA::MergeMode::kStandard);
            metric.Emit(et, key);
        }

        et = (EmitType)((int)EmitType::kComputeMergeExtended_2 + (split_group_num - 2));
        if (metric.GetEventCount(et) < kMergeSplitEventThreshold)
        {
            auto key = metric.GenerateStatKey(et);
            metric.Emit(et, key);
            std::vector<EncryptionKey> new_eks_extended =
                FNIAGKA::MergeGroup(eta,
                                    omega,
                                    user.second,
                                    cur_eks,
                                    decryption_keys[user.first],
                                    FNIAGKA::MergeMode::kExtended);
            metric.Emit(et, key);
        }

        // for (int i = 0; i < new_eks_extended.size(); i++)
        // {
        //     TestEncapDecap(eta,
        //                    omega,
        //                    std::make_shared<GroupInfo>(new_eks_extended[i].group_info_),
        //                    users,
        //                    new_eks_extended[i],
        //                    decryption_keys,
        //                    new_eks_extended[i].group_info_.GetMembers());
        // }
    }
}

void
TestSAAGKA(int security_level, std::shared_ptr<PublicParameter> pp)
{
    std::vector<std::chrono::microseconds> durations;

    for (int i = 1; i <= 10; i++)
    {
        int group_size = i * 10;
        int mult_cnt = 3 * group_size;
        int exp_cnt = group_size;
        int pairing_cnt = 1;

        std::vector<Big> r(exp_cnt);
        for (int j = 0; j < exp_cnt; j++)
        {
            pp->pfc_->random(r[j]);
        }
        G1 tmp = pp->pfc_->mult(pp->g0_, r[0]);

        // start
        auto now = std::chrono::steady_clock::now();
        for (int j = 0; j < mult_cnt; j++)
        {
            tmp = tmp + tmp;
        }

        for (int j = 0; j < exp_cnt; j++)
        {
            pp->pfc_->mult(pp->g0_, r[j]);
        }

        pp->pfc_->pairing(tmp, tmp);
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - now);
        durations.push_back(duration);
    }

    std::cout << "[";
    for (int i = 0; i < durations.size(); i++)
    {
        std::cout << durations[i].count();
        if (i != durations.size() - 1)
        {
            std::cout << ",";
        }
    }
    std::cout << "]";
}

void
TestUpkUpdate(std::shared_ptr<PublicParameter> pp,
              std::shared_ptr<FullParameter> omega,
              int user_num)
{
    // initialize
    auto& pki = Singleton<PKI>::GetInstance();
    std::vector<std::shared_ptr<FNIAGKA::User>> users(user_num);
    for (int i = 0; i < user_num; i++)
    {
        users[i] = FNIAGKA::UserGen(pp);
        pki.UserRegister(users[i]->uid_, users[i]->upk_);
    }
    auto group_info = GroupInfo::NewGroupInfo(pp->max_group_size_);
    for (int i = 0; i < users.size(); i++)
    {
        group_info.Occupy(users[i]->uid_);
    }

    std::vector<EncryptionKey> encryption_keys(user_num);
    std::vector<DecryptionKey> decryption_keys(user_num);
    for (int i = 0; i < users.size(); i++)
    {
        FNIAGKA::Agree(pp->max_group_size_,
                       omega,
                       users[i],
                       std::make_shared<GroupInfo>(group_info),
                       encryption_keys[i],
                       decryption_keys[i]);
    }

    // test
    auto& metric = Singleton<Metric>::GetInstance();
    for (int i = 0; i < users.size(); i++)
    {
        auto key = metric.GenerateStatKey(EmitType::kComputeUpdateUpk);
        UserPublicKey stale_upk = *users[i]->upk_;
        UserPrivateKey stale_usk = *users[i]->usk_;

        // update upk
        metric.Emit(EmitType::kComputeUpdateUpk, key);
        users[i]->UpdateKey(pp);
        metric.Emit(EmitType::kComputeUpdateUpk, key);

        // update group key
        for (int j = 0; j < users.size(); j++)
        {
            auto key = metric.GenerateStatKey(EmitType::kComputeUpdateGroupKey);
            metric.Emit(EmitType::kComputeUpdateGroupKey, key);
            if (j != i)
            {
                FNIAGKA::UpdateGroupKey(pp->max_group_size_,
                                        omega,
                                        users[j],
                                        users[i]->uid_,
                                        encryption_keys[j],
                                        decryption_keys[j],
                                        stale_upk);
            }
            else
            {
                FNIAGKA::UpdateGroupKey(pp->max_group_size_,
                                        omega,
                                        users[i],
                                        users[i]->uid_,
                                        encryption_keys[i],
                                        decryption_keys[i],
                                        stale_upk,
                                        std::make_shared<UserPrivateKey>(stale_usk));
            }
            metric.Emit(EmitType::kComputeUpdateGroupKey, key);
        }
    }

    metric.Summarize();
}

void
TestUpkUpdateV2(std::shared_ptr<FullParameter> omega, int user_num)
{
    // initialize
    auto& pki = Singleton<PKI>::GetInstance();
    std::vector<std::shared_ptr<FNIAGKA::User>> users(user_num);
    for (int i = 0; i < user_num; i++)
    {
        users[i] = FNIAGKA::UserGen(omega->pp_);
        pki.UserRegister(users[i]->uid_, users[i]->upk_);
    }

    std::vector<EncryptionKey> encryption_keys(user_num);
    std::vector<DecryptionKey> decryption_keys(user_num);
    auto group_info = GroupInfo::NewGroupInfo(omega->pp_->max_group_size_);
    for (int i = 0; i < users.size(); i++)
    {
        group_info.Occupy(users[i]->uid_);
    }
    for (int i = 0; i < users.size(); i++)
    {
        FNIAGKA::Agree(omega->pp_->max_group_size_,
                       omega,
                       users[i],
                       std::make_shared<GroupInfo>(group_info),
                       encryption_keys[i],
                       decryption_keys[i]);
    }

    std::vector<EncryptionKey> eks_used_for_update;
    eks_used_for_update.push_back(encryption_keys[0]);

    // test
    auto& metric = Singleton<Metric>::GetInstance();
    auto key = metric.GenerateStatKey(EmitType::kComputeUpdateUpkLaunchV2);
    metric.Emit(EmitType::kComputeUpdateUpkLaunchV2, key);
    auto kum = FNIAGKA::UserKeyUpdLaunch(omega, 1, eks_used_for_update);
    metric.Emit(EmitType::kComputeUpdateUpkLaunchV2, key);

    // duplicate UserKeyUpdLaunch for more accurate measurement
    for (int L = 1; L <= 5; L++)
    {
        for (int v = 2; v <= 10; v++)
        {
            std::vector<EncryptionKey> tmp_eks(L, encryption_keys[0]);
            EmitType et = (EmitType)((int)EmitType::kComputeUpdateUpkLaunchV2 + (L - 1));
            key = metric.GenerateStatKey(et);
            metric.Emit(et, key);
            FNIAGKA::UserKeyUpdLaunch(omega, v, tmp_eks);
            metric.Emit(et, key);
        }
    }

    // update upk and group key

    for (int i = 0; i < users.size(); i++)
    {
        key = metric.GenerateStatKey(EmitType::kComputeUpdateUpkV2);
        metric.Emit(EmitType::kComputeUpdateUpkV2, key);
        FNIAGKA::UserKeyUpd(omega, users[i], decryption_keys[i], kum);
        metric.Emit(EmitType::kComputeUpdateUpkV2, key);
    }

    for (int i = 0; i < users.size(); i++)
    {
        key = metric.GenerateStatKey(EmitType::kComputeUpdateGroupKeyV2);
        metric.Emit(EmitType::kComputeUpdateGroupKeyV2, key);
        FNIAGKA::GroupKeyUpd(omega,
                             std::make_shared<GroupInfo>(group_info),
                             users[i],
                             encryption_keys[i],
                             decryption_keys[i],
                             1);
        metric.Emit(EmitType::kComputeUpdateGroupKeyV2, key);
    }

    metric.Summarize();
}

int
main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << " <security_level> <max_group_size> [test_type]"
                  << std::endl;
        return 1;
    }
    int security_level = std::stoi(argv[1]);
    int max_group_size = std::stoi(argv[2]);
    std::string test_type = "default";
    if (argc >= 4)
    {
        test_type = argv[3];
    }

    if (security_level != 80 && security_level != 128)
    {
        std::cout << "security_level must be 80 or 128" << std::endl;
        return 1;
    }

    auto& metric = Singleton<Metric>::GetInstance();

    auto key = metric.GenerateStatKey(EmitType::kComputeSetup);

    metric.Emit(EmitType::kComputeSetup, key);
    auto pp = FNIAGKA::Setup(security_level, max_group_size);
    metric.Emit(EmitType::kComputeSetup, key);

    std::vector<std::shared_ptr<PNPublicKey>> pn_pks(kPNNum);
    for (int i = 0; i < kPNNum; i++)
    {
        key = metric.GenerateStatKey(EmitType::kComputePNGen);
        metric.Emit(EmitType::kComputePNGen, key);
        pn_pks[i] = FNIAGKA::PNGen(pp);
        metric.Emit(EmitType::kComputePNGen, key);
    }

    key = metric.GenerateStatKey(EmitType::kComputeNegotiate);
    metric.Emit(EmitType::kComputeNegotiate, key);
    auto omega = FNIAGKA::Negotiate(pp, pn_pks);
    metric.Emit(EmitType::kComputeNegotiate, key);

    std::cout << "Negotiate DONE" << std::endl;

    if (test_type == "test_upk_update")
    {
        TestUpkUpdate(pp, omega, 10);
        return 0;
    }
    else if (test_type == "test_upk_update_v2")
    {
        TestUpkUpdateV2(omega, max_group_size);
        return 0;
    }

    auto& pki = Singleton<PKI>::GetInstance();

    int user_total_num = max_group_size;
    std::vector<std::shared_ptr<FNIAGKA::User>> users(user_total_num);
    for (int i = 0; i < user_total_num; i++)
    {
        // Emit metric directly if fast mode is disabled. 
        // If fast mode is enabled, only record the first few `UserGen` that perform actual computation.
        bool should_emit_metric = !UseFastBenchmarkMode(max_group_size) || (UseFastBenchmarkMode(max_group_size) && i < kCachedPoolSize);

        if (should_emit_metric) {
            key = metric.GenerateStatKey(EmitType::kComputeUserGen);
            metric.Emit(EmitType::kComputeUserGen, key);
            users[i] = FNIAGKA::UserGen(pp);
            metric.Emit(EmitType::kComputeUserGen, key);
        } else {
            users[i] = FNIAGKA::UserGen(pp);
        }
        pki.UserRegister(users[i]->uid_, users[i]->upk_);
    }
    std::cout << "UserGen DONE" << std::endl;

    auto group_info = GroupInfo::NewGroupInfo(max_group_size);
    for (int i = 0; i < users.size() - 1; i++)
    {
        group_info.Occupy(users[i]->uid_);
    }

    std::vector<EncryptionKey> encryption_keys(user_total_num);
    std::vector<DecryptionKey> decryption_keys(user_total_num);
    for (int i = 0; i < users.size() - 1; i++)
    {
        key = metric.GenerateStatKey(EmitType::kComputeAgree);
        metric.Emit(EmitType::kComputeAgree, key);
        FNIAGKA::Agree(max_group_size,
                       omega,
                       users[i],
                       std::make_shared<GroupInfo>(group_info),
                       encryption_keys[i],
                       decryption_keys[i]);
        metric.Emit(EmitType::kComputeAgree, key);
    }

    TestEncapDecap(max_group_size,
                   omega,
                   std::make_shared<GroupInfo>(group_info),
                   users,
                   encryption_keys[0],
                   decryption_keys,
                   group_info.GetMembers());

    INFO("============ Test Add ============");

    auto uid_to_be_added = users[user_total_num - 1]->uid_;
    int j = 0;
    for (int j = 0; j < users.size() - 1; j++)
    {
        key = metric.GenerateStatKey(EmitType::kComputeAddUpd);
        metric.Emit(EmitType::kComputeAddUpd, key);
        FNIAGKA::AddUser(max_group_size,
                         omega,
                         users[j],
                         std::make_shared<GroupInfo>(group_info),
                         uid_to_be_added,
                         encryption_keys[j],
                         decryption_keys[j]);
        metric.Emit(EmitType::kComputeAddUpd, key);
    }

    key = metric.GenerateStatKey(EmitType::kComputeAddGen);
    metric.Emit(EmitType::kComputeAddGen, key);
    FNIAGKA::AddUser(max_group_size,
                     omega,
                     users[j],
                     std::make_shared<GroupInfo>(group_info),
                     uid_to_be_added,
                     encryption_keys[j],
                     decryption_keys[j]);
    metric.Emit(EmitType::kComputeAddGen, key);

    group_info.Occupy(uid_to_be_added);
    TestEncapDecap(max_group_size,
                   omega,
                   std::make_shared<GroupInfo>(group_info),
                   users,
                   encryption_keys[0],
                   decryption_keys,
                   group_info.GetMembers());

    INFO("============ Test Remove ============");
    auto uid_to_be_removed = users[users.size() - 1]->uid_;
    for (int j = 0; j < users.size() - 1; j++)
    {
        key = metric.GenerateStatKey(EmitType::kComputeRemove);
        metric.Emit(EmitType::kComputeRemove, key);
        FNIAGKA::RemoveUser(max_group_size,
                            omega,
                            users[j],
                            std::make_shared<GroupInfo>(group_info),
                            uid_to_be_removed,
                            encryption_keys[j],
                            decryption_keys[j]);

        metric.Emit(EmitType::kComputeRemove, key);
    }
    group_info.Vacate(uid_to_be_removed);
    TestEncapDecap(max_group_size,
                   omega,
                   std::make_shared<GroupInfo>(group_info),
                   users,
                   encryption_keys[0],
                   decryption_keys,
                   group_info.GetMembers());

    INFO("============ Test Split ============");
    std::unordered_map<int64_t, std::shared_ptr<FNIAGKA::User>> users_for_split_merge;
    std::unordered_map<int64_t, EncryptionKey> encryption_keys_for_split_merge;
    std::unordered_map<int64_t, DecryptionKey> decryption_keys_for_split_merge;
    for (int i = 0; i < users.size() - 1; i++)
    {
        users_for_split_merge[users[i]->uid_] = users[i];
        encryption_keys_for_split_merge[users[i]->uid_] = encryption_keys[i];
        decryption_keys_for_split_merge[users[i]->uid_] = decryption_keys[i];
    }

    TestSplitMergeGroup(max_group_size, omega, 2, users_for_split_merge, encryption_keys_for_split_merge, decryption_keys_for_split_merge);
    TestSplitMergeGroup(max_group_size, omega, 3, users_for_split_merge, encryption_keys_for_split_merge, decryption_keys_for_split_merge);
    TestSplitMergeGroup(max_group_size, omega, 4, users_for_split_merge, encryption_keys_for_split_merge, decryption_keys_for_split_merge);
    TestSplitMergeGroup(max_group_size, omega, 5, users_for_split_merge, encryption_keys_for_split_merge, decryption_keys_for_split_merge);

    metric.Summarize();

    // TestSAAGKA(security_level, pp);
}
