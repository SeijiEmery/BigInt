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
#define UNITTEST_REPORT_ON_SUCCESS 1
#include "unittest.hpp"

namespace storage {

constexpr size_t STORAGE_BITS = 32;
typedef uint32_t smallInt_t;  // BigInt storage type
typedef uint64_t bigInt_t;    // BigInt operation type (mul + add ops need 2x precision for overflow)
    
constexpr size_t LOW_BITMASK = (1L << STORAGE_BITS) - 1;
constexpr size_t HIGH_BITMASK = ~LOW_BITMASK;
    
UNITTEST_METHOD(verifyStorageValueTypes) {
    TEST_ASSERT_EQ(STORAGE_BITS, 32, "storage size changed! (tests assume 32-bit");
    TEST_ASSERT_EQ(sizeof(smallInt_t) * 8, STORAGE_BITS, "storage size does not match STORAGE_BITS!");
    TEST_ASSERT_EQ(sizeof(smallInt_t) * 2, sizeof(bigInt_t), "big int is not 2x size of small int!");
    
    TEST_ASSERT_EQ((smallInt_t)(1L << STORAGE_BITS), 0, "storage size not big enough");
    TEST_ASSERT_EQ((bigInt_t)(1L << STORAGE_BITS), 1L << STORAGE_BITS, "op size not big enough");
    TEST_ASSERT_EQ(LOW_BITMASK & HIGH_BITMASK, 0, "LOW_BITMASK overlaps with HIGH_BITMASK");
    TEST_ASSERT_EQ(LOW_BITMASK | HIGH_BITMASK, ((size_t)0) - 1, "LOW_BITMASK does not have perfect coverage with HIGH_BITMASK");
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

struct typeinfo_t {
    
};

struct member_typeinfo_t {
    std::string name;
    typeinfo_t type;
    
    member_typeinfo_t (std::string name, typeinfo_t type) : name(name), type(type) {}
};

template <typename T>
struct typeinfo {
    static typeinfo_t value;
};

typedef std::vector<member_typeinfo_t> member_typeinfo_tbl;

#define METACLASS_INFO struct classinfo {
#define CLASS_MEMBER_INFO const member_typeinfo_tbl members = {
#define END_CLASS_MEMBER_INFO };
#define END_METACLASS_INFO };

#define DESCRIBE_CLASS_MEMBER(member) member_typeinfo_t { #member, typeinfo<typeof(member)>::value },

#define METACLASS_MEMBER_INFO const member_typeinfo_tbl classinfo_members = {
#define END_METACLASS_MEMBER_INFO };

#define DESCRIBE_MEMBERS(args...) \
    const member_typeinfo tbl classinfo_members = { \


class BigInt {
    METACLASS_MEMBER_INFO
        DESCRIBE_CLASS_MEMBER(sections)
        DESCRIBE_CLASS_MEMBER(sign)
    END_METACLASS_MEMBER_INFO
    
    //    struct classinfo {
    //        const member_typeinfo_tbl members = {
    //            { "sections", typeinfo<std::vector<storage::smallInt_t>>::value },
    //            { "sign", typeinfo<typeof(sign)>::value },
    //        };
    //    };
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
        TEST_ASSERT_EQ(c.sections[1], 2);
        
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
        else {
            operator+=(10);
            operator*=(digit);
        }
        
        std::cout << "Pushed digit '" << digit << "': '" << toString() << "' "
            "sections = " << sections.size() << ": [ ";
        for (auto i = 0; i < sections.size(); ++i) {
            if (i != 0) std::cout << ", ";
            std::cout << sections[i];
        }
        std::cout << " ]\n";
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
        
        auto carry = v;
        for (auto i = 0; carry && i < sections.size(); ++i) {
            storage::bigInt_t sum = (storage::bigInt_t)sections[i] + (storage::bigInt_t)carry;
            
            storage::storeIntParts(sum, carry, sections[i]);
        }
        // If carry value remaining, push back as a new section "digit cluster"
        if (carry || !sections.size()) sections.push_back(carry);
        return *this;
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
        
    } UNITTEST_END_METHOD
    
    BigInt& operator *= (storage::smallInt_t v) {
        storage::smallInt_t carry = 0;
        for (auto i = 0; i < sections.size(); ++i) {
            auto r = sections[i] * v + carry;
            
            storage::storeIntParts(r, carry, sections[i]);
//            sections[i] = r & SECTION_BITMASK_LOW;
//            carry       = r & SECTION_BITMASK_HIGH;
        }
        if (carry) sections.push_back(carry);
        return *this;
    }
    static UNITTEST_METHOD(scalarMul) {
    
    } UNITTEST_END_METHOD
    
    
    BigInt& operator /= (storage::smallInt_t v) {
        storage::smallInt_t rem = 0;
        for (auto i = sections.size(); i > 0; --i) {
            auto x = storage::fromIntParts(rem, sections[i-1]);
    
            sections[i-1] = (storage::smallInt_t)(x / v);
            rem           = (storage::smallInt_t)(x % v);
        }
        // Pop off zero values
        while (sections.size() > 0 && sections.back() == 0)
            sections.pop_back();
        return *this;
    }
    static UNITTEST_METHOD(scalarDiv) {
    
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
    
    std::vector<char>& writeString (std::vector<char>& str) {
        if (sign) str.push_back('-');
        
        if (!sections.size()) {
            str.push_back('0');
            return str;
        } else {
            // Using a copy (this operation is destructive), write digits in reverse order
            BigInt temp { *this };
            assert(temp.sections.size() == sections.size());
            auto s0 = str.size();
            
            while (temp.sections.size() && temp.sections[0] != 0) {
                str.push_back('0' + temp.sections[0] % 10);
                temp /= 10;
            }
            
            // And then reverse to get correct value
            for (auto i = s0, j = str.size(); i < j; ++i, --j)
                std::swap(str[i], str[j]);
            return str;
        }
    }
    static UNITTEST_METHOD(writeString) {
        
    } UNITTEST_END_METHOD
    
private:
    std::vector<char> _tempStr; // Not threadsafe!
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
        RUN_UNITTEST(pushDecimalDigit, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(initFromString, UNITTEST_INSTANCE) &&
        RUN_UNITTEST(writeString, UNITTEST_INSTANCE);
    } UNITTEST_END_METHOD
};

std::ostream& operator << (std::ostream& os, BigInt& v) {
    return os << v.toString();
}

//UNITTEST_METHOD(runAllTests) {
//    RUN_UNITTEST_MAIN(storage, UNITTEST_INSTANCE);
//    RUN_UNITTEST_MAIN(BigInt,  UNITTEST_INSTANCE);
//    // more tests to go here...
//} UNITTEST_END_METHOD

bool runAllTests () {
    return
        RUN_UNITTEST_MAIN(storage) &&
        RUN_UNITTEST_MAIN(BigInt);
}


int main(int argc, const char * argv[]) {
    if (!runAllTests())
        return -1;
//    if (!RUN_UNITTEST(runAllTests))
//        return -1;
    
    auto x = BigInt { "-123456789" };
    auto y = BigInt { "2" };
    
    std::cout << "x = " << x << '\n';
    std::cout << "y = " << y << '\n';
    
    std::cout << "Hello, World!\n";
    return 0;
}
