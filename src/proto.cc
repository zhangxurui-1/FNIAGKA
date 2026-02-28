#include "FNIAGKA/proto.h"

#include "FNIAGKA/pki.h"
#include "log.h"
#include "pairing_1.h"
#include "singleton.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

int64_t FNIAGKA::User::id_counter_ = 0;
int64_t GroupInfo::id_counter_ = 0;

std::shared_ptr<PublicParameter>
FNIAGKA::Setup(int security_level, int max_group_size)
{
    auto pp = std::make_shared<PublicParameter>(security_level);
    auto pfc = pp->pfc_;

    pp->max_group_size_ = max_group_size;
    pfc->random(pp->g0_);
    pfc->random(pp->h_);
    pp->g_ = std::vector<G1>(max_group_size);
    for (int i = 0; i < max_group_size; i++)
    {
        pfc->random(pp->g_[i]);
    }

    INFO("Setup done");
    return pp;
}

std::shared_ptr<PNPublicKey>
FNIAGKA::PNGen(std::shared_ptr<PublicParameter> pp)
{
    auto pn_pk = std::make_shared<PNPublicKey>();

    auto pfc = pp->pfc_;
    Big nu;
    std::vector<Big> mu(pp->max_group_size_);
    pfc->random(nu);
    for (int i = 0; i < pp->max_group_size_; i++)
    {
        pfc->random(mu[i]);
    }

    pn_pk->p_ = pfc->mult(pp->g0_, nu);
    pn_pk->pjk_ =
        std::vector<std::vector<G1>>(pp->max_group_size_, std::vector<G1>(pp->max_group_size_));
    pn_pk->pj0_ = std::vector<G1>(pp->max_group_size_);
    for (int i = 0; i < pp->max_group_size_; i++)
    {
        pn_pk->pj0_[i] = pfc->mult(pp->g0_, mu[i]);
        for (int j = 0; j < pp->max_group_size_; j++)
        {
            if (i == j)
            {
                continue;
            }

            if (i < pp->max_group_size_ - 1)
            {
                pn_pk->pjk_[i][j] = pfc->mult(pp->g_[j], mu[i]);
            }
            else
            {
                pn_pk->pjk_[i][j] = pfc->mult(pp->g_[j], mu[i]) + pfc->mult(pp->h_, nu);
            }
        }
    }

    return pn_pk;
}

std::shared_ptr<FullParameter>
FNIAGKA::Negotiate(std::shared_ptr<PublicParameter> pp,
                   std::vector<std::shared_ptr<PNPublicKey>> pn_public_keys)
{
    // verify the validity of pn_public_keys
    for (int i = 0; i < pn_public_keys.size(); i++)
    {
        if (!IsValid(pp, pn_public_keys[i]))
        {
            WARN("PN public key verification failed");
            return nullptr;
        }
    }

    // negotiate
    auto omega = std::make_shared<FullParameter>();
    omega->pp_ = pp;
    omega->u_.resize(pp->max_group_size_);
    omega->ujk_ =
        std::vector<std::vector<G1>>(pp->max_group_size_, std::vector<G1>(pp->max_group_size_));
    omega->d_.resize(pp->max_group_size_);

    for (int k = 0; k < pn_public_keys.size(); k++)
    {
        if (k == 0)
        {
            omega->b = pn_public_keys[k]->p_;
        }
        else
        {
            omega->b = pn_public_keys[k]->p_ + omega->b;
        }
        for (int i = 0; i < pp->max_group_size_; i++)
        {
            if (k == 0)
            {
                omega->u_[i] = pn_public_keys[k]->pj0_[i];
            }
            else
            {
                omega->u_[i] = pn_public_keys[k]->pj0_[i] + omega->u_[i];
            }
            for (int j = 0; j < pp->max_group_size_; j++)
            {
                if (i == j)
                {
                    continue;
                }

                if (k == 0)
                {
                    omega->ujk_[i][j] = pn_public_keys[k]->pjk_[i][j];
                }
                else
                {
                    omega->ujk_[i][j] = pn_public_keys[k]->pjk_[i][j] + omega->ujk_[i][j];
                }
            }
        }
    }

    std::vector<bool> flags(pp->max_group_size_, false);
    for (int i = 0; i < pp->max_group_size_; i++)
    {
        if (i == 0)
        {
            omega->a = omega->u_[i];
        }
        else
        {
            omega->a = omega->a + omega->u_[i];
        }
        for (int j = 0; j < pp->max_group_size_; j++)
        {
            if (i == j)
            {
                continue;
            }
            if (!flags[j])
            {
                omega->d_[j] = omega->ujk_[i][j];
                flags[j] = true;
            }
            else
            {
                omega->d_[j] = omega->ujk_[i][j] + omega->d_[j];
            }
        }
    }

    return omega;
}

