/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
 * Copyright (C) Saam Barati.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.TypeSet = function(runtimeTypeDescriptionPayload)
{
    this._types = runtimeTypeDescriptionPayload;

    var typeSet = this._types.typeSet;
    var bitString = 0x0;
    if (typeSet.isFunction)
        bitString |= WebInspector.TypeSet.TypeBit.Function;
    if (typeSet.isUndefined)
        bitString |= WebInspector.TypeSet.TypeBit.Undefined;
    if (typeSet.isNull)
        bitString |= WebInspector.TypeSet.TypeBit.Null;
    if (typeSet.isBoolean)
        bitString |= WebInspector.TypeSet.TypeBit.Boolean;
    if (typeSet.isInteger)
        bitString |= WebInspector.TypeSet.TypeBit.Integer;
    if (typeSet.isNumber)
        bitString |= WebInspector.TypeSet.TypeBit.Number;
    if (typeSet.isString)
        bitString |= WebInspector.TypeSet.TypeBit.String;
    if (typeSet.isObject)
        bitString |= WebInspector.TypeSet.TypeBit.Object;

    console.assert(bitString);
    this._bitString = bitString;

    this._primitiveTypeNames = null;
};

WebInspector.TypeSet.fromPayload = function(payload)
{
    return new WebInspector.TypeSet(payload);
};

WebInspector.TypeSet.TypeBit = {
    "Function"    :  0x1,
    "Undefined"   :  0x2,
    "Null"        :  0x4,
    "Boolean"     :  0x8,
    "Integer"     :  0x10,
    "Number"      :  0x20,
    "String"      :  0x40,
    "Object"      :  0x80
};

WebInspector.TypeSet.NullOrUndefinedTypeBits = WebInspector.TypeSet.TypeBit.Null | WebInspector.TypeSet.TypeBit.Undefined;

WebInspector.TypeSet.prototype = {
    constructor: WebInspector.TypeSet,
    __proto__: WebInspector.Object.prototype,

    isContainedIn: function(test)
    {
        // This function checks if types in bitString are contained in the types described by the 'test' bitstring. (i.e we haven't seen more types than 'test').
        // We have seen fewer or equal number of types as 'test' if ANDing bitString with test doesn't zero out any of our bits.

        // For example:

        // 0b0110 (bitString)
        // 0b1111 (test)
        // ------ (AND)
        // 0b0110 == bitString

        // 0b0110 (bitString)
        // 0b0010 (test)
        // ------ (AND)
        // 0b0010 != bitString

        return this._bitString && (this._bitString & test) === this._bitString;
    },

    get primitiveTypeNames()
    {
        if (this._primitiveTypeNames)
            return this._primitiveTypeNames;

        this._primitiveTypeNames = [];
        var typeSet = this._types.typeSet;
        if (typeSet.isUndefined)
            this._primitiveTypeNames.push("Undefined");
        if (typeSet.isNull)
            this._primitiveTypeNames.push("Null");
        if (typeSet.isBoolean)
            this._primitiveTypeNames.push("Boolean");
        if (typeSet.isString)
            this._primitiveTypeNames.push("String");

        // It's implied that type Integer is contained in type Number. Don't put 
        // both 'Integer' and 'Number' into the set because this could imply that 
        // Number means to Double instead of Double|Integer.
        if (typeSet.isNumber)
            this._primitiveTypeNames.push("Number");
        else if (typeSet.isInteger)
            this._primitiveTypeNames.push("Integer");

        return this._primitiveTypeNames;
    }
};
