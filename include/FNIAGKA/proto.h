#pragma once

#include <cstdint>
#include <map>
#include <unordered_map>
#include <utility>

// Use Type-3 (asymmetric) pairing on a BLS curve supported by this MIRACL tree.
// NOTE: In this MIRACL version, `MR_PAIRING_BLS` corresponds to a k=24 BLS curve (BLS24-like),
// not the modern BLS12-381 curve.
// MIRACL pairing API: GT = pairing(G2, G1)
#define MR_PAIRING_BLS

#include "pairing_3.h"
#include "utils.h"

#include <memory>
#include <vector>

extern const int kCachedPoolSize;

struct PublicParameter
{
    std::shared_ptr<PFC> pfc_;
    G1 g0_; // base generator in G1 (right input of pairing)
    int max_group_size_;
    G2 h_;              // generator in G2 (left input of pairing)
    std::vector<G2> g_; // per-slot generators in G2

    PublicParameter(int security_level)
        : pfc_(std::make_shared<PFC>(security_level))
    {
    }
};

struct FullParameter
{
    std::shared_ptr<PublicParameter> pp_;
    std::vector<std::vector<G2>> ujk_;
    std::vector<G1> u_;
    std::vector<G2> d_;
    G1 a;
    G1 b;
};

struct PNPublicKey
{
    G1 p_;
    std::vector<std::vector<G2>> pjk_;
    std::vector<G1> pj0_;
};

struct PNPrivateKey
{
};

struct UserPublicKey
{
    std::vector<G1> uj_;
    std::vector<G1> wj_;
    std::vector<std::vector<G2>> ujk_;
};

struct UserPrivateKey
{
    std::vector<G2> ujj_;
    std::vector<Big> nuj_;
};

struct GroupInfo
{
    int64_t gid_;
    int64_t eta_;
    int member_num_;
    std::vector<int> membership_;
    std::map<int64_t, int> uid_to_slot_;
    std::vector<int> free_slots_;
    std::vector<int> free_slot_pos_;
    static int64_t id_counter_;

    void AddFreeSlot(int slot)
    {
        if (free_slot_pos_[slot] != -1)
        {
            return;
        }
        free_slot_pos_[slot] = free_slots_.size();
        free_slots_.push_back(slot);
    }

    void RemoveFreeSlot(int slot)
    {
        int pos = free_slot_pos_[slot];
        if (pos == -1)
        {
            return;
        }
        int last_slot = free_slots_.back();
        free_slots_[pos] = last_slot;
        free_slot_pos_[last_slot] = pos;
        free_slots_.pop_back();
        free_slot_pos_[slot] = -1;
    }

    friend std::ostream& operator<<(std::ostream& os, const GroupInfo& group_info)
    {
        os << "GroupInfo { gid: " << group_info.gid_ << ", eta: " << group_info.eta_
           << ", membership: [";
        for (size_t i = 0; i < group_info.membership_.size(); i++)
        {
            os << group_info.membership_[i];
            if (i != group_info.membership_.size() - 1)
            {
                os << ", ";
            }
        }
        os << "], uid_to_slot: { ";
        for (auto& p : group_info.uid_to_slot_)
        {
            os << "[" << p.first << "," << p.second << "] ";
        }
        os << " } }";
        return os;
    }

    static GroupInfo NewGroupInfo(int64_t eta,
                                  const std::vector<int>& membership = std::vector<int>())
    {
        GroupInfo group_info;
        group_info.gid_ = id_counter_++;
        group_info.eta_ = eta;
        if (membership.empty())
        {
            group_info.membership_ = std::vector<int>(eta, -1);
        }
        else
        {
            group_info.membership_ = membership;
        }

        group_info.member_num_ = 0;
        group_info.free_slots_.clear();
        group_info.free_slot_pos_ = std::vector<int>(group_info.membership_.size(), -1);
        for (int i = group_info.membership_.size() - 1; i >= 0; i--)
        {
            if (group_info.membership_[i] == -1)
            {
                group_info.AddFreeSlot(i);
                continue;
            }
            group_info.member_num_++;
            group_info.uid_to_slot_[group_info.membership_[i]] = i;
        }

        return group_info;
    }