bool
FNIAGKA::IsValid(std::shared_ptr<PublicParameter> pp, std::shared_ptr<PNPublicKey> pn_pk)
{
    for (int i = 0; i < pp->max_group_size_; i++)
    {
        for (int j = 0; j < pp->max_group_size_; j++)
        {
            if (i == j)
            {
                continue;
            }

            GT lhs = pp->pfc_->pairing(pp->g0_, pn_pk->pjk_[i][j]);
            GT rhs = pp->pfc_->pairing(pp->g_[j], pn_pk->pj0_[i]);
            if (i == pp->max_group_size_ - 1)
            {
                rhs = rhs * pp->pfc_->pairing(pp->h_, pn_pk->p_);
            }

            if (lhs != rhs)
            {
                return false;
            }
        }
    }

    return true;
}

std::shared_ptr<FNIAGKA::User>
FNIAGKA::UserGen(std::shared_ptr<PublicParameter> pp)
{
    auto user = std::make_shared<User>();
    int n = pp->max_group_size_;
    std::vector<Big> mu(n);
    std::vector<Big> nu(n);
    user->upk_->uj_.resize(n);
    user->upk_->wj_.resize(n);
    user->upk_->ujk_.resize(n, std::vector<G1>(n));
    user->usk_->ujj_.resize(n);

    for (int i = 0; i < n; i++)
    {
        pp->pfc_->random(mu[i]);
        pp->pfc_->random(nu[i]);

        user->upk_->uj_[i] = pp->pfc_->mult(pp->g0_, mu[i]);
        user->upk_->wj_[i] = pp->pfc_->mult(pp->g0_, nu[i]);
        for (int j = 0; j < n; j++)
        {
            if (i != j)
            {
                user->upk_->ujk_[i][j] =
                    pp->pfc_->mult(pp->g_[j], mu[i]) + pp->pfc_->mult(pp->h_, nu[i]);
            }
            else
            {
                user->usk_->ujj_[i] =
                    pp->pfc_->mult(pp->g_[j], mu[i]) + pp->pfc_->mult(pp->h_, nu[i]);
            }
        }
    }

    // verify
    // for (int i = 0; i < n; i++) {
    //     for (int j = 0; j < n; j++) {
    //         G1 e;
    //         if (i == j) {
    //             e = user->usk_->ujj_[i];
    //         } else {
    //             e = user->upk_->ujk_[i][j];
    //         }

    //         GT lhs = pp->pfc_->pairing(pp->g0_, e);
    //         GT rhs = pp->pfc_->pairing(pp->g_[j], user->upk_->uj_[i]) * pp->pfc_->pairing(pp->h_,
    //         user->upk_->wj_[i]); if (lhs != rhs) {
    //             WARN("User public key verification failed");
    //         }
    //     }
    // }

    return user;
}

void
FNIAGKA::Agree(int64_t eta,
               std::shared_ptr<FullParameter> omega,
               std::shared_ptr<User> user,
               std::shared_ptr<GroupInfo> group_info,
               EncryptionKey& ek,
               DecryptionKey& dk)
{
    auto& pki = Singleton<PKI>::GetInstance();

    // initialize
    bool is_member = false;
    ek.group_info_ = *group_info;
    ek.a_ = omega->a;
    ek.b_ = omega->b;
    dk.group_info_ = *group_info;

    for (int i = 0; i < group_info->membership_.size(); i++)
    {
        if (group_info->membership_[i] == user->uid_)
        {
            dk.dk_ = omega->d_[i];
            dk.slot_ = i;
            is_member = true;
            break;
        }
    }
    if (!is_member)
    {
        FATAL_ERROR("User " << user->uid_ << " is not a member of group " << group_info->gid_);
    }

    for (int i = 0; i < group_info->membership_.size(); i++)
    {
        if (group_info->membership_[i] == -1)
        {
            continue;
        }

        auto upk = pki.GetUserPublicKey(group_info->membership_[i]);
        if (!upk)
        {
            FATAL_ERROR("upk for user " << group_info->membership_[i] << " not found");
        }
        ek.a_ = ek.a_ + upk->uj_[i] + (-omega->u_[i]);
        if (i != omega->pp_->max_group_size_ - 1)
        {
            ek.b_ = ek.b_ + upk->wj_[i];
        }
        else
        {
            ek.b_ = ek.b_ + upk->wj_[i] + (-omega->b);
        }

        if (i != dk.slot_)
        {
            dk.dk_ = dk.dk_ + upk->ujk_[i][dk.slot_] + (-omega->ujk_[i][dk.slot_]);
        }
        else
        {
            dk.dk_ = dk.dk_ + user->usk_->ujj_[i];
        }
    }

    // verify
    // GT lhs = omega->pp_->pfc_->pairing(omega->pp_->g0_, dk.dk_);
    // GT rhs = omega->pp_->pfc_->pairing(omega->pp_->g_[dk.slot_], ek.a_) *
    //          omega->pp_->pfc_->pairing(omega->pp_->h_, ek.b_);
    // if (lhs != rhs)
    // {
    //     FATAL_ERROR("Key agreement failed for user " << user->uid_);
    // }
}

