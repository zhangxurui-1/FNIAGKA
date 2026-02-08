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

#include "FNIAGKA/proto.h"
#include <ctime>
#include <iostream>
#include <memory>
#include <vector>

//********* CHOOSE JUST ONE OF THESE **********
// #define MR_PAIRING_SS2  // AES-80 or AES-128 security GF(2^m) curve
// #define AES_SECURITY 80 // OR
// #define AES_SECURITY 128

#define MR_PAIRING_SSP  // AES-80 or AES-128 security GF(p) curve
#define AES_SECURITY 80 // OR
// #define AES_SECURITY 128
//*********************************************

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <security_level> <max_group_size>" << std::endl;
        return 1;
    }
    int security_level = std::stoi(argv[1]);
    int max_group_size = std::stoi(argv[2]);
    if (security_level != 80 && security_level != 128) {
        std::cout << "security_level must be 80 or 128" << std::endl;
        return 1;
    }

    auto pp = FNIAGKA::Setup(security_level, max_group_size);

    std::vector<std::shared_ptr<PNPublicKey>> pn_pks(3);
    for (int i = 0; i < 3; i++) {
        pn_pks[i] = FNIAGKA::PNGen(pp);
    }

    auto omega = FNIAGKA::Negotiate(pp, pn_pks);

    std::cout << "DONE" << std::endl;
}
