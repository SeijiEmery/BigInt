//
//  main.cpp
//  BigInt
//
//  Created by semery on 8/25/16.
//  Copyright © 2016 Seiji Emery. All rights reserved.
//

#include <iostream>
#include <vector>
#include <cassert>

#define ENABLE_UNITTESTS 1
#define UNITTEST_REPORT_ON_SUCCESS 0
#include "unittest.hpp"

namespace storage {

constexpr size_t STORAGE_BITS = 32;
typedef uint32_t smallInt_t;  // BigInt storage type
typedef uint64_t bigInt_t;    // BigInt operation type (mul + add ops need 2x precision for overflow)
    
constexpr size_t LOW_BITMASK = (1L << STORAGE_BITS) - 1;
constexpr size_t HIGH_BITMASK = ~LOW_BITMASK;
constexpr smallInt_t MAX = (smallInt_t)((1L << STORAGE_BITS) - 1);
    
UNITTEST_METHOD(verifyStorageValueTypes) {
    TEST_ASSERT_EQ(STORAGE_BITS, 32, "storage size changed! (tests assume 32-bit");
    TEST_ASSERT_EQ(sizeof(smallInt_t) * 8, STORAGE_BITS, "storage size does not match STORAGE_BITS!");
    TEST_ASSERT_EQ(sizeof(smallInt_t) * 2, sizeof(bigInt_t), "big int is not 2x size of small int!");
    
    TEST_ASSERT_EQ((smallInt_t)(1L << STORAGE_BITS), 0, "storage size not big enough");
    TEST_ASSERT_EQ((bigInt_t)(1L << STORAGE_BITS), 1L << STORAGE_BITS, "op size not big enough");
    TEST_ASSERT_EQ(LOW_BITMASK & HIGH_BITMASK, 0, "LOW_BITMASK overlaps with HIGH_BITMASK");
    TEST_ASSERT_EQ(LOW_BITMASK | HIGH_BITMASK, ((size_t)0) - 1, "LOW_BITMASK does not have perfect coverage with HIGH_BITMASK");
    
    TEST_ASSERT(MAX != 0, "storage::MAX cannot fit in storage value");
    TEST_ASSERT_EQ((bigInt_t)MAX, (bigInt_t)((1L << STORAGE_BITS) - 1), "invalid storage::MAX");
    TEST_ASSERT_EQ(MAX + 1, 0, "storage::MAX + 1 should wrap to 0");
    
} UNITTEST_END_METHOD

bigInt_t fromIntParts (smallInt_t high, smallInt_t low) {
    return ((bigInt_t)high << STORAGE_BITS) | (bigInt_t)low;
}
UNITTEST_METHOD(fromIntParts) {
    TEST_ASSERT_EQ(fromIntParts(0x15, 0x227), 0x1500000227);
    TEST_ASSERT_EQ(fromIntParts(0x0, (smallInt_t)0xAAAA12847923), 0x12847923);
    TEST_ASSERT_EQ(fromIntParts(0x0, 0x0), 0x0);
    TEST_ASSERT_EQ(fromIntParts(0xAA, 0x0), 0xAA00000000);
} UNITTEST_END_METHOD
    
void storeIntParts (bigInt_t v, smallInt_t& high, smallInt_t& low) {
    high = (smallInt_t)((v & HIGH_BITMASK) >> STORAGE_BITS);
    low  = (smallInt_t)(v & LOW_BITMASK);
}
UNITTEST_METHOD(storeIntParts) {
    smallInt_t a, b;
    
    storeIntParts(0x0, a, b);
    TEST_ASSERT_EQ(a, 0);
    TEST_ASSERT_EQ(b, 0);
    
    storeIntParts(0x1500000227, a, b);
    TEST_ASSERT_EQ(a, 0x15);
    TEST_ASSERT_EQ(b, 0x227);
    
    storeIntParts(0xAAAA12847923, a, b);
    TEST_ASSERT_EQ(a, 0xAAAA);
    TEST_ASSERT_EQ(b, 0x12847923);
    
    storeIntParts(0xAA00000000, a, b);
    TEST_ASSERT_EQ(a, 0xAA);
    TEST_ASSERT_EQ(b, 0x0);
} UNITTEST_END_METHOD
    
UNITTEST_MAIN_METHOD(storage::unittests) {
    RUN_UNITTEST(verifyStorageValueTypes, UNITTEST_INSTANCE);
    RUN_UNITTEST(fromIntParts,  UNITTEST_INSTANCE);
    RUN_UNITTEST(storeIntParts, UNITTEST_INSTANCE);
} UNITTEST_END_METHOD
    
}; // namespace storage