std::pair<GroupKey, KeyEncapsulation>
FNIAGKA::Encap(int64_t eta,
               std::shared_ptr<FullParameter> omega,
               std::shared_ptr<GroupInfo> group_info,
               const std::vector<int64_t>& receiver_uids,
               const EncryptionKey& ek)
{
    std::unordered_map<int, int64_t> no_receiver;
    for (int i = 0; i < group_info->membership_.size(); i++)
    {
        if (group_info->membership_[i] == -1)
        {
            continue;
        }
        no_receiver[group_info->membership_[i]] = i;
    }

    for (int uid : receiver_uids)
    {
        auto it = no_receiver.find(uid);
        if (it == no_receiver.end())
        {
            FATAL_ERROR("Receiver " << uid << " is not a member of group " << group_info->gid_);
        }
        no_receiver.erase(it);
    }

    G1 a = ek.a_;
    G1 b = ek.b_;
    auto& pki = Singleton<PKI>::GetInstance();
    for (auto& pair : no_receiver)
    {
        auto uid = pair.first;
        auto slot = pair.second;

        a = a + omega->u_[slot];
        if (slot == omega->pp_->max_group_size_ - 1)
        {
            b = b + omega->b;
        }
    }

    Big r;
    omega->pp_->pfc_->random(r);
    G1 tmp = omega->pp_->pfc_->mult(omega->pp_->h_, r);
    GroupKey group_key = omega->pp_->pfc_->pairing(tmp, b);
    KeyEncapsulation ct;
    ct.c1_ = omega->pp_->pfc_->mult(omega->pp_->g0_, r);
    ct.c2_ = omega->pp_->pfc_->mult(a, r);

    return std::make_pair(group_key, ct);
}

std::pair<GroupKey, bool>
FNIAGKA::Decap(int64_t eta,
               std::shared_ptr<FullParameter> omega,
               std::shared_ptr<GroupInfo> group_info,
               std::shared_ptr<User> user,
               const std::vector<int64_t>& receiver_uids,
               const DecryptionKey& dk,
               const KeyEncapsulation& ct)
{
    std::unordered_map<int, int64_t> no_receiver;
    for (int i = 0; i < group_info->membership_.size(); i++)
    {
        if (group_info->membership_[i] == -1)
        {
            continue;
        }
        no_receiver[group_info->membership_[i]] = i;
    }

    for (int uid : receiver_uids)
    {
        auto it = no_receiver.find(uid);
        if (it == no_receiver.end())
        {
            FATAL_ERROR("Receiver " << uid << " is not a member of group " << group_info->gid_);
        }
        no_receiver.erase(it);
    }

    if (no_receiver.find(user->uid_) != no_receiver.end())
    {
        FATAL_ERROR("User " << user->uid_ << " is not a receiver");
    }

    G1 d = dk.dk_;
    auto& pki = Singleton<PKI>::GetInstance();
    for (auto& pair : no_receiver)
    {
        auto uid = pair.first;
        auto slot = pair.second;

        d = d + omega->ujk_[slot][dk.slot_];
    }

    GT tmp1 = omega->pp_->pfc_->pairing(ct.c1_, d);
    GT tmp2 = omega->pp_->pfc_->pairing(-ct.c2_, omega->pp_->g_[dk.slot_]);
    GroupKey group_key = tmp1 * tmp2;
    return std::make_pair(group_key, true);
}

