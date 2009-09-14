/*
 * Copyright (c) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "LocalizedStrings.h"
#include "NotImplemented.h"
#include "PlatformString.h"

namespace WebCore {

String submitButtonDefaultLabel()
{
    return String("Submit");
}

String inputElementAltText()
{
    return String("Submit");
}

String resetButtonDefaultLabel()
{
    return String("Reset");
}

String searchableIndexIntroduction()
{
    return String("Searchable Index");
}

String fileButtonChooseFileLabel()
{
    return String("Choose File");
}

String fileButtonNoFileSelectedLabel()
{
    return String("No file selected");
}

String contextMenuItemTagOpenLinkInNewWindow()
{
    return String("Open Link in New Window");
}

String contextMenuItemTagDownloadLinkToDisk()
{
    return String("Download Link to Disk");
}

String contextMenuItemTagCopyLinkToClipboard()
{
    return String("Copy Link to Clipboard");
}

String contextMenuItemTagOpenImageInNewWindow()
{
    return String("Open Image in New Window");
}

String contextMenuItemTagDownloadImageToDisk()
{
    return String("Download Image to Disk");
}

String contextMenuItemTagCopyImageToClipboard()
{
    return String("Copy Image to Clipboard");
}

String contextMenuItemTagOpenFrameInNewWindow()
{
    return String("Open Frame in New Window");
}

String contextMenuItemTagCopy()
{
    return String("Copy");
}

String contextMenuItemTagGoBack()
{
    return String("Go Back");
}

String contextMenuItemTagGoForward()
{
    return String("Go Forward");
}

String contextMenuItemTagStop()
{
    return String("Stop");
}

String contextMenuItemTagReload()
{
    return String("Reload");
}

String contextMenuItemTagCut()
{
    return String("Cut");
}

String contextMenuItemTagPaste()
{
    return String("Paste");
}

String contextMenuItemTagNoGuessesFound()
{
    return String("No Guesses Found");
}

String contextMenuItemTagIgnoreSpelling()
{
    return String("Ignore Spelling");
}

String contextMenuItemTagLearnSpelling()
{
    return String("Learn Spelling");
}

String contextMenuItemTagSearchWeb()
{
    return String("Search Web");
}

String contextMenuItemTagLookUpInDictionary()
{
    return String("Look Up in Dictionary");
}

String contextMenuItemTagOpenLink()
{
    return String("Open Link");
}

String contextMenuItemTagIgnoreGrammar()
{
    return String("Ignore Grammar");
}

String contextMenuItemTagSpellingMenu()
{
    return String("Spelling");
}

String contextMenuItemTagShowSpellingPanel(bool show)
{
    return String(show ? "Show Spelling and Grammar" : "Hide Spelling and Grammar");
}

String contextMenuItemTagCheckSpelling()
{
    return String("Check Spelling");
}

String contextMenuItemTagCheckSpellingWhileTyping()
{
    return String("Check Spelling while Typing");
}

String contextMenuItemTagCheckGrammarWithSpelling()
{
    return String("Check Grammar with Spelling");
}

String contextMenuItemTagFontMenu()
{
    return String("Font");
}

String contextMenuItemTagBold()
{
    return String("Bold");
}

String contextMenuItemTagItalic()
{
    return String("Italic");
}

String contextMenuItemTagUnderline()
{
    return String("Underline");
}

String contextMenuItemTagOutline()
{
    return String("Outline");
}

String contextMenuItemTagWritingDirectionMenu()
{
    return String("Direction");
}

String contextMenuItemTagTextDirectionMenu()
{
    return String("Text Direction");
}

String contextMenuItemTagDefaultDirection()
{
    return String("Default Direction");
}

String contextMenuItemTagLeftToRight()
{
    return String("Left to Right");
}

String contextMenuItemTagRightToLeft()
{
    return String("Right to Left");
}

String contextMenuItemTagInspectElement()
{
    return String("Inspect Element");
}

String searchMenuNoRecentSearchesText()
{
    return String("No recent searches");
}

String searchMenuRecentSearchesText()
{
    return String("Recent searches");
}

String searchMenuClearRecentSearchesText()
{
    return String("Clear recent searches");
}

String AXDefinitionListTermText()
{
    return String("term");
}

String AXDefinitionListDefinitionText()
{
    return String("definition");
}

String AXButtonActionVerb()
{
    return String("press");
}

String AXRadioButtonActionVerb()
{
    return String("select");
}

String AXTextFieldActionVerb()
{
    return String("activate");
}

String AXCheckedCheckBoxActionVerb()
{
    return String("uncheck");
}

String AXUncheckedCheckBoxActionVerb()
{
    return String("check");
}

String AXLinkActionVerb()
{
    return String("jump");
}

String multipleFileUploadText(unsigned numberOfFiles)
{
    return String::number(numberOfFiles) + String(" files");
}

String unknownFileSizeText()
{
    return String("Unknown");
}

String imageTitle(const String& filename, const IntSize& size)
{
    notImplemented();
    return filename;
}

} // namespace WebCore
