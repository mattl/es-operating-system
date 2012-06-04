/*
 * Copyright 2012 Esrille Inc.
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

#include "Profile.h"

#include <iostream>

int main(int argc, char* argv[])
{
    Profile profile("profile");
    std::cout << "Profile: " << profile.hasError() << '\n';

    profile.createDirectory("cache");
    std::cout << "createDirectory: " << profile.hasError() << '\n';

    std::cout << "Push any key to quit: ";
    std::string input;
    std::getline(std::cin, input);
    return profile.hasError();
}