void
FNIAGKA::AddUser(int64_t eta,
                 std::shared_ptr<FullParameter> omega,
                 std::shared_ptr<User> user,
                 std::shared_ptr<GroupInfo> cur_group_info,
                 int64_t new_user_uid,
                 EncryptionKey& ek,
                 DecryptionKey& dk)
{
    GroupInfo new_group_info = *cur_group_info;
    int new_user_slot = -1;
    for (int i = 0; i < new_group_info.membership_.size(); i++)
    {
        if (new_group_info.membership_[i] == -1)
        {
            new_user_slot = i;
            new_group_info.membership_[i] = new_user_uid;
            break;
        }
    }

    if (user->uid_ == new_user_uid)
    {
        return Agree(eta, omega, user, std::make_shared<GroupInfo>(new_group_info), ek, dk);
    }

    auto upk = Singleton<PKI>::GetInstance().GetUserPublicKey(new_user_uid);
    if (!upk)
    {
        FATAL_ERROR("upk not found for new user " << new_user_uid);
    }

    ek.a_ = ek.a_ + upk->uj_[new_user_slot] + (-omega->u_[new_user_slot]);
    if (new_user_slot != omega->pp_->max_group_size_ - 1)
    {
        ek.b_ = ek.b_ + upk->wj_[new_user_slot];
    }
    else
    {
        ek.b_ = ek.b_ + upk->wj_[new_user_slot] + (-omega->b);
    }
    ek.group_info_ = new_group_info;

    dk.dk_ = dk.dk_ + upk->ujk_[new_user_slot][dk.slot_] + (-omega->ujk_[new_user_slot][dk.slot_]);
    dk.group_info_ = new_group_info;
}

void
FNIAGKA::RemoveUser(int64_t eta,
                    std::shared_ptr<FullParameter> omega,
                    std::shared_ptr<User> user,
                    std::shared_ptr<GroupInfo> cur_group_info,
                    int64_t removed_user_uid,
                    EncryptionKey& ek,
                    DecryptionKey& dk)
{
    GroupInfo new_group_info = *cur_group_info;
    int removed_user_slot = -1;
    for (int i = 0; i < new_group_info.membership_.size(); i++)
    {
        if (new_group_info.membership_[i] == removed_user_uid)
        {
            removed_user_slot = i;
            new_group_info.membership_[i] = -1;
            break;
        }
    }

    auto upk = Singleton<PKI>::GetInstance().GetUserPublicKey(removed_user_uid);
    if (!upk)
    {
        FATAL_ERROR("upk not found for removed user " << removed_user_uid);
    }

    ek.a_ = ek.a_ + omega->u_[removed_user_slot] + (-upk->uj_[removed_user_slot]);
    if (removed_user_slot != omega->pp_->max_group_size_ - 1)
    {
        ek.b_ = ek.b_ + (-upk->wj_[removed_user_slot]);
    }
    else
    {
        ek.b_ = ek.b_ + (-upk->wj_[removed_user_slot]) + omega->b;
    }
    ek.group_info_ = new_group_info;

    dk.dk_ = dk.dk_ + omega->ujk_[removed_user_slot][dk.slot_] +
             (-upk->ujk_[removed_user_slot][dk.slot_]);
    dk.group_info_ = new_group_info;
}

void
FNIAGKA::SplitGroup(int64_t eta,
                    std::shared_ptr<FullParameter> omega,
                    std::shared_ptr<User> user,
                    std::shared_ptr<GroupInfo> cur_group_info,
                    std::vector<GroupInfo>& target_group_infos,
                    EncryptionKey& ek,
                    DecryptionKey& dk)
{
    std::unordered_map<int64_t, int64_t> target_group_uid_to_slot;
    bool found_user = false;
    for (int i = 0; i < target_group_infos.size(); i++)
    {
        target_group_uid_to_slot.clear();
        for (int j = 0; j < target_group_infos[i].membership_.size(); j++)
        {
            int64_t uid = target_group_infos[i].membership_[j];
            if (uid != -1)
            {
                target_group_uid_to_slot[uid] = j;
            }
            if (uid == user->uid_)
            {
                found_user = true;
                ek.group_info_ = target_group_infos[i];
                dk.group_info_ = target_group_infos[i];
            }
        }
        if (found_user)
        {
            break;
        }
    }

    for (int i = 0; i < cur_group_info->membership_.size(); i++)
    {
        auto uid = cur_group_info->membership_[i];
        if (uid == -1 || target_group_uid_to_slot.find(uid) != target_group_uid_to_slot.end())
        {
            continue;
        }

        auto upk = Singleton<PKI>::GetInstance().GetUserPublicKey(uid);
        ek.a_ = ek.a_ + omega->u_[i] + (-upk->uj_[i]);
        if (i != omega->pp_->max_group_size_ - 1)
        {
            ek.b_ = ek.b_ + (-upk->wj_[i]);
        }
        else
        {
            ek.b_ = ek.b_ + (-upk->wj_[i]) + omega->b;
        }

        dk.dk_ = dk.dk_ + omega->ujk_[i][dk.slot_] + (-upk->ujk_[i][dk.slot_]);
    }
}

