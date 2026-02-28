#include "FNIAGKA/proto.h"
#include "log.h"

#include <memory>
#include <unordered_map>

class PKI
{
    std::unordered_map<int64_t, std::shared_ptr<UserPublicKey>> user_public_keys_;

  public:
    inline std::shared_ptr<UserPublicKey> GetUserPublicKey(int64_t uid)
    {
        if (user_public_keys_.find(uid) != user_public_keys_.end())
        {
            return user_public_keys_[uid];
        }
        else
        {
            return nullptr;
        }
    }

    inline bool UserRegister(int64_t uid, std::shared_ptr<UserPublicKey> upk)
    {
        if (user_public_keys_.find(uid) != user_public_keys_.end())
        {
            return false;
        }
        user_public_keys_[uid] = upk;
        return true;
    }
};