class BigInt {
    std::vector<storage::smallInt_t> sections;
    bool sign = false;
public:
    BigInt (const std::string& s) {
        auto ptr = s.c_str();
        initFromString(ptr);
    }
    BigInt (const BigInt & v) : sections(v.sections), sign(v.sign) {}
private:
    // Private constructor for unit tests (unsafe for external code; would need to normalize values)
    BigInt (std::initializer_list<storage::smallInt_t> values) : sections(values) {}
    BigInt (bool sign, std::initializer_list<storage::smallInt_t> values) : sign(sign), sections(values) {}
    BigInt () {}
    
    template <bool sign = true>
    static BigInt create (std::initializer_list<storage::smallInt_t> values) { return { sign, values }; }
    
public:
    void initFromString (const char*& s) {
        sections.clear();
        
        if (s[0] == '-')      ++s, sign = true;
        else if (s[0] == '+') ++s, sign = false;
        
        if (s[0] < '0' || s[0] > '9')
            throw new std::runtime_error("String does not form a valid integer!");
        
        while (!(s[0] < '0' || s[0] > '9'))
            pushDecimalDigit(s[0] - '0'), ++s;
    }
    static UNITTEST_METHOD(initFromString) {
        
        BigInt a { "1" };
        TEST_ASSERT_EQ(a.sections.size(), 1);
        TEST_ASSERT_EQ(a.sections[0], 1);
        
        BigInt b { "42" };
        TEST_ASSERT_EQ(b.sections.size(), 1);
        TEST_ASSERT_EQ(b.sections[0], 42);
        
        // If this changes next tests won't work
        TEST_ASSERT_EQ(sizeof(typeof(b.sections[0])), 4, "base size changed? (expected 32-bit storage, 64-bit ops)");
        
        BigInt c { "4294967297" }; // 1 << 32 + 1
        TEST_ASSERT_EQ(c.sections.size(), 2);
        TEST_ASSERT_EQ(c.sections[0], 1);
        TEST_ASSERT_EQ(c.sections[1], 1);
        
        BigInt d { "64424509677" }; // (1 << 32) * 15 + 237
        TEST_ASSERT_EQ(d.sections.size(), 2);
        TEST_ASSERT_EQ(d.sections[0], 237);
        TEST_ASSERT_EQ(d.sections[1], 15);
        
        BigInt e { "-64424509677" };
        TEST_ASSERT_EQ(e.sign, true);
        TEST_ASSERT_EQ(e.sections.size(), 2);
        TEST_ASSERT_EQ(e.sections[0], 237);
        TEST_ASSERT_EQ(e.sections[1], 15);
        
        BigInt f { "0" };
        TEST_ASSERT_EQ(f.sections.size(), 1);
        TEST_ASSERT_EQ(f.sections[0], 0);
        
        BigInt g { "64424509440" }; // (1 << 32) * 15
        TEST_ASSERT_EQ(g.sections.size(), 2);
        TEST_ASSERT_EQ(g.sections[0], 0);
        TEST_ASSERT_EQ(g.sections[1], 15);
        
        BigInt h { "4294967296" }; // 1 << 32 exact
        TEST_ASSERT_EQ(h.sections.size(), 2);
        TEST_ASSERT_EQ(h.sections[0], 0);
        TEST_ASSERT_EQ(h.sections[1], 1);
        
    } UNITTEST_END_METHOD
    