std::vector<EncryptionKey>
FNIAGKA::MergeGroup(int64_t eta,
                    std::shared_ptr<FullParameter> omega,
                    std::shared_ptr<User> user,
                    const std::vector<EncryptionKey>& eks,
                    DecryptionKey& dk,
                    MergeMode mode)
{
    if (mode == MergeMode::kStandard)
    {
        return MergeGroupStandard(eta, omega, user, eks, dk);
    }
    else if (mode == MergeMode::kExtended)
    {
        return MergeGroupExtended(eta, omega, user, eks, dk);
    }
    FATAL_ERROR("merge mode not supported");
}

std::vector<EncryptionKey>
FNIAGKA::MergeGroupStandard(int64_t eta,
                            std::shared_ptr<FullParameter> omega,
                            std::shared_ptr<User> user,
                            const std::vector<EncryptionKey>& eks,
                            DecryptionKey& dk)
{
    std::vector<EncryptionKey> new_eks(eks);
    sort(new_eks.begin(), new_eks.end(), [](const EncryptionKey& a, const EncryptionKey& b) {
        return a.group_info_.member_num_ > b.group_info_.member_num_;
    });

    int j = new_eks.size() - 1;
    auto& pki = Singleton<PKI>::GetInstance();
    while (j > 0)
    {
        GroupInfo group_info_from = new_eks[j].group_info_;
        if (group_info_from.member_num_ == 0)
        {
            new_eks.erase(new_eks.begin() + j);
            j--;
            continue;
        }

        bool abort = false;
        for (auto pair : group_info_from.uid_to_slot_)
        {
            auto uid = pair.first;
            auto slot = pair.second;
            bool migrate_success = false;
            for (int k = 0; k < j; k++)
            {
                GroupInfo& group_info_to = new_eks[k].group_info_;
                if (group_info_to.TryOccupyWithSlot(uid, slot))
                {
                    migrate_success = true;
                    auto upk = pki.GetUserPublicKey(uid);
                    if (!upk)
                    {
                        FATAL_ERROR("upk not found for user " << uid);
                    }

                    // step 1: update new ek
                    new_eks[k].a_ = new_eks[k].a_ + upk->uj_[slot] + (-omega->u_[slot]);
                    if (slot != omega->pp_->max_group_size_ - 1)
                    {
                        new_eks[k].b_ = new_eks[k].b_ + upk->wj_[slot];
                    }
                    else
                    {
                        new_eks[k].b_ = new_eks[k].b_ + upk->wj_[slot] + (-omega->b);
                    }

                    // step 2: update old ek
                    group_info_from.Vacate(uid);

                    new_eks[j].a_ = new_eks[j].a_ + omega->u_[slot] + (-upk->uj_[slot]);
                    if (slot != omega->pp_->max_group_size_ - 1)
                    {
                        new_eks[j].b_ = new_eks[j].b_ + (-upk->wj_[slot]);
                    }
                    else
                    {
                        new_eks[j].b_ = new_eks[j].b_ + (-upk->wj_[slot]) + omega->b;
                    }
                    break;
                }
            }

            if (!migrate_success)
            {
                abort = true;
                break;
            }
        }
        if (abort)
        {
            break;
        }
    }

    // compute dk
    for (int i = 0; i < new_eks.size(); i++)
    {
        auto it = new_eks[i].group_info_.uid_to_slot_.find(user->uid_);
        if (it != new_eks[i].group_info_.uid_to_slot_.end())
        {
            dk.dk_ = omega->d_[it->second];
            dk.group_info_ = new_eks[i].group_info_;
            dk.slot_ = it->second;

            std::vector<int>& membership = new_eks[i].group_info_.membership_;
            for (int j = 0; j < membership.size(); j++)
            {
                if (membership[j] == -1)
                {
                    continue;
                }
                auto upk = pki.GetUserPublicKey(membership[j]);
                if (j != dk.slot_)
                {
                    dk.dk_ = dk.dk_ + upk->ujk_[j][dk.slot_] + (-omega->ujk_[j][dk.slot_]);
                }
                else
                {
                    dk.dk_ = dk.dk_ + user->usk_->ujj_[j];
                }
            }
        }
    }

    return new_eks;
}

