# BigInt

This is a simple, unfinished bignum library that I wrote after figuring out some
basic algorithms to add and multiply arbitrary-size numbers. All of this is based
on paper calculations – I didn't just copy something from the internet – which is
why it's unfinished and probably not optimal.

This project demonstrates:
    – Algorithms for performing some arithmetic operations on arbitrary-size 
      integers, and efficient storage of those integers. 
    – Thorough unittesting
    – A small, minimalistic unittesting library (unittest.hpp), also posted here:
      https://gist.github.com/SeijiEmery/8351b7f41241b3eaa0b47b056d80008c

Features:
    – read / write to / from string
    – BigInt + i32
    – BigInt * i32 + i32
    – BigInt * i32
    – BigInt / i32
    – BigInt * BigInt
    – BigInt ** i32    (can calculate 2 ** (2 ** 15) very quickly, then do further ops)
    – comparison, zero-checking
    – negative numbers were not implemented properly
    – addition / subtraction of bignums were not implemented for some reason;
      this was an extracurricular project, so I ran out of time


On the unittesting library: This is *not* difficult to implement, and I thought
googletest, etc., were somewhat bloated; this library was designed to function 
similarly to D unittests, and is only a 187 line header file, including 
documentation. That said, I did take a bunch of shortcuts: messages are dumped
to std::cerr, it's not very configurable, doesn't run tests in parallel, etc.