    void pushDecimalDigit (storage::smallInt_t digit) {
        assert(digit <= 9);
        if (!sections.size())
            sections.push_back(digit);
        else
            multiplyAdd(10, digit);
    }
    static UNITTEST_METHOD(pushDecimalDigit) {
        
        BigInt a { 0 };
        TEST_ASSERT_EQ(a.sections.size(), 1);
        a.sections.pop_back();
        
        a.pushDecimalDigit(9);
        TEST_ASSERT_EQ(a.sections.size(), 1);
        TEST_ASSERT_EQ(a.sections[0], 9);
        
        a.pushDecimalDigit(1);
        TEST_ASSERT_EQ(a.sections.size(), 1);
        TEST_ASSERT_EQ(a.sections[0], 91);
        
        a.pushDecimalDigit(5);
        TEST_ASSERT_EQ(a.sections.size(), 1);
        TEST_ASSERT_EQ(a.sections[0], 915);
        
    } UNITTEST_END_METHOD
    
    BigInt& operator+= (storage::smallInt_t v) {
        return multiplyAdd(1, v);
    }
    static UNITTEST_METHOD(scalarAdd) {
        
        BigInt a { 15 };
        TEST_ASSERT_EQ(a.sections.size(), 1, "bad initial size!");
        TEST_ASSERT_EQ(a.sections[0], 15, "bad init value");
        
        a += 3;
        TEST_ASSERT_EQ(a.sections.size(), 1, "bad size after += 3");
        TEST_ASSERT_EQ(a.sections[0], 18, "+= 3");
        
        a += 12;
        TEST_ASSERT_EQ(a.sections.size(), 1, "bad size after += 12");
        TEST_ASSERT_EQ(a.sections[0], 30, "+= 12");
        
        a += (((size_t)1 << storage::STORAGE_BITS) - 1);
        TEST_ASSERT_EQ(a.sections.size(), 2, "should overflow to 2 values");
        TEST_ASSERT_EQ(a.sections[0], 29, "low  value (post overflow)");
        TEST_ASSERT_EQ(a.sections[1], 1,  "high value (post overflow)");
        
        a.sections.pop_back();
        a.sections.pop_back();
        TEST_ASSERT_EQ(a.sections.size(), 0, "bad section size!");
        
        a += 0;
        TEST_ASSERT_EQ(a.sections.size(), 1, "bad size after += 0");
        TEST_ASSERT_EQ(a.sections[0], 0,     "probably dead now");
        
        a.sections.pop_back();
        TEST_ASSERT_EQ(a.sections.size(), 0, "bad size (see above)");
        
        a += 1;
        TEST_ASSERT_EQ(a.sections.size(), 1, "bad size after += 1");
        TEST_ASSERT_EQ(a.sections[0], 1,     "+= 1");
        
        // note: this is reversed:
        //         bit 0-31      bit 32-63     bit 64-95     bit 96-127
        BigInt b { storage::MAX, storage::MAX, storage::MAX, 125 };
        TEST_ASSERT_EQ(b.sections.size(), 4, "b invalid storage size?!");
        TEST_ASSERT_EQ(b.sections[0], storage::MAX, "b initial min value");
        TEST_ASSERT_EQ(b.sections[3], 125, "b initial max value");
        
        b += 6;
        TEST_ASSERT_EQ(b.sections.size(), 4, "b storage size should not change");
        TEST_ASSERT_EQ(b.sections[0], 5, "bit 0: storage::MAX should overflow to 5");
        TEST_ASSERT_EQ(b.sections[1], 0, "bit 32: storage::MAX should overflow to 0 (carry +1)");
        TEST_ASSERT_EQ(b.sections[2], 0, "bit 64: storage::MAX should overflow to 0 (carry +1)");
        TEST_ASSERT_EQ(b.sections[3], 126, "bit 96: should get carry +1");
        
    } UNITTEST_END_METHOD
    