    int Occupy(int64_t uid)
    {
        if (uid_to_slot_.find(uid) != uid_to_slot_.end() || member_num_ == eta_ || free_slots_.empty())
        {
            return -1;
        }
        int slot = free_slots_.back();
        RemoveFreeSlot(slot);
        membership_[slot] = uid;
        uid_to_slot_[uid] = slot;
        member_num_++;
        return slot;
    }

    void Vacate(int64_t uid)
    {
        auto it = uid_to_slot_.find(uid);
        if (it == uid_to_slot_.end())
        {
            return;
        }
        int slot = it->second;
        membership_[slot] = -1;
        uid_to_slot_.erase(it);
        member_num_--;
        AddFreeSlot(slot);
    }

    bool TryOccupyWithSlot(int64_t uid, int slot)
    {
        if (uid_to_slot_.find(uid) != uid_to_slot_.end())
        {
            return false;
        }
        if (membership_[slot] != -1)
        {
            return false;
        }
        RemoveFreeSlot(slot);
        membership_[slot] = uid;
        uid_to_slot_[uid] = slot;
        member_num_++;
        return true;
    }

    std::vector<int64_t> GetMembers() const
    {
        std::vector<int64_t> members;
        for (int i = 0; i < membership_.size(); i++)
        {
            if (membership_[i] != -1)
            {
                members.push_back(membership_[i]);
            }
        }
        return members;
    }
};

struct EncryptionKey
{
    G1 a_;
    G1 b_;
    GroupInfo group_info_;

    friend std::ostream& operator<<(std::ostream& os, const EncryptionKey& ek)
    {
        os << "EncryptionKey { a: " << ToString(ek.a_) << ", b: " << ToString(ek.b_)
           << ", group_info: " << ek.group_info_ << " }";
        return os;
    }
};

struct DecryptionKey
{
    G2 dk_;
    GroupInfo group_info_;
    int64_t slot_;

    friend std::ostream& operator<<(std::ostream& os, const DecryptionKey& dk)
    {
        os << "DecryptionKey { dk: " << ToString(dk.dk_) << ", group_info: " << dk.group_info_
           << ", slot: " << dk.slot_ << " }";
        return os;
    }
};

struct KeyEncapsulation
{
    G1 c1_;
    G1 c2_;
};

using GroupKey = GT;

struct KeyUpdMaterial
{
    int version_;
    int ct_num_;
    std::vector<std::vector<uint8_t>> ct_r_;
    std::vector<KeyEncapsulation> ct_key_;
    std::vector<GroupInfo> group_infos_;
};

class FNIAGKA
{
  public:
    enum MergeMode
    {
        kStandard = 0,
        kExtended = 1
    };

    class User
    {
      public:
        int version_;
        int64_t uid_;
        std::shared_ptr<UserPublicKey> upk_;
        std::shared_ptr<UserPrivateKey> usk_;
        std::unordered_map<int, Big> upd_r_; // for key update v2, [version] --> [r]
        // for key update v2, [version] --> [stale usk]
        std::unordered_map<int, UserPrivateKey> stale_usks_;

        static int64_t id_counter_;

        User()
            : version_(0),
              uid_(id_counter_++),
              upk_(std::make_shared<UserPublicKey>()),
              usk_(std::make_shared<UserPrivateKey>())
        {
        }

        // for key update v1
        void UpdateKey(std::shared_ptr<PublicParameter> pp);

        // for key update v2
        inline void CacheR(int version, Big r)
        {
            upd_r_[version] = r;
        }

        inline std::pair<Big, bool> GetR(int version) const
        {
            auto it = upd_r_.find(version);
            if (it == upd_r_.end())
            {
                return std::make_pair(Big(), false);
            }
            return std::make_pair(it->second, true);
        }
    };

    FNIAGKA() = delete;
    ~FNIAGKA() = delete;
    static std::shared_ptr<PublicParameter> Setup(int security_level, int max_group_size);
    static std::shared_ptr<PNPublicKey> PNGen(std::shared_ptr<PublicParameter> pp);
    static std::shared_ptr<FullParameter> Negotiate(
        std::shared_ptr<PublicParameter> pp,
        const std::vector<std::shared_ptr<PNPublicKey>>& pn_public_keys);
    static std::shared_ptr<User> UserGen(std::shared_ptr<PublicParameter> pp);
    static void Agree(int64_t eta,
                      std::shared_ptr<FullParameter> omega,
                      std::shared_ptr<User> user,
                      std::shared_ptr<GroupInfo> group_info,
                      EncryptionKey& ek,
                      DecryptionKey& dk);

