// SPDX-License-Identifier: Apache-2.0

export module gblib:rand;

export void seed_rand(unsigned int seed);
export bool success(int x);
export double double_rand();
export int int_rand(int low, int high);
export long long_rand(long low, long high);
export int round_rand(double);