    BigInt& operator *= (storage::smallInt_t v) {
        return multiplyAdd(v, 0);
    }
    static UNITTEST_METHOD(scalarMul) {
        
        BigInt a { 1 };
        TEST_ASSERT_EQ(a.sections.size(), 1);
        TEST_ASSERT_EQ(a.sections[0], 1);
        
        a *= 15;
        TEST_ASSERT_EQ(a.sections.size(), 1);
        TEST_ASSERT_EQ(a.sections[0], 15);
        
        a *= storage::MAX;
        TEST_ASSERT_EQ(a.sections.size(), 2, "15 * storage::MAX");
        TEST_ASSERT_EQ(a.sections[0], (15L * storage::MAX) % (1L << storage::STORAGE_BITS), "15 * storage::MAX: low bits");
        TEST_ASSERT_EQ(a.sections[1], (15L * storage::MAX) / (1L << storage::STORAGE_BITS), "15 * storage::MAX: high bits");
        
        //
        // Test multiplying a 160-bit big num by a large 32-bit coefficient:
        //
        
        // note: reversed
        //         min value                                       max value
        BigInt b { 0x28fa9923, 0x49378824, 0xffff99ff, 0xffffffff, 0x22487943 };
        TEST_ASSERT_EQ(b.sections.size(), 5, "b storage section count");
        TEST_ASSERT_EQ(b.sections[0], 0x28fa9923, "b[0] initial");
        TEST_ASSERT_EQ(b.sections[1], 0x49378824, "b[1] initial");
        TEST_ASSERT_EQ(b.sections[2], 0xffff99ff, "b[2] initial");
        TEST_ASSERT_EQ(b.sections[3], 0xffffffff, "b[3] initial");
        TEST_ASSERT_EQ(b.sections[4], 0x22487943, "b[4] initial");
        
        // Multiply the above big num by a large number
        b *= 0x59ff2938;
        
        // Calculate our expected result manually:
        size_t MAX_WRAP = 1L << storage::STORAGE_BITS;
        size_t m0 = (0x28fa9923L * 0x59ff2938L),      x0 = m0 % MAX_WRAP, c0 = m0 / MAX_WRAP;
        size_t m1 = (0x49378824L * 0x59ff2938L + c0), x1 = m1 % MAX_WRAP, c1 = m1 / MAX_WRAP;
        size_t m2 = (0xffff99ffL * 0x59ff2938L + c1), x2 = m2 % MAX_WRAP, c2 = m2 / MAX_WRAP;
        size_t m3 = (0xffffffffL * 0x59ff2938L + c2), x3 = m3 % MAX_WRAP, c3 = m3 / MAX_WRAP;
        size_t m4 = (0x22487943L * 0x59ff2938L + c3), x4 = m4 % MAX_WRAP, c4 = m4 / MAX_WRAP;
        
        // And compare with result:
        TEST_ASSERT_EQ(b.sections.size(), 6, "should have 6 sections?");
        TEST_ASSERT_EQ(b.sections[0], x0, "b[0] post-multiply");
        TEST_ASSERT_EQ(b.sections[1], x1, "b[1] post-multiply");
        TEST_ASSERT_EQ(b.sections[2], x2, "b[2] post-multiply");
        TEST_ASSERT_EQ(b.sections[3], x3, "b[3] post-multiply");
        TEST_ASSERT_EQ(b.sections[4], x4, "b[4] post-multiply");
        TEST_ASSERT_EQ(b.sections[5], c4, "b[5] post-multiply");
        
    } UNITTEST_END_METHOD
    
