#include "hinstr.h"

TEST(hipe_instruction, copy) {
	hipe_instruction src, dest;

	hipe_instruction_init(&src);
	hipe_instruction_init(&dest);

	src.arg_length[0] = 3;
	src.arg_length[1] = 5; 
	src.arg_length[2] = 1; 
	src.arg_length[3] = 3;
	// Allocates memory for string lengths provided above.
	hipe_instruction_alloc(&src);
	strncpy(src.arg[0], "foo", 3);
	strncpy(src.arg[1], "hello", 5);
	strncpy(src.arg[2], "a", 1);
	strncpy(src.arg[3], "bar", 3);

	hipe_instruction_copy(&dest, &src);

	for (int i = 0; i < HIPE_NARGS; ++i)
		EXPECT_EQ(strcmp(src.arg[i], dest.arg[i]), 0);

	hipe_instruction_clear(&src);
	hipe_instruction_clear(&dest);
}

TEST(hipe_instruction, move) {
	hipe_instruction src, dest;

	hipe_instruction_init(&src);
	hipe_instruction_init(&dest);

	src.arg_length[0] = 3;
	src.arg_length[1] = 5; 
	src.arg_length[3] = 3;
	// Allocates memory for string lengths provided above.
	hipe_instruction_alloc(&src);
	strncpy(src.arg[0], "foo", 3);
	strncpy(src.arg[1], "hello", 5);
	strncpy(src.arg[3], "bar", 3);

	hipe_instruction_move(&dest, &src);

	// All source strings got moved and reset so should now be null.
	for (int i = 0; i < HIPE_NARGS; ++i)
		// Compare directly with NULL rather than NULL as second parameter to
		// EXPECT_EQ because C++ doesn't like it.
		EXPECT_EQ(src.arg[i] == NULL, true);
	
	EXPECT_EQ(strcmp(dest.arg[0], "foo"), 0);
	EXPECT_EQ(strcmp(dest.arg[1], "hello"), 0);
	EXPECT_EQ(dest.arg[2] == NULL, true);
	EXPECT_EQ(strcmp(dest.arg[3], "bar"), 0);

	hipe_instruction_clear(&dest);
}

