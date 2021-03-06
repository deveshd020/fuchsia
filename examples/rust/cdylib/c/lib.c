// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "examples/rust/cdylib/rust/crust.h"

void print_msg(void) {
  // Call the method from the Rust crate.
  printf("rust says: '%d'\n", crust_get_int());
}
