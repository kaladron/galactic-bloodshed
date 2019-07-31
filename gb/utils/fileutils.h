// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <cstdio>
#include <cstdlib>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

/**
 * Helper utils and classes for making common operations with files eaiser.
 */
void InitFile(const std::string &path,
              void *buffer = nullptr,
              size_t length = 0);
void EmptyFile(const std::string &path);

#endif  // MAKEPLANET_H
