/*
 * Copyright (C) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#ifndef PageGroupIndexedDatabase_h
#define PageGroupIndexedDatabase_h

#if ENABLE(INDEXED_DATABASE)

#include "Supplementable.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

class IDBFactoryBackendInterface;
class PageGroup;

class PageGroupIndexedDatabase : public Supplement<PageGroup> {
public:
    explicit PageGroupIndexedDatabase(const String& databaseDirectoryIdentifier);
    virtual ~PageGroupIndexedDatabase();

    static PageGroupIndexedDatabase* from(PageGroup&);

    IDBFactoryBackendInterface* factoryBackend();

private:
    static const char* supplementName();

    String m_databaseDirectoryIdentifier;
    RefPtr<IDBFactoryBackendInterface> m_factoryBackend;
};

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)

#endif // PageGroupIndexedDatabase_h
