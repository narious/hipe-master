#include "keylist.h"

#define NCHRS 64
// A string of all the characters that can be mapped to.
static const char *CHRS = "0123456789"
			  "ABCDEFGHIJKLMNOPQRSTUVWXYZ_"
			  "abcdefghijklmnopqrstuvwxyz~";

TEST(keylist, map6bit) {
	KeyList k { "foo" };

	for (int i = 0; i < NCHRS; ++i)
		EXPECT_EQ(k.map6bitToAlphaNumeric(i), CHRS[i]);
}