std::vector<EncryptionKey>
FNIAGKA::MergeGroupExtended(int64_t eta,
                            std::shared_ptr<FullParameter> omega,
                            std::shared_ptr<User> user,
                            const std::vector<EncryptionKey>& eks,
                            DecryptionKey& dk)
{
    std::vector<EncryptionKey> new_eks(eks);
    sort(new_eks.begin(), new_eks.end(), [](const EncryptionKey& a, const EncryptionKey& b) {
        return a.group_info_.member_num_ > b.group_info_.member_num_;
    });

    int i = 0, j = new_eks.size() - 1;
    auto& pki = Singleton<PKI>::GetInstance();

    while (i < j)
    {
        if (new_eks[i].group_info_.member_num_ == eta)
        {
            i++;
            continue;
        }
        if (new_eks[j].group_info_.member_num_ == 0)
        {
            new_eks.erase(new_eks.begin() + j);
            j--;
            continue;
        }

        GroupInfo& group_info_from = new_eks[j].group_info_;

        std::vector<int64_t> uids;
        uids.reserve(group_info_from.uid_to_slot_.size());
        for (const auto& kv : group_info_from.uid_to_slot_)
        {
            uids.push_back(kv.first);
        }

        for (auto uid : uids)
        {
            auto slot_from = group_info_from.uid_to_slot_.at(uid);
            auto slot_to = new_eks[i].group_info_.Occupy(uid);
            if (slot_to == -1)
            {
                break;
            }

            auto upk = pki.GetUserPublicKey(uid);
            if (!upk)
            {
                FATAL_ERROR("upk not found");
            }

            // step 1: update new ek
            new_eks[i].a_ = new_eks[i].a_ + upk->uj_[slot_to] + (-omega->u_[slot_to]);
            if (slot_to != omega->pp_->max_group_size_ - 1)
            {
                new_eks[i].b_ = new_eks[i].b_ + upk->wj_[slot_to];
            }
            else
            {
                new_eks[i].b_ = new_eks[i].b_ + upk->wj_[slot_to] + (-omega->b);
            }

            // step 2: update old ek (现在删不会影响 uids 的遍历)
            group_info_from.Vacate(uid);

            new_eks[j].a_ = new_eks[j].a_ + omega->u_[slot_from] + (-upk->uj_[slot_from]);
            if (slot_from != omega->pp_->max_group_size_ - 1)
            {
                new_eks[j].b_ = new_eks[j].b_ + (-upk->wj_[slot_from]);
            }
            else
            {
                new_eks[j].b_ = new_eks[j].b_ + (-upk->wj_[slot_from]) + omega->b;
            }
        }
    }

    // compute dk
    for (int i = 0; i < new_eks.size(); i++)
    {
        auto it = new_eks[i].group_info_.uid_to_slot_.find(user->uid_);
        if (it != new_eks[i].group_info_.uid_to_slot_.end())
        {
            dk.dk_ = omega->d_[it->second];
            dk.group_info_ = new_eks[i].group_info_;
            dk.slot_ = it->second;

            std::vector<int>& membership = new_eks[i].group_info_.membership_;
            for (int j = 0; j < membership.size(); j++)
            {
                if (membership[j] == -1)
                {
                    continue;
                }
                auto upk = pki.GetUserPublicKey(membership[j]);
                if (j != dk.slot_)
                {
                    dk.dk_ = dk.dk_ + upk->ujk_[j][dk.slot_] + (-omega->ujk_[j][dk.slot_]);
                }
                else
                {
                    dk.dk_ = dk.dk_ + user->usk_->ujj_[j];
                }
            }
        }
    }

    if (new_eks[j].group_info_.member_num_ == 0)
    {
        new_eks.erase(new_eks.begin() + j);
    }

    return new_eks;
}
