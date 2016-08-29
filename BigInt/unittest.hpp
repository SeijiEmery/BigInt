//
//  unittest.hpp
//
//  Created by semery on 8/25/16.
//  Copyright © 2016 Seiji Emery. All rights reserved.
//

#ifndef __unittest_hpp__
#define __unittest_hpp__

// Mini unit-testing API
//
// ENABLE_UNITTESTS (#define 1 / 0)
//   enable / disable unit testing.
//   If disabled, no unit testing code will be generated except testing stubs with your user code,
//   which will be unused and should get eliminated by the linker.
//   I'm assuming that we can't wrap a macro expansion in #ifdef, or we would use that to eliminate
//   all unittest code.
//
// UNITTEST_REPORT_ON_SUCCESS (#define 1 / 0)
//   If enabled, prints "All tests passed." when a test case passes; otherwise, is silent and
//   only prints when a testcase contained errors. Prints to std::cerr in either case.
//
//
// UNITTEST_METHOD(name)
//   define a method unittest_##name with the appropriate unittesting scaffold.
//
// UNITTEST_MAIN_METHOD(name)
//   same as UNITTEST_METHOD, but with the function name 'unittest', not 'unittest_##name'.
//   The intended usage is to define a main testing method for a class or global file / module,
//   named (so name shows up in the unit testing log), but so you can just call unittest() /
//   MyClass::unittest().
//   eg.
//     class Foo {
//         static UNITTEST_MAIN_METHOD(Foo)
//             RUN_UNITTEST(failTest, UNITTEST_INSTANCE);
//         UNITTEST_END_METHOD
//
//         static UNITTEST_METHOD(failTest)
//              TEST_ASSERT(1, "should succeed");
//              TEST_ASSERT_EQ(1, 2, "should fail");
//         UNITTEST_END_METHOD
//     };
//
//     int main () {
//         return RUN_UNITTEST_MAIN(Foo) ? 0 : -1;
//     }
//     // =>
//     //   Assertion Failed: /path/to/somefile.cpp:12: 1 != 2 should fail
//     //   Unittest FAILED: failTest: 1 / 2 tests passed.
//     //   Unittest FAILED: Foo::unittest: 0 / 1 tests passed.
//
//
// UNITTEST_END_METHOD
//   end a method block started with UNITTEST_METHOD.
//
// RUN_UNITTEST(name, [bool printResults = true])
//   executes the unittest unittest_##name, and returns true/false if all tests within
//   that method passed. If printResults is true, prints results to std::cerr.
//
// RUN_UNITTEST(name, testContext, [bool printResults = true])
//   same as the above, but instead of r
//
// UNITTEST_INSTANCE
//   expands to the local testing instance if used in a UNITTEST_METHOD block,
//   usable by RUN_UNITTEST (will produce errors if used outside)
//
//
// TEST_ASSERT(bool cond, std::string msg, [const char* file = __FILE__], [size_t line = __LINE__])
//   assert that cond is true (if false, prints msg to std::cerr with file + line info,
//   and marks this unit test as failing (does _not_ terminate early)).
//   Usable only within a UNITTEST_METHOD block.
//
// TEST_ASSERT_EQ(a, b, std::string msg, [const char* file = __FILE__], [size_t line = __LINE__])
//   assert that a == b using operator== (a and b _may_ be different types).
//   If fails, writes values of a, b, msg, and file / line info to std::cerr, and marks this
//   unit test as failing (does _not_ terminate early).
//   Separate implementations are written for value types A, B and reference types A&, B&.
//   Usable only within a UNITTEST_METHOD block.
//

#ifndef ENABLE_UNITTESTS
#define ENABLE_UNITTESTS 1
#endif

#ifndef UNITTEST_REPORT_ON_SUCCESS
#define UNITTEST_REPORT_ON_SUCCESS 0
#endif

#if ENABLE_UNITTESTS

#include <iostream>

    struct UnitTest_Results {
        const char* name;
        unsigned passed = 0, attempted = 0;
        
        UnitTest_Results (const char* name) : name(name) {}
        
        bool assertThat (bool cond, std::string msg, const char* file, size_t line) {
            if (cond) return ++attempted, ++passed, true;
            else      return ++attempted, std::cerr << "Assertion Failed: " << file << ":" << line << ": " << msg << '\n', false;
        }
        bool assertThat (bool cond, const char* file, size_t line) {
            if (cond) return ++attempted, ++passed, true;
            else      return ++attempted, std::cerr << "Assertion Failed: " << file << ":" << line << '\n', false;
        }
        template <typename A, typename B>
        bool assertEq (const A& a, const B& b, std::string msg, const char* file, size_t line) {
            if (a == b) return ++attempted, ++passed, true;
            else        return ++attempted, std::cerr << "Assertion Failed: " << file << ":" << line << ": '"
                << a << "' != '" << b << "' " << msg << '\n', false;
        }
        template <typename A, typename B>
        bool assertEq (const A& a, const B& b, const char* file, size_t line) {
            if (a == b) return ++attempted, ++passed, true;
            else        return ++attempted, std::cerr << "Assertion Failed: " << file << ":" << line << ": '"
                << a << "' != '" << b << "'\n", false;
        }
        
        // Print results (optional), and collect results into another collection @testCollection.
        // (eg. collect multiple sub test results into a single test result. passed / attempted
        //  will be incremented by 1 for each collection that testCollection is called on, with
        //  passed incremented iff passed == attempted for that test).
        bool checkResults (UnitTest_Results& testCollection, bool printResults = true) {
            if (printResults) reportTestResults();
            
            ++testCollection.attempted;
            return passed == attempted ?
                (++testCollection.passed, true) :
                false;
        }
        // Print results (optional), and return whether all results
        bool checkResults (bool printResults = true) {
            if (printResults) reportTestResults();
            return passed == attempted;
        }
    private:
        void reportTestResults () {
            if (passed != attempted)
                std::cerr << "Unittest FAILED: " << name << ":\t"
                << passed << " / " << attempted << " tests passed.\n";
    #if UNITTEST_REPORT_ON_SUCCESS
            else
                std::cerr << "Unittest PASSED: " << name << ":\tAll tests passed.\n";
    #endif // UNITTEST_REPORT_ON_SUCCESS
        }
    };

    #define UNITTEST_MAIN_METHOD(name) \
    UnitTest_Results unittest () { \
    UnitTest_Results _testResults { #name };

    #define UNITTEST_METHOD(name) \
    UnitTest_Results unittest_##name () { \
    UnitTest_Results _testResults { #name };

    #define UNITTEST_END_METHOD \
    return _testResults; }

    #define RUN_UNITTEST(name, args...) \
    unittest_##name().checkResults(args)

    #define RUN_UNITTEST_MAIN(name, args...) \
    name::unittest().checkResults(args)

    #define UNITTEST_INSTANCE \
    (_testResults)

    #define TEST_ASSERT(args...) (_testResults.assertThat(args, __FILE__, __LINE__))
    #define TEST_ASSERT_EQ(args...) (_testResults.assertEq(args, __FILE__, __LINE__))

#else // ENABLE_UNITTESTS

    #define UNITTEST_MAIN_METHOD(name) void unittest () {
    #define UNITTEST_METHOD(name) void unittest_##name () {
    #define UNITTEST_END_METHOD }

    #define RUN_UNITTEST(args...) (true)
    #define RUN_UNITTEST_MAIN(args...) (true)
    #define UNITTEST_INSTANCE (0)

    #define TEST_ASSERT(args...)
    #define TEST_ASSERT_EQ(args...)

#endif // ENABLE_UNITTESTS

#endif /* __unittest_hpp__ */