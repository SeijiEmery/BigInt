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

#define ENABLE_UNITTESTS
//#define UNITTEST_REPORT_ON_SUCCESS
#include "unittest.hpp"

namespace storage {

constexpr size_t STORAGE_BITS = 16;
typedef uint16_t smallInt_t;  // BigInt storage type
typedef uint32_t bigInt_t;    // BigInt operation type (mul + add ops need 2x precision for overflow)
    
constexpr size_t LOW_BITMASK = (1L << STORAGE_BITS) - 1;
constexpr size_t HIGH_BITMASK = ~LOW_BITMASK;

bigInt_t fromIntParts (smallInt_t high, smallInt_t low) {
    return ((bigInt_t)high << STORAGE_BITS) | (bigInt_t)low;
}
void storeIntParts (bigInt_t v, smallInt_t& high, smallInt_t& low) {
    high = (smallInt_t)(v & HIGH_BITMASK);
    low  = (smallInt_t)(v & LOW_BITMASK);
}
                   
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
    
    static UNITTEST_MAIN_METHOD(BigInt) {
        RUN_UNITTEST(initFromString,   UNITTEST_INSTANCE);
        RUN_UNITTEST(pushDecimalDigit, UNITTEST_INSTANCE);
        RUN_UNITTEST(scalarAdd,        UNITTEST_INSTANCE);
        RUN_UNITTEST(scalarMul,        UNITTEST_INSTANCE);
        RUN_UNITTEST(scalarDiv,        UNITTEST_INSTANCE);
        RUN_UNITTEST(writeString,      UNITTEST_INSTANCE);
    } UNITTEST_END_METHOD
    
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
//        TEST_ASSERT(false, "Should fail");
        TEST_ASSERT(true, "Should succeed");
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
        TEST_ASSERT_EQ(2 + 2, 4, "2 + 2 = 5?");
    } UNITTEST_END_METHOD
    
    BigInt& operator+= (storage::smallInt_t v) {
        for (auto i = 0; v && i < sections.size(); ++i) {
            auto p = sections[i] + v;
            
            storage::storeIntParts(p, v, sections[i]);
//            sections[i] = p & SECTION_BITMASK_LOW;
//            v           = p & SECTION_BITMASK_HIGH;
        }
        // If carry value remaining, push back as a new section "digit cluster"
        if (v) sections.push_back(v);
        return *this;
    }
    static UNITTEST_METHOD(scalarAdd) {
    
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
};

std::ostream& operator << (std::ostream& os, BigInt& v) {
    return os << v.toString();
}

UNITTEST_METHOD(runAllTests) {
    RUN_UNITTEST_MAIN(BigInt, UNITTEST_INSTANCE);
    // more tests to go here...
    
} UNITTEST_END_METHOD

int main(int argc, const char * argv[]) {
    if (!RUN_UNITTEST(runAllTests))
        return -1;
    
    auto x = BigInt { "-123456789" };
    auto y = BigInt { "2" };
    
    std::cout << "x = " << x << '\n';
    std::cout << "y = " << y << '\n';
    
    std::cout << "Hello, World!\n";
    return 0;
}