    // Executes a multiply + add operation: multiplies each digit in BigNum by base, adds an additional scalar value (carry).
    // Incidentally, multiply is just this with carry = 0, and add scalar is this with base = 1.
    BigInt& multiplyAdd (storage::smallInt_t base, storage::smallInt_t carry) {
        for (auto i = 0; i < sections.size(); ++i) {
            auto r = (storage::bigInt_t)sections[i] * (storage::bigInt_t)base + (storage::bigInt_t)carry;
            storage::storeIntParts(r, carry, sections[i]);
        }
        if (carry || !sections.size()) sections.push_back(carry);
        return *this;
    }
    static UNITTEST_METHOD(scalarMultiplyAdd) {
        // Since we've already tested *= and += (we'll assume the above tests passed),
        // we'll use this test to double check x = x * 10 + n, which is the core of our
        // decimal-to-binary algorithm.
        
        BigInt a { 0 };
        TEST_ASSERT_EQ(a.sections.size(), 1, "a initial size");
        TEST_ASSERT_EQ(a.sections[0], 0,     "a[0] initial");
        
        a.multiplyAdd(10, 1);
        TEST_ASSERT_EQ(a.sections.size(), 1);
        TEST_ASSERT_EQ(a.sections[0], 1, "a[0] (0) * 10 + 1");
        
        a.multiplyAdd(10, 9);
        TEST_ASSERT_EQ(a.sections.size(), 1);
        TEST_ASSERT_EQ(a.sections[0], 19, "a[0] (1) * 10 + 9");
        
        a.multiplyAdd(256, 22);
        TEST_ASSERT_EQ(a.sections.size(), 1);
        TEST_ASSERT_EQ(a.sections[0], 4886, "a[0] (19) * 256 + 22");
        
        // Test with true big int:
        BigInt b { 0x1210981F, 0xFA093811, 0x9C049814, 0x342981F9 };
        TEST_ASSERT_EQ(b.sections.size(), 4);
        TEST_ASSERT_EQ(b.sections[0], 0x1210981F);
        TEST_ASSERT_EQ(b.sections[1], 0xFA093811);
        TEST_ASSERT_EQ(b.sections[2], 0x9C049814);
        TEST_ASSERT_EQ(b.sections[3], 0x342981F9);
        
        b.multiplyAdd(256, 5);
        TEST_ASSERT_EQ(b.sections.size(), 5);
        TEST_ASSERT_EQ(b.sections[0], 0x10981F05);
        TEST_ASSERT_EQ(b.sections[1], 0x09381112);
        TEST_ASSERT_EQ(b.sections[2], 0x049814FA);
        TEST_ASSERT_EQ(b.sections[3], 0x2981F99C);
        TEST_ASSERT_EQ(b.sections[4], 0x00000034);
        
    } UNITTEST_END_METHOD
    
    BigInt& scalarDiv (storage::smallInt_t d, storage::smallInt_t& rem) {
        rem = 0;
        for (auto i = sections.size(); i > 0; --i) {
            auto n = storage::fromIntParts(rem, sections[i-1]);
            sections[i-1] = (storage::smallInt_t)(n / (storage::bigInt_t)d);
            rem           = (storage::smallInt_t)(n % (storage::bigInt_t)d);
        }
        while (sections.size() > 0 && sections.back() == 0)
            sections.pop_back();
        return *this;
    }
    BigInt& operator /= (storage::smallInt_t v) {
        storage::smallInt_t r;
        return scalarDiv(v, r);
    }
    static UNITTEST_METHOD(scalarDiv) {
        
        // Unit tests TBD
    
    } UNITTEST_END_METHOD
    
    // Signed multiply / divide operations
    BigInt& operator *= (int v) {
        if (v < 0)
            sign = !sign, v = -v;
        return operator *= ((storage::smallInt_t)v);
    }
    BigInt& operator /= (int v) {
        if (v < 0)
            sign = !sign, v = -v;
        return operator /= ((storage::smallInt_t)v);
    }
    
    std::vector<char>& writeString (std::vector<char>& str, bool writeNull = false) {
        if (sign) str.push_back('-');
        
        if (!sections.size()) {
            str.push_back('0');
            return str;
        } else {
            // Using a copy (this operation is destructive), write digits in reverse order
            BigInt temp { *this };
            assert(temp.sections.size() == sections.size());
            auto s0 = str.size();
            
            storage::smallInt_t rem;
            while (temp.sections.size()) {
                temp.scalarDiv(10, rem);
                str.push_back('0' + rem);
            }
            
            // And then reverse to get correct value
            for (auto i = s0, j = str.size()-1; i < j; ++i, --j)
                std::swap(str[i], str[j]);
            
            if (writeNull)
                str.push_back('\0');
            return str;
        }
    }
    static UNITTEST_METHOD(writeString) {
        std::vector<char> s;
        TEST_ASSERT_EQ(std::string("0"), BigInt{0}.writeString(s, true).data()); s.clear();
        TEST_ASSERT_EQ(std::string("1"), BigInt{1}.writeString(s, true).data()); s.clear();
        
        typedef std::initializer_list<storage::smallInt_t> iv;
        TEST_ASSERT_EQ(BigInt{true, iv{24}}.sign, true);
        TEST_ASSERT_EQ(BigInt{true, iv{24}}.sections.size(), 1);
        TEST_ASSERT_EQ(BigInt{true, iv{24}}.sections[0], 24);
        TEST_ASSERT_EQ(std::string("-24"), BigInt{true, iv{24}}.writeString(s, true).data()); s.clear();
        
        TEST_ASSERT_EQ(std::string("680564733841876926926749214863536422912"), BigInt::pow2(129).writeString(s, true).data()); s.clear();
    } UNITTEST_END_METHOD
    
