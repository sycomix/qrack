#pragma once

#include <iomanip>
#include <sstream>

/* A quick-and-dirty epsilon for clamping floating point values. */
#define QRACK_TEST_EPSILON 0.5

/* Declare the stream-to-probability prior to including catch.hpp. */
namespace Qrack {
inline std::ostream& outputPerBitProbs(std::ostream& os, Qrack::CoherentUnit const& constReg);
inline std::ostream& outputProbableResult(std::ostream& os, Qrack::CoherentUnit const& constReg);
inline std::ostream& outputIndependentBits(std::ostream& os, Qrack::CoherentUnit const& constReg);

inline std::ostream& operator<<(std::ostream& os, Qrack::CoherentUnit const& constReg)
{
    if (os.flags() & std::ios_base::showpoint) {
        os.unsetf(std::ios_base::showpoint);
        return outputPerBitProbs(os, constReg);
    }
    // Super expensive and slow.
    if (os.flags() & std::ios_base::showbase) {
        os.unsetf(std::ios_base::showbase);
        return outputIndependentBits(os, constReg);
    }
    return outputProbableResult(os, constReg);
}

inline std::ostream& outputPerBitProbs(std::ostream& os, Qrack::CoherentUnit const& constReg)
{
    Qrack::CoherentUnit& qftReg = (Qrack::CoherentUnit&)constReg;
    os << "[\n";

    for (int i = qftReg.GetQubitCount() - 1; i >= 0; i--) {
        os << "\t " << std::setw(2) << i << "]: " << qftReg.Prob(i) << std::endl;
    }
    return os;
}

inline std::ostream& outputProbableResult(std::ostream& os, Qrack::CoherentUnit const& constReg)
{
    int i;

    Qrack::CoherentUnit& qftReg = (Qrack::CoherentUnit&)constReg;
    double maxProb = 0;
    int maxProbIdx = 0;

    // Iterate through all possible values of the bit array, starting at the
    // max.
    for (i = qftReg.GetMaxQPower() - 1; i >= 0; i--) {
        double prob = qftReg.ProbAll(i);
        if (prob > maxProb) {
            maxProb = prob;
            maxProbIdx = i;
        }
    }

    os << qftReg.GetQubitCount() << "/";

    // Print the resulting maximum probability bit pattern.
    for (i = qftReg.GetMaxQPower() >> 1; i > 0; i >>= 1) {
        // printf("%08X & %08X\n", i, maxProbIdx);
        if (i & maxProbIdx) {
            os << "1";
        } else {
            os << "0";
        }
    }

    // And print the probability, for interest.
    os << ":" << maxProb;

    return os;
}

inline std::ostream& outputIndependentBits(std::ostream& os, Qrack::CoherentUnit const& constReg)
{
    Qrack::CoherentUnit& qftReg = (Qrack::CoherentUnit&)constReg;
    os << "" << qftReg.GetQubitCount() << "/";

    for (int j = qftReg.GetQubitCount() - 1; j >= 0; j--) {
        os << (int)(qftReg.Prob(j) > QRACK_TEST_EPSILON);
    }

    return os;
}

inline std::ostream& operator<<(std::ostream& os, const std::unique_ptr<Qrack::CoherentUnit>& constReg)
{
    Qrack::CoherentUnit& qftReg = (Qrack::CoherentUnit&)*constReg;
    os << qftReg;
    return os;
}

} // namespace Qrack

#include "catch.hpp"

/*
 * A fixture to create a unique CoherentUnit test, of the appropriate type, for
 * each executing test case.
 */
class CoherentUnitTestFixture {
protected:
    std::unique_ptr<Qrack::CoherentUnit> qftReg;

public:
    CoherentUnitTestFixture();
};

class ProbPattern : public Catch::MatcherBase<Qrack::CoherentUnit> {
    bitLenInt start;
    bitLenInt length;
    uint64_t mask;

public:
    ProbPattern(bitLenInt s, bitLenInt l, uint64_t m)
        : start(s)
        , length(l)
        , mask(m)
    {
    }

    virtual bool match(Qrack::CoherentUnit const& constReg) const override
    {
        Qrack::CoherentUnit& qftReg = (Qrack::CoherentUnit&)constReg;

        if (length == 0) {
            ((ProbPattern*)this)->length = qftReg.GetQubitCount();
        }

        if (length > sizeof(mask) * 8) {
            WARN("requested length " << length << " larger than possible bitmap " << sizeof(mask) * 8);
            return false;
        }

        for (int j = 0; j < length; j++) {
            /* Consider anything more than a 50% probability as a '1'. */
            bool bit = (qftReg.Prob(j + start) > QRACK_TEST_EPSILON);
            if (bit != !!(mask & (1 << j))) {
                return false;
            }
        }
        return true;
    }

    virtual bool match(const std::unique_ptr<Qrack::CoherentUnit>& constReg) const { return match(*constReg); }

    virtual std::string describe() const
    {
        std::ostringstream ss;
        ss << "matches bit pattern [" << (int)start << "," << start + length << "]: " << (int)length << "/";
        for (int j = length; j >= 0; j--) {
            ss << !!((int)(mask & (1 << j)));
        }
        return ss.str();
    }
};

inline ProbPattern HasProbability(bitLenInt s, bitLenInt l, uint64_t m) { return ProbPattern(s, l, m); }
inline ProbPattern HasProbability(uint64_t m) { return ProbPattern(0, 0, m); }
