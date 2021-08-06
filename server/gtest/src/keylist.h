/*
 * Testing the hipe server keylist.
 */
// Prefix definition with 'T' to separate from hipe server's
// keylist files.
#ifndef TKEYLIST_H
#define TKEYLIST_H

#include <gtest/gtest.h>
// Want to test a private function so make it public.
#define private public
#include "../../src/keylist.h"
#undef private

#endif