    static void AddUser(int64_t eta,
                        std::shared_ptr<FullParameter> omega,
                        std::shared_ptr<User> user,
                        std::shared_ptr<GroupInfo> cur_group_info,
                        int64_t new_user_uid,
                        EncryptionKey& ek,
                        DecryptionKey& dk);

    static void RemoveUser(int64_t eta,
                           std::shared_ptr<FullParameter> omega,
                           std::shared_ptr<User> user,
                           std::shared_ptr<GroupInfo> cur_group_info,
                           int64_t removed_user_uid,
                           EncryptionKey& ek,
                           DecryptionKey& dk);

    static void SplitGroup(int64_t eta,
                           std::shared_ptr<FullParameter> omega,
                           std::shared_ptr<User> user,
                           std::shared_ptr<GroupInfo> cur_group_info,
                           std::vector<GroupInfo>& target_group_infos,
                           EncryptionKey& ek,
                           DecryptionKey& dk);

    static std::vector<EncryptionKey> MergeGroup(int64_t eta,
                                                 std::shared_ptr<FullParameter> omega,
                                                 std::shared_ptr<User> user,
                                                 const std::vector<EncryptionKey>& eks,
                                                 DecryptionKey& dk,
                                                 MergeMode mode);

    static std::pair<GroupKey, KeyEncapsulation> Encap(int64_t eta,
                                                       std::shared_ptr<FullParameter> omega,
                                                       std::shared_ptr<GroupInfo> group_info,
                                                       const std::vector<int64_t>& receiver_uids,
                                                       const EncryptionKey& ek);

    static std::pair<GroupKey, bool> Decap(int64_t eta,
                                           std::shared_ptr<FullParameter> omega,
                                           std::shared_ptr<GroupInfo> group_info,
                                           std::shared_ptr<User> user,
                                           const std::vector<int64_t>& receiver_uids,
                                           const DecryptionKey& dk,
                                           const KeyEncapsulation& ct);

    static void UpdateGroupKey(
        int64_t eta,
        std::shared_ptr<FullParameter> omega,
        std::shared_ptr<User> user,
        int64_t target_uid, // the user who updates its key
        EncryptionKey& cur_ek,
        DecryptionKey& cur_dk,
        UserPublicKey& stale_upk, // the old public key of the target user before update
        std::shared_ptr<UserPrivateKey> stale_usk =
            nullptr // the old private key of the target user before update
    );

    static bool IsValid(std::shared_ptr<PublicParameter> pp, std::shared_ptr<PNPublicKey> pn_pk);

    static KeyUpdMaterial UserKeyUpdLaunch(std::shared_ptr<FullParameter> omega,
                                           int version,
                                           const std::vector<EncryptionKey>& eks);

    static void UserKeyUpd(std::shared_ptr<FullParameter> omege,
                           std::shared_ptr<User> user,
                           const DecryptionKey& dk, // dk is for decrypting kum
                           const KeyUpdMaterial& kum);

    static void GroupKeyUpd(std::shared_ptr<FullParameter> omega,
                            std::shared_ptr<GroupInfo> group_info,
                            std::shared_ptr<User> user,
                            EncryptionKey& ek,
                            DecryptionKey& dk,
                            int version);

  private:
    static std::vector<EncryptionKey> MergeGroupStandard(int64_t eta,
                                                         std::shared_ptr<FullParameter> omega,
                                                         std::shared_ptr<User> user,
                                                         const std::vector<EncryptionKey>& eks,
                                                         DecryptionKey& dk);

    static std::vector<EncryptionKey> MergeGroupExtended(int64_t eta,
                                                         std::shared_ptr<FullParameter> omega,
                                                         std::shared_ptr<User> user,
                                                         const std::vector<EncryptionKey>& eks,
                                                         DecryptionKey& dk);
};
