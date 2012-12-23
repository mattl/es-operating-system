/*
 * Copyright 2011 Esrille Inc.
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

#include "RadioNodeListImp.h"

namespace org
{
namespace w3c
{
namespace dom
{
namespace bootstrap
{

RadioNodeListImp::RadioNodeListImp()
{
}

RadioNodeListImp::~RadioNodeListImp()
{
    list.clear();
}

void RadioNodeListImp::addItem(Node item)
{
    list.push_back(item);
}

Node RadioNodeListImp::item(unsigned int index)
{
    if (list.size() <= index)
        return 0;
    return list[index];
}

unsigned int RadioNodeListImp::getLength()
{
    return list.size();
}

std::u16string RadioNodeListImp::getValue()
{
    // TODO: implement me!
    return u"";
}

void RadioNodeListImp::setValue(const std::u16string& value)
{
    // TODO: implement me!
}

}
}
}
}
