#include "log.h"

#include "FNIAGKA/proto.h"
#include "pairing_1.h"
#include <memory>
#include <vector>

int64_t FNIAGKA::User::id_counter_ = 0;

std::shared_ptr<PublicParameter> FNIAGKA::Setup(int security_level, int max_group_size) {
    auto pp = std::make_shared<PublicParameter>(security_level);
    auto pfc = pp->pfc_;

    pp->max_group_size_ = max_group_size;
    pfc->random(pp->g0_);
    pfc->random(pp->h_);
    pp->g_ = std::vector<G1>(max_group_size);
    for (int i = 0; i < max_group_size; i++) {
        pfc->random(pp->g_[i]);
    }

    INFO("Setup done");
    return pp;
}

std::shared_ptr<PNPublicKey> FNIAGKA::PNGen(std::shared_ptr<PublicParameter> pp) {
    auto pn_pk = std::make_shared<PNPublicKey>();

    auto pfc = pp->pfc_;
    Big nu;
    std::vector<Big> mu(pp->max_group_size_);
    pfc->random(nu);
    for (int i = 0; i < pp->max_group_size_; i++) {
        pfc->random(mu[i]);
    }

    pn_pk->p_ = pfc->mult(pp->g0_, nu);
    pn_pk->pjk_ = std::vector<std::vector<G1>>(pp->max_group_size_, std::vector<G1>(pp->max_group_size_));
    pn_pk->pj0_ = std::vector<G1>(pp->max_group_size_);
    for (int i = 0; i < pp->max_group_size_; i++) {
        pn_pk->pj0_[i] = pfc->mult(pp->g0_, mu[i]);
        for (int j = 0; j < pp->max_group_size_; j++) {
            if (i == j) {
                continue;
            }

            if (i < pp->max_group_size_ - 1) {
                pn_pk->pjk_[i][j] = pfc->mult(pp->g_[j], mu[i]);
            } else {
                pn_pk->pjk_[i][j] = pfc->mult(pp->g_[j], mu[i]) + pfc->mult(pp->h_, nu);
            }
        }
    }

    return pn_pk;
}

std::shared_ptr<FullParameter> FNIAGKA::Negotiate(std::shared_ptr<PublicParameter> pp, std::vector<std::shared_ptr<PNPublicKey>> pn_public_keys) {
    // verify the validity of pn_public_keys
    for (int i = 0; i < pn_public_keys.size(); i++) {
        if (!IsValid(pp, pn_public_keys[i])) {
            WARN("PN public key verification failed");
            return nullptr;
        }
    }

    // negotiate
    auto omega = std::make_shared<FullParameter>();
    omega->pp_ = pp;
    omega->u_.resize(pp->max_group_size_);
    omega->ujk_ = std::vector<std::vector<G1>>(pp->max_group_size_, std::vector<G1>(pp->max_group_size_));
    omega->d_.resize(pp->max_group_size_);

    for (int k = 0; k < pn_public_keys.size(); k++) {
        if (k == 0) {
            omega->b = pn_public_keys[k]->p_;
        } else {
            omega->b = pn_public_keys[k]->p_ + omega->b;
        }
        for (int i = 0; i < pp->max_group_size_; i++) {
            if (k == 0) {
                omega->u_[i] = pn_public_keys[k]->pj0_[i];
            } else {
                omega->u_[i] = pn_public_keys[k]->pj0_[i] + omega->u_[i];
            }
            for (int j = 0; j < pp->max_group_size_; j++) {
                if (i == j) {
                    continue;
                }

                if (k == 0) {
                    omega->ujk_[i][j] = pn_public_keys[k]->pjk_[i][j];
                } else {
                    omega->ujk_[i][j] = pn_public_keys[k]->pjk_[i][j] + omega->ujk_[i][j];
                }
            }
        }
    }

    std::vector<bool> flags(pp->max_group_size_, false);
    for (int i = 0; i < pp->max_group_size_; i++) {
        if (i == 0) {
            omega->a = omega->u_[i];
        } else {
            omega->a = omega->a + omega->u_[i];
        }
        for (int j = 0; j < pp->max_group_size_; j++) {
            if (i == j) {
                continue;
            }
            if (!flags[j]) {
                omega->d_[j] = omega->ujk_[i][j];
                flags[j] = true;
            } else {
                omega->d_[j] = omega->ujk_[i][j] + omega->d_[j];
            }
        }
    }

    return omega;
}

bool FNIAGKA::IsValid(std::shared_ptr<PublicParameter> pp, std::shared_ptr<PNPublicKey> pn_pk) {
    for (int i = 0; i < pp->max_group_size_; i++) {
        for (int j = 0; j < pp->max_group_size_; j++) {
            if (i == j) {
                continue;
            }

            GT lhs = pp->pfc_->pairing(pp->g0_, pn_pk->pjk_[i][j]);
            GT rhs = pp->pfc_->pairing(pp->g_[j], pn_pk->pj0_[i]);
            if (i == pp->max_group_size_ - 1) {
                rhs = rhs * pp->pfc_->pairing(pp->h_, pn_pk->p_);
            }

            if (lhs != rhs) {
                return false;
            }
        }
    }

    return true;
}