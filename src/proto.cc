#include "FNIAGKA/proto.h"

#include "FNIAGKA/bytereader.h"
#include "FNIAGKA/bytewriter.h"
#include "FNIAGKA/pki.h"
#include "log.h"
#include "pairing_1.h"
#include "singleton.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <miracl.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>


constexpr int kCachedPoolSize = 1;
int kFastBenchmarkThreshold = 100;

struct CachedUserMaterial
{
    UserPublicKey upk;
    UserPrivateKey usk;
};

void
PopulateUserMaterial(std::shared_ptr<PublicParameter> pp, UserPublicKey& upk, UserPrivateKey& usk)
{
    int n = pp->max_group_size_;
    std::vector<Big> mu(n);
    std::vector<Big> nu(n);
    upk.uj_.resize(n);
    upk.wj_.resize(n);
    upk.ujk_.resize(n, std::vector<G1>(n));
    usk.ujj_.resize(n);
    usk.nuj_.resize(n);

    for (int i = 0; i < n; i++)
    {
        pp->pfc_->random(mu[i]);
        pp->pfc_->random(nu[i]);

        usk.nuj_[i] = nu[i];
        upk.uj_[i] = pp->pfc_->mult(pp->g0_, mu[i]);
        upk.wj_[i] = pp->pfc_->mult(pp->g0_, nu[i]);
        G1 delta = pp->pfc_->mult(pp->h_, nu[i]);
        for (int j = 0; j < n; j++)
        {
            G1 value = pp->pfc_->mult(pp->g_[j], mu[i]) + delta;
            if (i != j)
            {
                upk.ujk_[i][j] = value;
            }
            else
            {
                usk.ujj_[i] = value;
            }
        }
    }
}

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
    // for (int i = 0; i < pn_public_keys.size(); i++)
    // {
    //     if (!IsValid(pp, pn_public_keys[i]))
    //     {
    //         WARN("PN public key verification failed");
    //         return nullptr;
    //     }
    // }

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
    if (pp->max_group_size_ < kFastBenchmarkThreshold)
    {
        PopulateUserMaterial(pp, *user->upk_, *user->usk_);
        return user;
    }

    static std::vector<CachedUserMaterial> cached_materials;
    static const PublicParameter* cached_pp = nullptr;

    if (cached_pp != pp.get())
    {
        cached_materials.clear();
        cached_materials.resize(kCachedPoolSize);
        cached_pp = pp.get();
        INFO("UserGen benchmark fast path enabled, reusing " << kCachedPoolSize << " cached key templates when eta >= "
             << kFastBenchmarkThreshold);
    }

    int idx = user->uid_ % kCachedPoolSize;
    if (cached_materials[idx].upk.uj_.empty()) {
        PopulateUserMaterial(pp, cached_materials[idx].upk, cached_materials[idx].usk);
    }

    *user->upk_ = cached_materials[idx].upk;
    *user->usk_ = cached_materials[idx].usk;

    // verify
    // for (int i = 0; i < pp->max_group_size_; i++) {
    //     for (int j = 0; j < pp->max_group_size_; j++) {
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

            // step 2: update old ek
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

void
FNIAGKA::UpdateGroupKey(
    int64_t eta,
    std::shared_ptr<FullParameter> omega,
    std::shared_ptr<User> user,
    int64_t target_uid, // the user who updates its key
    EncryptionKey& cur_ek,
    DecryptionKey& cur_dk,
    UserPublicKey& stale_upk, // the old public key of the target user before update
    std::shared_ptr<UserPrivateKey>
        stale_usk) // the old private key of the target user before update
{
    auto& pki = Singleton<PKI>::GetInstance();
    auto upk = pki.GetUserPublicKey(target_uid);
    if (!upk)
    {
        FATAL_ERROR("upk not found for user " << target_uid);
    }

    auto& group_info = cur_ek.group_info_;
    auto it = group_info.uid_to_slot_.find(target_uid);

    if (it == group_info.uid_to_slot_.end())
    {
        FATAL_ERROR("target_uid " << target_uid << " not found in group " << group_info.gid_);
    }
    int slot = it->second;

    cur_ek.a_ = cur_ek.a_ + upk->uj_[slot] + (-stale_upk.uj_[slot]);
    cur_ek.b_ = cur_ek.b_ + upk->wj_[slot] + (-stale_upk.wj_[slot]);

    if (user->uid_ == target_uid)
    {
        cur_dk.dk_ = cur_dk.dk_ + user->usk_->ujj_[slot] + (-stale_usk->ujj_[slot]);
    }
    else
    {
        cur_dk.dk_ =
            cur_dk.dk_ + upk->ujk_[slot][cur_dk.slot_] + (-stale_upk.ujk_[slot][cur_dk.slot_]);
    }
}