    operator bool () const { return sections.size() && !(sections.size() == 1 && sections[0] == 0); }
    
    bool operator == (const BigInt& other) const { return this->cmp(other) == 0; }
    bool operator <= (const BigInt& other) const { return this->cmp(other) <= 0; }
    bool operator >= (const BigInt& other) const { return this->cmp(other) >= 0; }
    
    
    int cmp (const BigInt& other) const {
        if (!this->operator bool()) return !other.operator bool() ? 0 : other.sign ? 1 : -1;
        if (!other.operator bool()) return sign ? -1 : 1;
        if (sign != other.sign)     return sign ? -1 : 1;
        assert(sections.size() && other.sections.size());
        
        if (sections.size() != other.sections.size())
            return sections.size() < other.sections.size() ? -2 : 2;
        
//        for (auto i = std::min(sections.size(), other.sections.size()); i --> 0; ) {
        for (auto i = 0; i < std::min(sections.size(), other.sections.size()); ++i) {
            if (sections[i] < other.sections[i]) return sign ? 1 : -1;
            if (sections[i] > other.sections[i]) return sign ? -1 : 1;
        }
        return 0;
    }
    static UNITTEST_METHOD(cmp) {
#define BINT(sign,args...) BigInt{sign, std::initializer_list<storage::smallInt_t>{args}}
#define BINT_TEST_CMP(a,b, args...) TEST_ASSERT_EQ(a.cmp(b), args)
        
        TEST_ASSERT_EQ(BINT(true,  0).operator bool(), false, "BigInt operator bool ( 0 )");
        TEST_ASSERT_EQ(BINT(false, 0).operator bool(), false, "BigInt operator bool ( 0 )");
        TEST_ASSERT_EQ(BINT(true,  1).operator bool(), true,  "BigInt operator bool ( 1 )");
        TEST_ASSERT_EQ(BINT(true, 24, 12, 99, 84, 239).operator bool(), true, "BigInt operator bool ( ... )");
        
        auto a = BINT(true, 0);
        TEST_ASSERT(a.sections.size()) && (a.sections.pop_back(), true) &&
        TEST_ASSERT_EQ(a.operator bool(), false);
        
        a.sections.push_back(0); TEST_ASSERT_EQ(a.operator bool(), false);
        a.sections[0] = 1;       TEST_ASSERT_EQ(a.operator bool(), true);
        
        BINT_TEST_CMP(BINT(true,  0), BINT(true,  0), 0);
        BINT_TEST_CMP(BINT(true,  0), BINT(false, 0), 0);
        BINT_TEST_CMP(BINT(false, 0), BINT(true,  0), 0);
        BINT_TEST_CMP(BINT(false, 0), BINT(false, 0), 0);

        BINT_TEST_CMP(BINT(true,  42), BINT(true,  42),  0, "42 == 42?");
        BINT_TEST_CMP(BINT(false, 42), BINT(false, 42),  0, "42 == 42?");
        BINT_TEST_CMP(BINT(true,  42), BINT(false, 42), -1, "-42 < 42?");
        BINT_TEST_CMP(BINT(false, 42), BINT(true,  42),  1, "42 > -42?");
        
        BINT_TEST_CMP(BINT(true,  42), BINT(false, 0), -1, "-42 < 0?");
        BINT_TEST_CMP(BINT(false, 42), BINT(false, 0),  1,  "42 > 0?");
        BINT_TEST_CMP(BINT(true,  42), BINT(true,  0), -1, "-42 < 0?");
        BINT_TEST_CMP(BINT(false, 42), BINT(true,  0),  1,  "42 > 0?");
        
        BINT_TEST_CMP(BINT(true,  42), BINT(true,  41), -1, "-42 < -41?");
        BINT_TEST_CMP(BINT(false, 42), BINT(false, 43), -1,  "42 < 43?");
        
        BINT_TEST_CMP(BINT(true,  42, 299, 384), BINT(true,  42, 299, 384), 0, "-42 299 384 == -42 299 384?");
        BINT_TEST_CMP(BINT(false, 42, 299, 384), BINT(false, 42, 299, 384), 0, "+42 299 384 == +42 299 384?");
        
        BINT_TEST_CMP(BINT(false, 41, 399, 389), BINT(false, 42, 299, 384), -1, "+41 399 389 < +42 299 384?");
        BINT_TEST_CMP(BINT(true,  41, 399, 389), BINT(true,  42, 299, 384),  1, "-41 399 389 > -42 299 384?");
        
        BINT_TEST_CMP(BINT(false, 42, 399, 383), BINT(false, 42, 299, 384), 1, "+42 399 383 > +42 299 384?");
        BINT_TEST_CMP(BINT(false, 42, 299, 389), BINT(false, 42, 299, 384), 1, "+42 299 389 > +42 299 384?");
        
        BINT_TEST_CMP(BigInt::pow2(229), BigInt::pow2(229), 0);
        BINT_TEST_CMP(BigInt::pow2(230), BigInt::pow2(229), 1);
        BINT_TEST_CMP(BigInt::pow2(229), BigInt::pow2(230), -1);
        
#undef BINT_TEST_CMP
#undef BINT
    } UNITTEST_END_METHOD
    
