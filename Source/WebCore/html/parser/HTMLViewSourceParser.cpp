/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "HTMLViewSourceParser.h"

#include "HTMLDocumentParser.h"
#include "HTMLNames.h"
#include "AtomicHTMLToken.h" //Only for cdos browser

namespace WebCore {

HTMLViewSourceParser::HTMLViewSourceParser(HTMLViewSourceDocument& document)
    : DecodedDataDocumentParser(document)
    , m_tokenizer(std::make_unique<HTMLTokenizer>(HTMLParserOptions(document)))
{
}

HTMLViewSourceParser::~HTMLViewSourceParser()
{
}

void HTMLViewSourceParser::insert(const SegmentedString&)
{
    ASSERT_NOT_REACHED();
}

void HTMLViewSourceParser::pumpTokenizer()
{
    while (true) {
        m_sourceTracker.startToken(m_input.current(), *(m_tokenizer.get()));
        auto rawToken = m_tokenizer->nextToken(m_input.current());
        if (!rawToken)
            break;

        m_sourceTracker.endToken(m_input.current(), *(m_tokenizer.get()));

        AtomicHTMLToken token(*rawToken);
        document()->addSource(sourceForToken(token), *rawToken);
        updateTokenizerState(token);
        m_token.clear();
        rawToken->clear();
    }
}

void HTMLViewSourceParser::append(PassRefPtr<StringImpl> input)
{
    m_input.appendToEnd(String(input));
    pumpTokenizer();
}

String HTMLViewSourceParser::sourceForToken(AtomicHTMLToken& token)
{
    return m_sourceTracker.source(token);
}

void HTMLViewSourceParser::updateTokenizerState(AtomicHTMLToken& token)
{
    // FIXME: The tokenizer should do this work for us.
    if (token.type() != HTMLToken::StartTag)
        return;
    m_tokenizer->updateStateFor(AtomicString(token.name()));
}

void HTMLViewSourceParser::finish()
{
    if (!m_input.haveSeenEndOfFile())
        m_input.markEndOfFile();
    pumpTokenizer();
    document()->finishedParsing();
}

}