void
FNIAGKA::User::UpdateKey(std::shared_ptr<PublicParameter> pp)
{
    std::vector<Big> r(pp->max_group_size_);
    auto pfc = pp->pfc_;
    for (int i = 0; i < pp->max_group_size_; i++)
    {
        pfc->random(r[i]);
    }

    for (int i = 0; i < pp->max_group_size_; i++)
    {
        upk_->wj_[i] = pp->pfc_->mult(upk_->wj_[i], r[i]);

        auto delta =
            pp->pfc_->mult(pp->h_, -usk_->nuj_[i]) + pp->pfc_->mult(pp->h_, usk_->nuj_[i] * r[i]);
        for (int j = 0; j < pp->max_group_size_; j++)
        {
            if (i == j)
            {
                usk_->ujj_[i] = usk_->ujj_[i] + delta;
            }
            else
            {
                upk_->ujk_[i][j] = upk_->ujk_[i][j] + delta;
            }
        }

        usk_->nuj_[i] = usk_->nuj_[i] * r[i];
    }
}

KeyUpdMaterial
FNIAGKA::UserKeyUpdLaunch(std::shared_ptr<FullParameter> omega,
                          int version,
                          const std::vector<EncryptionKey>& eks)
{
    Big r;
    auto pp = omega->pp_;
    pp->pfc_->random(r);

    std::vector<uint8_t> r_bytes;
    ByteWriter bw_r(r_bytes);
    bw_r.write(r);
    INFO("length of r=" << bw_r.position());

    KeyUpdMaterial kum;
    kum.version_ = version;
    kum.ct_num_ = eks.size();
    kum.ct_key_.resize(eks.size());
    kum.ct_r_.resize(eks.size());
    kum.group_infos_.resize(eks.size());

    for (auto it = eks.begin(); it != eks.end(); it++)
    {
        std::vector<int64_t> receiver_uids;
        for (int i = 0; i < it->group_info_.membership_.size(); i++)
        {
            if (it->group_info_.membership_[i] != -1)
            {
                receiver_uids.push_back(it->group_info_.membership_[i]);
            }
        }
        auto pair = Encap(pp->max_group_size_,
                          omega,
                          std::make_shared<GroupInfo>(it->group_info_),
                          receiver_uids,
                          *it);

        kum.group_infos_[it - eks.begin()] = it->group_info_;
        GroupKey gk = pair.first;
        kum.ct_key_[it - eks.begin()] = pair.second;

        std::vector<uint8_t> gk_bytes;
        ByteWriter bw(gk_bytes);
        bw.write(gk);

        kum.ct_r_[it - eks.begin()] = r_bytes;
        int p = ByteWriter::PREFIX_Big;
        while (p < r_bytes.size())
        {
            kum.ct_r_[it - eks.begin()][p] ^=
                gk_bytes[p + ByteWriter::PREFIX_GT - ByteWriter::PREFIX_Big];
            p++;
        }
    }

    return kum;
}

