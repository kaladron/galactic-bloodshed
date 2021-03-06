// Copyright 2014 The Galactic Bloodshed Authors. All rights reserved.
// Use of this source code is governed by a license that can be
// found in the COPYING file.

/**
 * Different name generator implementations.
 */

import std;

#include "gb/creator/namegen.h"

bool SequentialNameGenerator::next() {
  std::ostringstream out;
  out << prefix << currval << suffix;
  currval++;
  current_value = out.str();
  return true;
}

bool IterativeNameGenerator::next() {
  current_value = "";
  if (head == tail) {
    return false;
  }
  current_value = *head;
  ++head;
  return true;
}
