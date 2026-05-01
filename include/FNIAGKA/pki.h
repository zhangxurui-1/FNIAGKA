#include "FNIAGKA/proto.h"
#include "log.h"

#include <map>
#include <memory>

class PKI
{
    std::map<int64_t, std::shared_ptr<UserPublicKey>> user_public_keys_;

    // [version] --> [uid] --> upk
    std::map<int, std::map<int64_t, std::shared_ptr<UserPublicKey>>> updated_user_public_keys_;

  public:
    inline std::shared_ptr<UserPublicKey> GetUserPublicKey(int64_t uid)
    {
        if (user_public_keys_.find(uid) != user_public_keys_.end())
        {
            return user_public_keys_[uid];
        }
        return nullptr;
    }

    inline bool UserRegister(int64_t uid, std::shared_ptr<UserPublicKey> upk)
    {
        if (user_public_keys_.find(uid) != user_public_keys_.end())
        {
            return false;
        }
        user_public_keys_[uid] = std::make_shared<UserPublicKey>(*upk);
        return true;
    }

    inline bool UserKeyUpdate(int64_t uid, int version, std::shared_ptr<UserPublicKey> upk)
    {
        if (version <= 0)
        {
            return UserRegister(uid, upk);
        }

        if (updated_user_public_keys_.find(version) == updated_user_public_keys_.end())
        {
            updated_user_public_keys_[version] =
                std::map<int64_t, std::shared_ptr<UserPublicKey>>();
        }

        auto& upks = updated_user_public_keys_.find(version)->second;
        if (upks.find(uid) != upks.end())
        {
            return false;
        }
        upks[uid] = std::make_shared<UserPublicKey>(*upk);
        return true;
    }

    inline std::shared_ptr<UserPublicKey> GetUserPublicKey(int64_t uid, int version)
    {
        if (version <= 0)
        {
            return GetUserPublicKey(uid);
        }

        if (updated_user_public_keys_.find(version) == updated_user_public_keys_.end())
        {
            return nullptr;
        }
        auto& upks = updated_user_public_keys_.find(version)->second;
        if (upks.find(uid) != upks.end())
        {
            return upks[uid];
        }

        return nullptr;
    }
};
