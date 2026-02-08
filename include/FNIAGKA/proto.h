#pragma once

#include <cstdint>
#define MR_PAIRING_SSP

#include "pairing_1.h"
#include <memory>
#include <vector>

struct PublicParameter {
    std::shared_ptr<PFC> pfc_;
    G1 g0_; // generator of G1
    int max_group_size_;
    G1 h_;
    std::vector<G1> g_;

    PublicParameter(int security_level) : pfc_(std::make_shared<PFC>(security_level)) {
    }
};

struct FullParameter {
    std::shared_ptr<PublicParameter> pp_;
    std::vector<std::vector<G1>> ujk_;
    std::vector<G1> u_;
    std::vector<G1> d_;
    G1 a;
    G1 b;
};

struct PNPublicKey {
    G1 p_;
    std::vector<std::vector<G1>> pjk_;
    std::vector<G1> pj0_;
};

struct PNPrivateKey {
};

struct UserPublicKey {
};

struct UserPrivateKey {
};

class FNIAGKA {
  public:
    class User {
        int64_t uid_;
        std::shared_ptr<UserPublicKey> upk_;
        std::shared_ptr<UserPrivateKey> usk_;

        static int64_t id_counter_;
        User() : uid_(id_counter_++) {}
    };

    FNIAGKA() = delete;
    ~FNIAGKA() = delete;
    static std::shared_ptr<PublicParameter> Setup(int security_level, int max_group_size);
    static std::shared_ptr<PNPublicKey> PNGen(std::shared_ptr<PublicParameter> pp);
    static std::shared_ptr<FullParameter> Negotiate(std::shared_ptr<PublicParameter> pp, std::vector<std::shared_ptr<PNPublicKey>> pn_public_keys);
    static std::shared_ptr<User> UserGen(std::shared_ptr<PublicParameter> pp);

    static bool IsValid(std::shared_ptr<PublicParameter> pp, std::shared_ptr<PNPublicKey> pn_pk);

  private:
};
