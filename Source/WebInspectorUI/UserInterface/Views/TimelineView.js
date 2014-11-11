/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

WebInspector.TimelineView = function(representedObject)
{
    if (this.constructor === WebInspector.TimelineView) {
        // When instantiated directly return an instance of a type-based concrete subclass.

        console.assert(representedObject && representedObject instanceof WebInspector.Timeline);

        var timelineType = representedObject.type;
        if (timelineType === WebInspector.TimelineRecord.Type.Network)
            return new WebInspector.NetworkTimelineView(representedObject);

        if (timelineType === WebInspector.TimelineRecord.Type.Layout)
            return new WebInspector.LayoutTimelineView(representedObject);

        if (timelineType === WebInspector.TimelineRecord.Type.Script)
            return new WebInspector.ScriptTimelineView(representedObject);

        throw Error("Can't make a Timeline for an unknown representedObject.");
    }

    // Concrete object instantiation.
    console.assert(this.constructor !== WebInspector.TimelineView && this instanceof WebInspector.TimelineView);

    WebInspector.Object.call(this);

    console.assert(representedObject instanceof WebInspector.Timeline || representedObject instanceof WebInspector.TimelineRecording);
    this._representedObject = representedObject;

    this._contentTreeOutline = WebInspector.timelineSidebarPanel.createContentTreeOutline();

    this.element = document.createElement("div");
    this.element.classList.add(WebInspector.TimelineView.StyleClassName);

    this._zeroTime = 0;
    this._startTime = 0;
    this._endTime = 5;
    this._currentTime = 0;
};

WebInspector.TimelineView.StyleClassName = "timeline-view";

WebInspector.TimelineView.Event = {
    SelectionPathComponentsDidChange: "timeline-view-selection-path-components-did-change"
};

WebInspector.TimelineView.prototype = {
    constructor: WebInspector.TimelineView,
    __proto__: WebInspector.Object.prototype,

    // Public

    get representedObject()
    {
        return this._representedObject;
    },

    get navigationSidebarTreeOutline()
    {
        return this._contentTreeOutline;
    },

    get navigationSidebarTreeOutlineLabel()
    {
        // Implemented by sub-classes if needed.
        return null;
    },

    get selectionPathComponents()
    {
        if (!this._contentTreeOutline.selectedTreeElement || this._contentTreeOutline.selectedTreeElement.hidden)
            return null;

        var pathComponent = new WebInspector.GeneralTreeElementPathComponent(this._contentTreeOutline.selectedTreeElement);
        pathComponent.addEventListener(WebInspector.HierarchicalPathComponent.Event.SiblingWasSelected, this.treeElementPathComponentSelected, this);
        return [pathComponent];
    },

    get zeroTime()
    {
        return this._zeroTime;
    },

    set zeroTime(x)
    {
        if (this._zeroTime === x)
            return;

        this._zeroTime = x || 0;

        this.needsLayout();
    },

    get startTime()
    {
        return this._startTime;
    },

    set startTime(x)
    {
        if (this._startTime === x)
            return;

        this._startTime = x || 0;

        this.needsLayout();
    },

    get endTime()
    {
        return this._endTime;
    },

    set endTime(x)
    {
        if (this._endTime === x)
            return;

        this._endTime = x || 0;

        this.needsLayout();
    },

    get currentTime()
    {
        return this._currentTime;
    },

    set currentTime(x)
    {
        if (this._currentTime === x)
            return;

        var oldCurrentTime = this._currentTime;

        this._currentTime = x || 0;

        function checkIfLayoutIsNeeded(currentTime)
        {
            // Include some wiggle room since the current time markers can be clipped off the ends a bit and still partially visible.
            const wiggleTime = 0.05; // 50ms
            return this._startTime - wiggleTime <= currentTime && currentTime <= this._endTime + wiggleTime;
        }

        if (checkIfLayoutIsNeeded.call(this, oldCurrentTime) || checkIfLayoutIsNeeded.call(this, this._currentTime))
            this.needsLayout();
    },

    get visible()
    {
        return this._visible;
    },

    reset: function()
    {
        this._contentTreeOutline.removeChildren();
    },

    shown: function()
    {
        this._visible = true;

        // Implemented by sub-classes if needed.
    },

    hidden: function()
    {
        // Implemented by sub-classes if needed.

        this._visible = false;
    },

    matchTreeElementAgainstCustomFilters: function(treeElement)
    {
        // Implemented by sub-classes if needed.
        return true;
    },

    updateLayout: function()
    {
        if (this._scheduledLayoutUpdateIdentifier) {
            cancelAnimationFrame(this._scheduledLayoutUpdateIdentifier);
            delete this._scheduledLayoutUpdateIdentifier;
        }

        // Implemented by sub-classes if needed.
    },

    updateLayoutIfNeeded: function()
    {
        if (!this._scheduledLayoutUpdateIdentifier)
            return;
        this.updateLayout();
    },

    // Protected

    treeElementPathComponentSelected: function(event)
    {
        // Implemented by sub-classes if needed.
    },

    needsLayout: function()
    {
        if (!this._visible)
            return;

        if (this._scheduledLayoutUpdateIdentifier)
            return;

        this._scheduledLayoutUpdateIdentifier = requestAnimationFrame(this.updateLayout.bind(this));
    }
};
