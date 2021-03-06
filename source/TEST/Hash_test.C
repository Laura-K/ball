// -*- Mode: C++; tab-width: 2; -*-
// vi: set ts=2:
//

#include <BALL/CONCEPT/classTest.h>

///////////////////////////
#include <BALL/COMMON/hash.h>
///////////////////////////

START_TEST(Hash)

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

using namespace BALL;
char* c = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
string s1 = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
String s2 = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

CHECK([EXTRA] HashIndexhashString(const char* str))
	TEST_EQUAL(hashString(c), 140)
RESULT

CHECK([EXTRA] hashPJWString(const char* str))
	TEST_EQUAL(hashPJWString(c), 1450744505)
RESULT

CHECK([EXTRA] hashElfString(const char* str))
	TEST_EQUAL(hashElfString(c), 19269177)
RESULT

CHECK([EXTRA] Hash(const T& key))
	TEST_EQUAL(Hash(1), 1)
RESULT

CHECK([EXTRA] HashIndex Hash(const string& s))
	TEST_EQUAL(Hash(s1), 140)
RESULT

CHECK([EXTRA] HashIndex Hash(const String& s))
	TEST_EQUAL(Hash(s2), 140)
RESULT

CHECK([EXTRA] HashIndex Hash(void *const& ptr))
	void* x = 0;
	TEST_EQUAL(Hash(x), 0)
	x = (void*)123;
	TEST_EQUAL(Hash(x), 106040973)
RESULT

CHECK([EXTRA] HashIndex getNextPrime(HashIndex l))
	TEST_EQUAL(getNextPrime(0), 3)
	TEST_EQUAL(getNextPrime(1), 3)
	TEST_EQUAL(getNextPrime(2), 3)
	TEST_EQUAL(getNextPrime(3), 3)
	TEST_EQUAL(getNextPrime(4), 5)
	TEST_EQUAL(getNextPrime(5), 5)
RESULT

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
END_TEST