    BigInt operator * (const BigInt & v) {
        
        // Zero case
        if (!operator bool() || !v.operator bool()) {
            return sections.clear(), *this;
        }
        
        // Otherwise, prepare to multiply, and reserve space for new digits
        BigInt r; r.sections.resize(sections.size() + v.sections.size(), 0);
        
        storage::smallInt_t carry;
        for (auto i = 0; i < sections.size(); ++i) {
            for (auto j = 0; j < v.sections.size(); ++j) {
                auto p = (storage::bigInt_t)sections[i] * (storage::bigInt_t)v.sections[j] + (storage::bigInt_t)r.sections[i+j];
                storage::storeIntParts(p, carry, r.sections[i+j]);
                
                for (auto k = i+j+1; carry != 0; ++k) {
                    if (k > r.sections.size()) {
                        r.sections.push_back(carry); break;
                    } else {
                        auto s = (storage::bigInt_t)r.sections[k] + (storage::bigInt_t)carry;
                        storage::storeIntParts(s, carry, r.sections[k]);
                    }
                }
            }
        }
        return r;
    }
    
    static UNITTEST_METHOD(bigInt_mul) {
        auto a = BigInt::pow2(39) * BigInt::pow2(78);
        TEST_ASSERT_EQ(std::string(BigInt::pow2(117).toString()), "166153499473114484112975882535043072");

        auto x = BigInt("92837508234109812317501984209810928409182094187192");
        auto y = BigInt("19874891279817498172489713987498173849713897489171");
        auto z = x * y;
        
        auto zero = BigInt{0};
        auto one  = BigInt{1};
        
        TEST_ASSERT_EQ(std::string(x.toString()), "92837508234109812317501984209810928409182094187192");
        TEST_ASSERT_EQ(std::string(y.toString()), "19874891279817498172489713987498173849713897489171");
        TEST_ASSERT_EQ(std::string(z.toString()), "18451353828420942924773305110003083474370975946120"
                                                  "06265189858865520503519713569495483976002866897832");
        TEST_ASSERT_EQ(z, z);
        TEST_ASSERT_EQ(z * 1, z);
        TEST_ASSERT_EQ(z * 0, x * 0);
        TEST_ASSERT_EQ(std::string((z * zero).toString()), "0");
        TEST_ASSERT_EQ(z * one,  z);
        TEST_ASSERT_EQ(z * zero, zero);
        
    } UNITTEST_END_METHOD
    
    
    static UNITTEST_METHOD(bigInt_add) {
        
    } UNITTEST_END_METHOD
    
