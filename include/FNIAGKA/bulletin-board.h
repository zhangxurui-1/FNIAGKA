#pragma once

#include "proto.h"
#include <memory>

class BulletinBoard {
  public:
    BulletinBoard() = default;
    ~BulletinBoard() = default;

    inline void SetPublicParameter(std::shared_ptr<PublicParameter> pp) {
        pp_ = pp;
    }
    inline void SetFullParameter(std::shared_ptr<FullParameter> omega) {
        omega_ = omega;
    }

    inline std::shared_ptr<PublicParameter> GetPublicParameter() {
        return pp_;
    }
    inline std::shared_ptr<FullParameter> GetFullParameter() {
        return omega_;
    }

  private:
    std::shared_ptr<PublicParameter> pp_;
    std::shared_ptr<FullParameter> omega_;
};