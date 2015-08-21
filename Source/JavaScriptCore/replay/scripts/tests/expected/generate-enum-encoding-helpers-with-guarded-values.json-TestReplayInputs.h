/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// DO NOT EDIT THIS FILE. It is automatically generated from generate-enum-encoding-helpers-with-guarded-values.json
// by the script: JavaScriptCore/replay/scripts/CodeGeneratorReplayInputs.py

#ifndef generate_enum_encoding_helpers_with_guarded_values_json_TestReplayInputs_h
#define generate_enum_encoding_helpers_with_guarded_values_json_TestReplayInputs_h

#if ENABLE(WEB_REPLAY)
#include "InternalNamespaceHeaderIncludeDummy.h"
#include <platform/ExternalNamespaceHeaderIncludeDummy.h>

namespace WebCore {
enum MouseButton : unsigned;
}


namespace Test {
class SavedMouseButton;
} // namespace Test

namespace JSC {
template<> struct TEST_EXPORT_MACRO InputTraits<Test::SavedMouseButton> {
    static InputQueue queue() { return InputQueue::ScriptMemoizedData; }
    static const String& type();

    static void encode(JSC::EncodedValue&, const Test::SavedMouseButton&);
    static bool decode(JSC::EncodedValue&, std::unique_ptr<Test::SavedMouseButton>&);
};

} // namespace JSC

namespace Test {
class SavedMouseButton : public NondeterministicInput<SavedMouseButton> {
public:
    TEST_EXPORT_MACRO SavedMouseButton(MouseButton button);
    virtual ~SavedMouseButton();

    MouseButton button() const { return m_button; }
private:
    MouseButton m_button;
};
} // namespace Test

SPECIALIZE_TYPE_TRAITS_BEGIN(Test::SavedMouseButton)
    static bool isType(const NondeterministicInputBase& input) { return input.type() == InputTraits<Test::SavedMouseButton>::type(); }
SPECIALIZE_TYPE_TRAITS_END()

#define TEST_REPLAY_INPUT_NAMES_FOR_EACH(macro) \
    macro(SavedMouseButton) \
    \
// end of TEST_REPLAY_INPUT_NAMES_FOR_EACH

#endif // ENABLE(WEB_REPLAY)

#endif // generate-enum-encoding-helpers-with-guarded-values.json-TestReplayInputs_h