    static UNITTEST_METHOD(bigInt_sub) {
        
    } UNITTEST_END_METHOD
    
    static UNITTEST_METHOD(bigInt_div) {
        
    } UNITTEST_END_METHOD
    
    
    
    
    
    // Vector (BigInt * BigInt) Addition, Multiplication, Division TBD
    
    // BigInt 2's complement negative numbers TBD
    
    // BigInt comparison TBD
    
    
    // Basic pow2 operation
    static BigInt pow2 (unsigned n) {
        BigInt x { 1 };
        while (n > 0)
            --n, x *= 2;
        return x;
    }
    static UNITTEST_METHOD(pow2) {
        TEST_ASSERT_EQ(std::string(BigInt::pow2(0).toString()), "1", "2^0");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(1).toString()), "2", "2^1");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(2).toString()), "4", "2^2");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(3).toString()), "8", "2^3");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(4).toString()), "16", "2^4");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(5).toString()), "32", "2^5");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(6).toString()), "64", "2^6");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(7).toString()), "128", "2^7");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(8).toString()), "256", "2^8");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(16).toString()), "65536", "2^16");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(32).toString()), "4294967296", "2^32");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(64).toString()), "18446744073709551616", "2^64");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(128).toString()), "340282366920938463463374607431768211456", "2^128");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(256).toString()),
                "115792089237316195423570985008687907853269984665640564039457584007913129639936", "2^256");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(512).toString()),
                "134078079299425970995740249982058461274793658205923933777235614437217640300735"
                "46976801874298166903427690031858186486050853753882811946569946433649006084096", "2^512");
        TEST_ASSERT_EQ(std::string(BigInt::pow2(1024).toString()),
                "17976931348623159077293051907890247336179769789423065727343008115773267580550"
                "09631327084773224075360211201138798713933576587897688144166224928474306394741"
                "24377767893424865485276302219601246094119453082952085005768838150682342462881"
                "473913110540827237163350510684586298239947245938479716304835356329624224137216", "2^1024");
    } UNITTEST_END_METHOD
    
private:
    std::vector<char> _tempStr;
public:
    const char* toString () {
        _tempStr.clear();
        writeString(_tempStr);
        _tempStr.push_back('\0');
        return _tempStr.data();
    }
    
    static UNITTEST_MAIN_METHOD(BigInt) {
        RUN_UNITTEST(scalarAdd, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(scalarMul, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(scalarDiv, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(scalarMultiplyAdd, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(pushDecimalDigit, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(initFromString, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(writeString, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(pow2, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(cmp, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(bigInt_mul, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(bigInt_add, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(bigInt_sub, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(bigInt_div, UNITTEST_INSTANCE);
    } UNITTEST_END_METHOD
};

std::ostream& operator << (std::ostream& os, BigInt v) {
    return os << v.toString();
}

bool runAllTests () {
    return
        RUN_UNITTEST_MAIN(storage) &&
        RUN_UNITTEST_MAIN(BigInt);
}


int main(int argc, const char * argv[]) {
    if (!runAllTests())
        return -1;
    
    auto x = BigInt { "-123456789123456789123456789123456789123456789" };
    auto y = BigInt { "2" };
    
    std::cout << "x = " << x << '\n';
    std::cout << "y = " << y << '\n';
    
    unsigned i = 0; auto v = BigInt { "1" };
    while (i < 130) {
        std::cout << "2^" << i << " = " << v << '\n';
        v *= 2;
        i += 1;
    }
    
    auto n = 32768; // 2^15; will calculate by bruteforcing x *= 2 n times.
    std::cout << "2^" << n << " = " << BigInt::pow2(n) << '\n';
    
    return 0;
}
