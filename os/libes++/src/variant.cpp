/*
 * Copyright 2008 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <es/variant.h>

#ifdef __x86_64__

Variant apply(int argc, Variant* argv, Variant (*function)()){}
Variant apply(int argc, Variant* argv, bool (*function)()){}
Variant apply(int argc, Variant* argv, uint8_t (*function)()){}
Variant apply(int argc, Variant* argv, int16_t (*function)()){}
Variant apply(int argc, Variant* argv, uint16_t (*function)()){}
Variant apply(int argc, Variant* argv, int32_t (*function)()){}
Variant apply(int argc, Variant* argv, uint32_t (*function)()){}
Variant apply(int argc, Variant* argv, int64_t (*function)()){}
Variant apply(int argc, Variant* argv, uint64_t (*function)()){}
Variant apply(int argc, Variant* argv, float (*function)()){}
Variant apply(int argc, Variant* argv, double (*function)()){}
Variant apply(int argc, Variant* argv, const char* (*function)()){}
Variant apply(int argc, Variant* argv, es::IInterface* (*function)()){}

#endif  // __x86_64__
