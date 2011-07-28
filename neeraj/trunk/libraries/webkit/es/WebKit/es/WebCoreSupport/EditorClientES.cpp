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
#include "EditorClientES.h"

#include "EditCommand.h"
#include "Editor.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameView.h"
#include "KeyboardEvent.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformString.h"
#include "SelectionController.h"

namespace WebCore {

EditorClientES::~EditorClientES()
{
}

void EditorClientES::pageDestroyed()
{
    delete this;
}

bool EditorClientES::shouldDeleteRange(Range*)
{
    notImplemented();
    return true;
}

bool EditorClientES::shouldShowDeleteInterface(HTMLElement*)
{
    notImplemented();
    return false;
}

bool EditorClientES::smartInsertDeleteEnabled()
{
    notImplemented();
    return false;
}

bool EditorClientES::isSelectTrailingWhitespaceEnabled()
{
    notImplemented();
    return false;
}

bool EditorClientES::isContinuousSpellCheckingEnabled()
{
    notImplemented();
    return false;
}

void EditorClientES::toggleContinuousSpellChecking()
{
    notImplemented();
}

bool EditorClientES::isGrammarCheckingEnabled()
{
    notImplemented();
    return false;
}

void EditorClientES::toggleGrammarChecking()
{
    notImplemented();
}

int EditorClientES::spellCheckerDocumentTag()
{
    notImplemented();
    return 0;
}

bool EditorClientES::isEditable()
{
    notImplemented();
    return false;
}

bool EditorClientES::shouldBeginEditing(Range*)
{
    notImplemented();
    return true;
}

bool EditorClientES::shouldEndEditing(Range*)
{
    notImplemented();
    return true;
}

bool EditorClientES::shouldInsertNode(Node*, Range*, EditorInsertAction)
{
    notImplemented();
    return true;
}

bool EditorClientES::shouldInsertText(const String&, Range*, EditorInsertAction)
{
    notImplemented();
    return true;
}

bool EditorClientES::shouldApplyStyle(CSSStyleDeclaration*,
                                      Range*)
{
    notImplemented();
    return true;
}

bool EditorClientES::shouldMoveRangeAfterDelete(Range*, Range*)
{
    notImplemented();
    return true;
}

bool EditorClientES::shouldChangeSelectedRange(Range* fromRange, Range* toRange,
                                EAffinity, bool stillSelecting)
{
    notImplemented();
    return true;
}

void EditorClientES::didBeginEditing()
{
    notImplemented();
}

void EditorClientES::respondToChangedContents()
{
    notImplemented();
}

void EditorClientES::didEndEditing()
{
    notImplemented();
}

void EditorClientES::didWriteSelectionToPasteboard()
{
    notImplemented();
}

void EditorClientES::didSetSelectionTypesForPasteboard()
{
    notImplemented();
}

void EditorClientES::registerCommandForUndo(PassRefPtr<EditCommand> command)
{
    notImplemented();
}

void EditorClientES::registerCommandForRedo(PassRefPtr<EditCommand> command)
{
    notImplemented();
}

void EditorClientES::clearUndoRedoOperations()
{
    notImplemented();
}

bool EditorClientES::canUndo() const
{
    notImplemented();
    return false;
}

bool EditorClientES::canRedo() const
{
    notImplemented();
    return false;
}

void EditorClientES::undo()
{
    notImplemented();
}

void EditorClientES::redo()
{
    notImplemented();
}

void EditorClientES::handleInputMethodKeydown(KeyboardEvent* event)
{
    notImplemented();
}

void EditorClientES::handleKeyboardEvent(KeyboardEvent* event)
{
    notImplemented();
}

void EditorClientES::textFieldDidBeginEditing(Element*)
{
    notImplemented();
}

void EditorClientES::textFieldDidEndEditing(Element*)
{
    notImplemented();
}

void EditorClientES::textDidChangeInTextField(Element*)
{
    notImplemented();
}

bool EditorClientES::doTextFieldCommandFromEvent(Element*, KeyboardEvent*)
{
    notImplemented();
    return false;
}

void EditorClientES::textWillBeDeletedInTextField(Element*)
{
    notImplemented();
}

void EditorClientES::textDidChangeInTextArea(Element*)
{
    notImplemented();
}

void EditorClientES::respondToChangedSelection()
{
    notImplemented();
}

void EditorClientES::ignoreWordInSpellDocument(const String&)
{
    notImplemented();
}

void EditorClientES::learnWord(const String&)
{
    notImplemented();
}

void EditorClientES::checkSpellingOfString(const UChar*, int length, int* misspellingLocation, int* misspellingLength)
{
    notImplemented();
}

void EditorClientES::checkGrammarOfString(const UChar*, int length, Vector<GrammarDetail>&, int* badGrammarLocation, int* badGrammarLength)
{
    notImplemented();
}

void EditorClientES::updateSpellingUIWithGrammarString(const String&, const GrammarDetail& detail)
{
    notImplemented();
}

void EditorClientES::updateSpellingUIWithMisspelledWord(const String&)
{
    notImplemented();
}

void EditorClientES::showSpellingUI(bool show)
{
    notImplemented();
}

bool EditorClientES::spellingUIIsShowing()
{
    notImplemented();
    return false;
}

void EditorClientES::getGuessesForWord(const String&, Vector<String>& guesses)
{
    notImplemented();
}

String EditorClientES::getAutoCorrectSuggestionForMisspelledWord(const WebCore::String&)
{
    notImplemented();
    return String();
}

void EditorClientES::setInputMethodState(bool enabled)
{
    notImplemented();
}

} // namespace WebCore
