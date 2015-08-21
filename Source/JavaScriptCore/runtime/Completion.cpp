/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2007, 2013 Apple Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "Completion.h"

#include "CallFrame.h"
#include "CodeProfiling.h"
#include "Debugger.h"
#include "Interpreter.h"
#include "JSGlobalObject.h"
#include "JSLock.h"
#include "JSCInlines.h"
#include "Parser.h"
#include <wtf/WTFThreadData.h>

bool m_ObfuscateCodeSate=false;  //Only for cdos browser

namespace JSC {

//Only for cdos browser
void setObfuscateCodeSate(bool obfuscateCodeSate)
{
   m_ObfuscateCodeSate = obfuscateCodeSate;
}

bool checkSyntax(ExecState* exec, const SourceCode& source, JSValue* returnedException)
{
    JSLockHolder lock(exec);
    RELEASE_ASSERT(exec->vm().atomicStringTable() == wtfThreadData().atomicStringTable());

    ProgramExecutable* program = ProgramExecutable::create(exec, source);
    JSObject* error = program->checkSyntax(exec);
    if (error) {
        if (returnedException)
            *returnedException = error;
        return false;
    }

    return true;
}
    
bool checkSyntax(VM& vm, const SourceCode& source, ParserError& error)
{
    JSLockHolder lock(vm);
    RELEASE_ASSERT(vm.atomicStringTable() == wtfThreadData().atomicStringTable());
    return !!parse<ProgramNode>(&vm, source, 0, Identifier(), JSParseNormal, JSParseProgramCode, error);
}

JSValue evaluate(ExecState* exec, const SourceCode& source, JSValue thisValue, JSValue* returnedException)
{
    JSLockHolder lock(exec);
    RELEASE_ASSERT(exec->vm().atomicStringTable() == wtfThreadData().atomicStringTable());
    RELEASE_ASSERT(!exec->vm().isCollectorBusy());

    CodeProfiling profile(source);

    ProgramExecutable* program = ProgramExecutable::create(exec, source);
    if (!program) {
        if (returnedException)
            *returnedException = exec->vm().exception();

        exec->vm().clearException();
        return jsUndefined();
    }

    if (!thisValue || thisValue.isUndefinedOrNull())
        thisValue = exec->vmEntryGlobalObject();
    JSObject* thisObj = jsCast<JSObject*>(thisValue.toThis(exec, NotStrictMode));
    /*****
      add by gwg 判断source是否为混淆代码
      混淆代码检测部分有两个值需要设定为可被用户配置的：
         熵的阈值【默认为1.1】
        字串长度阈值【默认为350】
    ***/
    if(m_ObfuscateCodeSate)
    {
       String code_str=source.toString();
       double count[128],sum=0.0,entropy=0.0;
       unsigned i,len=code_str.length(),wordsize=0,metric_size=0;  //长度
       UChar ch;
       memset(count,0,sizeof(double)*128);
       for(i=0;i<len;i++) {  
          ch=code_str.at(i);
	  if(ch<128) {
	     if(ch==32||ch==10) {
	        if(wordsize>350)  // 这里的350需要处理为可以由用户配置的值
		   metric_size=1;
		wordsize=0;
		count[ch]=count[ch]+1.0;
	     }
	     else {
	        count[ch]=count[ch]+1.0;
		wordsize=wordsize+1;
	     }
          }
       }
       if(wordsize>350)  // 这里的350需要处理为可以由用户配置的值
          metric_size=1;
       for(i=0;i<128;i++)
          sum=sum+count[i];
       for(i=0;i<128;i++) {
          if(count[i]==0.0) continue; 
	  entropy=entropy-(count[i]/sum)*log10(count[i]/sum);
       }
       if(entropy<1.1 && metric_size==1) //熵低于1.1且含有长度大于350字符word的认为是混淆代码  这里的1.1需要处理为可以由用户配置的值
	return jsUndefined();  //目前如果发现是可疑的混淆代码，则不对其进行解析执行，直接返回
    }
    /*****add by gwg*****/
    JSValue result = exec->interpreter()->execute(program, exec, thisObj);

    if (exec->hadException()) {
        if (returnedException)
            *returnedException = exec->exception();

        exec->clearException();
        return jsUndefined();
    }

    RELEASE_ASSERT(result);
    return result;
}

} // namespace JSC