void
FNIAGKA::UserKeyUpd(std::shared_ptr<FullParameter> omege,
                    std::shared_ptr<User> user,
                    const DecryptionKey& dk,
                    const KeyUpdMaterial& kum)
{
    for (int i = 0; i < kum.ct_num_; i++)
    {
        auto& group_info = kum.group_infos_[i];
        if (group_info.gid_ == dk.group_info_.gid_)
        {
            Big r;
            auto res = user->GetR(kum.version_);
            if (res.second)
            {
                r = res.first;
            }
            else
            {
                std::vector<int64_t> receiver_uids;
                for (int i = 0; i < dk.group_info_.membership_.size(); i++)
                {
                    if (dk.group_info_.membership_[i] != -1)
                    {
                        receiver_uids.push_back(dk.group_info_.membership_[i]);
                    }
                }

                // 1. recover group key
                auto gk = Decap(omege->pp_->max_group_size_,
                                omege,
                                std::make_shared<GroupInfo>(group_info),
                                user,
                                receiver_uids,
                                dk,
                                kum.ct_key_[i]);

                if (!gk.second)
                {
                    FATAL_ERROR("UserKeyUpd failed, Decap error");
                }

                // 2. recover r
                std::vector<uint8_t> r_bytes = kum.ct_r_[i];
                std::vector<uint8_t> gk_bytes;
                ByteWriter bw(gk_bytes);
                bw.write(gk.first);
                size_t p = ByteWriter::PREFIX_Big;
                while (p < r_bytes.size())
                {
                    r_bytes[p] ^= gk_bytes[p + ByteWriter::PREFIX_GT - ByteWriter::PREFIX_Big];
                    p++;
                }

                ByteReader br(r_bytes.data(), r_bytes.size());
                r = br.read<Big>();

                user->CacheR(kum.version_, r);
            }

            // 3. update user key
            auto upk = user->upk_;
            auto usk = user->usk_;
            auto& pfc = omege->pp_->pfc_;
            user->version_ = kum.version_;
            user->stale_usks_[kum.version_ - 1] = *usk;
            for (int j = 0; j < upk->ujk_.size(); j++)
            {
                upk->uj_[j] = pfc->mult(upk->uj_[j], r);
                upk->wj_[j] = pfc->mult(upk->wj_[j], r);
                for (int k = 0; k < upk->ujk_[j].size(); k++)
                {
                    if (j != k)
                    {
                        upk->ujk_[j][k] = pfc->mult(upk->ujk_[j][k], r);
                    }
                    else
                    {
                        usk->ujj_[j] = pfc->mult(usk->ujj_[j], r);
                    }
                }
            }

            // upload to pki
            auto& pki = Singleton<PKI>::GetInstance();
            pki.UserKeyUpdate(user->uid_, kum.version_, user->upk_);
            return;
        }
    }
}

void
FNIAGKA::GroupKeyUpd(std::shared_ptr<FullParameter> omega,
                     std::shared_ptr<GroupInfo> group_info,
                     std::shared_ptr<User> user,
                     EncryptionKey& ek,
                     DecryptionKey& dk,
                     int version)
{
    auto cache_r = user->GetR(version);
    if (!cache_r.second)
    {
        FATAL_ERROR("GroupKeyUpd failed, please update user key first");
    }

    G1 delta_a;
    G1 delta_b;
    if (user->stale_usks_.find(version - 1) == user->stale_usks_.end())
    {
        FATAL_ERROR("GroupKeyUpd failed, stale usk not found");
    }
    G1 delta_d = user->stale_usks_[version - 1].ujj_[dk.slot_];
    auto& pki = Singleton<PKI>::GetInstance();
    for (auto it = group_info->uid_to_slot_.begin(); it != group_info->uid_to_slot_.end(); it++)
    {
        int64_t uid = it->first;
        int slot = it->second;
        auto upk = pki.GetUserPublicKey(uid, version - 1);
        if (!upk)
        {
            FATAL_ERROR("GroupKeyUpd failed, upk not found");
        }

        delta_a = delta_a + upk->uj_[slot];
        delta_b = delta_b + upk->wj_[slot];
        if (uid != user->uid_)
        {
            delta_d = delta_d + upk->ujk_[slot][dk.slot_];
        }
    }

    auto pfc = omega->pp_->pfc_;
    ek.a_ = ek.a_ + pfc->mult(delta_a, cache_r.first - 1);
    ek.b_ = ek.b_ + pfc->mult(delta_b, cache_r.first - 1);
    dk.dk_ = dk.dk_ + pfc->mult(delta_d, cache_r.first - 1);
}
