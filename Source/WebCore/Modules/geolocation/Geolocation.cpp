/*
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Torch Mobile, Inc.
 * Copyright 2010, The Android Open Source Project
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
#include "Geolocation.h"

#if ENABLE(GEOLOCATION)

#include "Document.h"
#include "Frame.h"
#include "Geoposition.h"
#include "Page.h"
#include <wtf/CurrentTime.h>
#include <wtf/Ref.h>

#include "Coordinates.h"
#include "GeolocationController.h"
#include "GeolocationError.h"
#include "GeolocationPosition.h"
#include "PositionError.h"

namespace WebCore {

static const char permissionDeniedErrorMessage[] = "User denied Geolocation";
static const char failedToStartServiceErrorMessage[] = "Failed to start Geolocation service";
static const char framelessDocumentErrorMessage[] = "Geolocation cannot be used in frameless documents";

static PassRefPtr<Geoposition> createGeoposition(GeolocationPosition* position)
{
    if (!position)
        return 0;
    
    RefPtr<Coordinates> coordinates = Coordinates::create(position->latitude(), position->longitude(), position->canProvideAltitude(), position->altitude(), 
                                                          position->accuracy(), position->canProvideAltitudeAccuracy(), position->altitudeAccuracy(),
                                                          position->canProvideHeading(), position->heading(), position->canProvideSpeed(), position->speed());
    return Geoposition::create(coordinates.release(), convertSecondsToDOMTimeStamp(position->timestamp()));
}

static PassRefPtr<PositionError> createPositionError(GeolocationError* error)
{
    PositionError::ErrorCode code = PositionError::POSITION_UNAVAILABLE;
    switch (error->code()) {
    case GeolocationError::PermissionDenied:
        code = PositionError::PERMISSION_DENIED;
        break;
    case GeolocationError::PositionUnavailable:
        code = PositionError::POSITION_UNAVAILABLE;
        break;
    }

    return PositionError::create(code, error->message());
}

Geolocation::GeoNotifier::GeoNotifier(Geolocation* geolocation, PassRefPtr<PositionCallback> successCallback, PassRefPtr<PositionErrorCallback> errorCallback, PassRefPtr<PositionOptions> options)
    : m_geolocation(geolocation)
    , m_successCallback(successCallback)
    , m_errorCallback(errorCallback)
    , m_options(options)
    , m_timer(*this, &Geolocation::GeoNotifier::timerFired)
    , m_useCachedPosition(false)
{
    ASSERT(m_geolocation);
    ASSERT(m_successCallback);
    // If no options were supplied from JS, we should have created a default set
    // of options in JSGeolocationCustom.cpp.
    ASSERT(m_options);
}

void Geolocation::GeoNotifier::setFatalError(PassRefPtr<PositionError> error)
{
    // If a fatal error has already been set, stick with it. This makes sure that
    // when permission is denied, this is the error reported, as required by the
    // spec.
    if (m_fatalError)
        return;

    m_fatalError = error;
    // An existing timer may not have a zero timeout.
    m_timer.stop();
    m_timer.startOneShot(0);
}

void Geolocation::GeoNotifier::setUseCachedPosition()
{
    m_useCachedPosition = true;
    m_timer.startOneShot(0);
}

bool Geolocation::GeoNotifier::hasZeroTimeout() const
{
    return m_options->hasTimeout() && m_options->timeout() == 0;
}

void Geolocation::GeoNotifier::runSuccessCallback(Geoposition* position)
{
    // If we are here and the Geolocation permission is not approved, something has
    // gone horribly wrong.
    if (!m_geolocation->isAllowed())
        CRASH();

    m_successCallback->handleEvent(position);
}

void Geolocation::GeoNotifier::runErrorCallback(PositionError* error)
{
    if (m_errorCallback)
        m_errorCallback->handleEvent(error);
}

void Geolocation::GeoNotifier::startTimerIfNeeded()
{
    if (m_options->hasTimeout())
        m_timer.startOneShot(m_options->timeout() / 1000.0);
}

void Geolocation::GeoNotifier::stopTimer()
{
    m_timer.stop();
}

void Geolocation::GeoNotifier::timerFired()
{
    m_timer.stop();

    // Protect this GeoNotifier object, since it
    // could be deleted by a call to clearWatch in a callback.
    Ref<GeoNotifier> protect(*this);

    // Test for fatal error first. This is required for the case where the Frame is
    // disconnected and requests are cancelled.
    if (m_fatalError) {
        runErrorCallback(m_fatalError.get());
        // This will cause this notifier to be deleted.
        m_geolocation->fatalErrorOccurred(this);
        return;
    }

    if (m_useCachedPosition) {
        // Clear the cached position flag in case this is a watch request, which
        // will continue to run.
        m_useCachedPosition = false;
        m_geolocation->requestUsesCachedPosition(this);
        return;
    }

    if (m_errorCallback) {
        RefPtr<PositionError> error = PositionError::create(PositionError::TIMEOUT, ASCIILiteral("Timeout expired"));
        m_errorCallback->handleEvent(error.get());
    }
    m_geolocation->requestTimedOut(this);
}

bool Geolocation::Watchers::add(int id, PassRefPtr<GeoNotifier> prpNotifier)
{
    ASSERT(id > 0);
    RefPtr<GeoNotifier> notifier = prpNotifier;

    if (!m_idToNotifierMap.add(id, notifier.get()).isNewEntry)
        return false;
    m_notifierToIdMap.set(notifier.release(), id);
    return true;
}

Geolocation::GeoNotifier* Geolocation::Watchers::find(int id)
{
    ASSERT(id > 0);
    return m_idToNotifierMap.get(id);
}

void Geolocation::Watchers::remove(int id)
{
    ASSERT(id > 0);
    if (auto notifier = m_idToNotifierMap.take(id))
        m_notifierToIdMap.remove(notifier);
}

void Geolocation::Watchers::remove(GeoNotifier* notifier)
{
    if (auto identifier = m_notifierToIdMap.take(notifier))
        m_idToNotifierMap.remove(identifier);
}

bool Geolocation::Watchers::contains(GeoNotifier* notifier) const
{
    return m_notifierToIdMap.contains(notifier);
}

void Geolocation::Watchers::clear()
{
    m_idToNotifierMap.clear();
    m_notifierToIdMap.clear();
}

bool Geolocation::Watchers::isEmpty() const
{
    return m_idToNotifierMap.isEmpty();
}

void Geolocation::Watchers::getNotifiersVector(GeoNotifierVector& copy) const
{
    copyValuesToVector(m_idToNotifierMap, copy);
}

Ref<Geolocation> Geolocation::create(ScriptExecutionContext* context)
{
    auto geolocation = adoptRef(*new Geolocation(context));
    geolocation.get().suspendIfNeeded();
    return geolocation;
}

Geolocation::Geolocation(ScriptExecutionContext* context)
    : ActiveDOMObject(context)
    , m_allowGeolocation(Unknown)
    , m_isSuspended(false)
    , m_hasChangedPosition(false)
    , m_resumeTimer(*this, &Geolocation::resumeTimerFired)
{
}

Geolocation::~Geolocation()
{
    ASSERT(m_allowGeolocation != InProgress);
}

Document* Geolocation::document() const
{
    return downcast<Document>(scriptExecutionContext());
}

Frame* Geolocation::frame() const
{
    return document() ? document()->frame() : nullptr;
}

Page* Geolocation::page() const
{
    return document() ? document()->page() : nullptr;
}

bool Geolocation::canSuspend() const
{
    return !hasListeners();
}

void Geolocation::suspend(ReasonForSuspension reason)
{
    // Allow pages that no longer have listeners to enter the page cache.
    // Have them stop updating and reset geolocation permissions when the page is resumed.
    if (reason == ActiveDOMObject::DocumentWillBecomeInactive) {
        ASSERT(!hasListeners());
        stop();
        m_resetOnResume = true;
    }

    // Suspend GeoNotifier timeout timers.
    if (hasListeners())
        stopTimers();

    m_isSuspended = true;
    m_resumeTimer.stop();
    ActiveDOMObject::suspend(reason);
}

void Geolocation::resume()
{
#if USE(WEB_THREAD)
    ASSERT(WebThreadIsLockedOrDisabled());
#endif
    ActiveDOMObject::resume();

    if (!m_resumeTimer.isActive())
        m_resumeTimer.startOneShot(0);
}

void Geolocation::resumeTimerFired()
{
    m_isSuspended = false;

    if (m_resetOnResume) {
        resetAllGeolocationPermission();
        m_resetOnResume = false;
    }

    // Resume GeoNotifier timeout timers.
    if (hasListeners()) {
        GeoNotifierSet::const_iterator end = m_oneShots.end();
        for (GeoNotifierSet::const_iterator it = m_oneShots.begin(); it != end; ++it)
            (*it)->startTimerIfNeeded();
        GeoNotifierVector watcherCopy;
        m_watchers.getNotifiersVector(watcherCopy);
        for (size_t i = 0; i < watcherCopy.size(); ++i)
            watcherCopy[i]->startTimerIfNeeded();
    }

    if ((isAllowed() || isDenied()) && !m_pendingForPermissionNotifiers.isEmpty()) {
        // The pending permission was granted while the object was suspended.
        setIsAllowed(isAllowed());
        ASSERT(!m_hasChangedPosition);
        ASSERT(!m_errorWaitingForResume);
        return;
    }

    if (isDenied() && hasListeners()) {
        // The permission was revoked while the object was suspended.
        setIsAllowed(false);
        return;
    }

    if (m_hasChangedPosition) {
        positionChanged();
        m_hasChangedPosition = false;
    }

    if (m_errorWaitingForResume) {
        handleError(m_errorWaitingForResume.get());
        m_errorWaitingForResume = nullptr;
    }
}

void Geolocation::resetAllGeolocationPermission()
{
    if (m_isSuspended) {
        m_resetOnResume = true;
        return;
    }

    if (m_allowGeolocation == InProgress) {
        Page* page = this->page();
        if (page)
            GeolocationController::from(page)->cancelPermissionRequest(this);

        // This return is not technically correct as GeolocationController::cancelPermissionRequest() should have cleared the active request.
        // Neither iOS nor OS X supports cancelPermissionRequest() (https://bugs.webkit.org/show_bug.cgi?id=89524), so we workaround that and let ongoing requests complete. :(
        return;
    }

    // 1) Reset our own state.
    stopUpdating();
    m_allowGeolocation = Unknown;
    m_hasChangedPosition = false;
    m_errorWaitingForResume = nullptr;

    // 2) Request new permission for the active notifiers.
    stopTimers();

    // Go over the one shot and re-request permission.
    GeoNotifierSet::iterator end = m_oneShots.end();
    for (GeoNotifierSet::iterator it = m_oneShots.begin(); it != end; ++it)
        startRequest((*it).get());
    // Go over the watchers and re-request permission.
    GeoNotifierVector watcherCopy;
    m_watchers.getNotifiersVector(watcherCopy);
    for (size_t i = 0; i < watcherCopy.size(); ++i)
        startRequest(watcherCopy[i].get());
}

void Geolocation::stop()
{
    Page* page = this->page();
    if (page && m_allowGeolocation == InProgress)
        GeolocationController::from(page)->cancelPermissionRequest(this);
    // The frame may be moving to a new page and we want to get the permissions from the new page's client.
    m_allowGeolocation = Unknown;
    cancelAllRequests();
    stopUpdating();
    m_hasChangedPosition = false;
    m_errorWaitingForResume = nullptr;
    m_pendingForPermissionNotifiers.clear();
}

Geoposition* Geolocation::lastPosition()
{
    Page* page = this->page();
    if (!page)
        return 0;

    m_lastPosition = createGeoposition(GeolocationController::from(page)->lastPosition());

    return m_lastPosition.get();
}

void Geolocation::getCurrentPosition(PassRefPtr<PositionCallback> successCallback, PassRefPtr<PositionErrorCallback> errorCallback, PassRefPtr<PositionOptions> options)
{
    if (!frame())
        return;

    RefPtr<GeoNotifier> notifier = GeoNotifier::create(this, successCallback, errorCallback, options);
    startRequest(notifier.get());

    m_oneShots.add(notifier);
}

int Geolocation::watchPosition(PassRefPtr<PositionCallback> successCallback, PassRefPtr<PositionErrorCallback> errorCallback, PassRefPtr<PositionOptions> options)
{
    if (!frame())
        return 0;

    RefPtr<GeoNotifier> notifier = GeoNotifier::create(this, successCallback, errorCallback, options);
    startRequest(notifier.get());

    int watchID;
    // Keep asking for the next id until we're given one that we don't already have.
    do {
        watchID = m_scriptExecutionContext->circularSequentialID();
    } while (!m_watchers.add(watchID, notifier));
    return watchID;
}

void Geolocation::startRequest(GeoNotifier *notifier)
{
    // Check whether permissions have already been denied. Note that if this is the case,
    // the permission state can not change again in the lifetime of this page.
    if (isDenied())
        notifier->setFatalError(PositionError::create(PositionError::PERMISSION_DENIED, ASCIILiteral(permissionDeniedErrorMessage)));
    else if (haveSuitableCachedPosition(notifier->options()))
        notifier->setUseCachedPosition();
    else if (notifier->hasZeroTimeout())
        notifier->startTimerIfNeeded();
    else if (!isAllowed()) {
        // if we don't yet have permission, request for permission before calling startUpdating()
        m_pendingForPermissionNotifiers.add(notifier);
        requestPermission();
    } else if (startUpdating(notifier))
        notifier->startTimerIfNeeded();
    else
        notifier->setFatalError(PositionError::create(PositionError::POSITION_UNAVAILABLE, ASCIILiteral(failedToStartServiceErrorMessage)));
}

void Geolocation::fatalErrorOccurred(Geolocation::GeoNotifier* notifier)
{
    // This request has failed fatally. Remove it from our lists.
    m_oneShots.remove(notifier);
    m_watchers.remove(notifier);

    if (!hasListeners())
        stopUpdating();
}

void Geolocation::requestUsesCachedPosition(GeoNotifier* notifier)
{
    // This is called asynchronously, so the permissions could have been denied
    // since we last checked in startRequest.
    if (isDenied()) {
        notifier->setFatalError(PositionError::create(PositionError::PERMISSION_DENIED, ASCIILiteral(permissionDeniedErrorMessage)));
        return;
    }

    m_requestsAwaitingCachedPosition.add(notifier);

    // If permissions are allowed, make the callback
    if (isAllowed()) {
        makeCachedPositionCallbacks();
        return;
    }

    // Request permissions, which may be synchronous or asynchronous.
    requestPermission();
}

void Geolocation::makeCachedPositionCallbacks()
{
    // All modifications to m_requestsAwaitingCachedPosition are done
    // asynchronously, so we don't need to worry about it being modified from
    // the callbacks.
    GeoNotifierSet::const_iterator end = m_requestsAwaitingCachedPosition.end();
    for (GeoNotifierSet::const_iterator iter = m_requestsAwaitingCachedPosition.begin(); iter != end; ++iter) {
        GeoNotifier* notifier = iter->get();
        notifier->runSuccessCallback(lastPosition());

        // If this is a one-shot request, stop it. Otherwise, if the watch still
        // exists, start the service to get updates.
        if (!m_oneShots.remove(notifier) && m_watchers.contains(notifier)) {
            if (notifier->hasZeroTimeout() || startUpdating(notifier))
                notifier->startTimerIfNeeded();
            else
                notifier->setFatalError(PositionError::create(PositionError::POSITION_UNAVAILABLE, ASCIILiteral(failedToStartServiceErrorMessage)));
        }
    }

    m_requestsAwaitingCachedPosition.clear();

    if (!hasListeners())
        stopUpdating();
}

void Geolocation::requestTimedOut(GeoNotifier* notifier)
{
    // If this is a one-shot request, stop it.
    m_oneShots.remove(notifier);

    if (!hasListeners())
        stopUpdating();
}

bool Geolocation::haveSuitableCachedPosition(PositionOptions* options)
{
    Geoposition* cachedPosition = lastPosition();
    if (!cachedPosition)
        return false;
    if (!options->hasMaximumAge())
        return true;
    if (!options->maximumAge())
        return false;
    DOMTimeStamp currentTimeMillis = convertSecondsToDOMTimeStamp(currentTime());
    return cachedPosition->timestamp() > currentTimeMillis - options->maximumAge();
}

void Geolocation::clearWatch(int watchID)
{
    if (watchID <= 0)
        return;

    if (GeoNotifier* notifier = m_watchers.find(watchID))
        m_pendingForPermissionNotifiers.remove(notifier);
    m_watchers.remove(watchID);
    
    if (!hasListeners())
        stopUpdating();
}

void Geolocation::setIsAllowed(bool allowed)
{
    // Protect the Geolocation object from garbage collection during a callback.
    Ref<Geolocation> protect(*this);

    // This may be due to either a new position from the service, or a cached
    // position.
    m_allowGeolocation = allowed ? Yes : No;
    
    if (m_isSuspended)
        return;

    // Permission request was made during the startRequest process
    if (!m_pendingForPermissionNotifiers.isEmpty()) {
        handlePendingPermissionNotifiers();
        m_pendingForPermissionNotifiers.clear();
        return;
    }

    if (!isAllowed()) {
        RefPtr<PositionError> error = PositionError::create(PositionError::PERMISSION_DENIED,  ASCIILiteral(permissionDeniedErrorMessage));
        error->setIsFatal(true);
        handleError(error.get());
        m_requestsAwaitingCachedPosition.clear();
        m_hasChangedPosition = false;
        m_errorWaitingForResume = nullptr;

        return;
    }

    // If the service has a last position, use it to call back for all requests.
    // If any of the requests are waiting for permission for a cached position,
    // the position from the service will be at least as fresh.
    if (lastPosition())
        makeSuccessCallbacks();
    else
        makeCachedPositionCallbacks();
}

void Geolocation::sendError(GeoNotifierVector& notifiers, PositionError* error)
{
     GeoNotifierVector::const_iterator end = notifiers.end();
     for (GeoNotifierVector::const_iterator it = notifiers.begin(); it != end; ++it) {
         RefPtr<GeoNotifier> notifier = *it;
         
         notifier->runErrorCallback(error);
     }
}

void Geolocation::sendPosition(GeoNotifierVector& notifiers, Geoposition* position)
{
    GeoNotifierVector::const_iterator end = notifiers.end();
    for (GeoNotifierVector::const_iterator it = notifiers.begin(); it != end; ++it)
        (*it)->runSuccessCallback(position);
}

void Geolocation::stopTimer(GeoNotifierVector& notifiers)
{
    GeoNotifierVector::const_iterator end = notifiers.end();
    for (GeoNotifierVector::const_iterator it = notifiers.begin(); it != end; ++it)
        (*it)->stopTimer();
}

void Geolocation::stopTimersForOneShots()
{
    GeoNotifierVector copy;
    copyToVector(m_oneShots, copy);
    
    stopTimer(copy);
}

void Geolocation::stopTimersForWatchers()
{
    GeoNotifierVector copy;
    m_watchers.getNotifiersVector(copy);
    
    stopTimer(copy);
}

void Geolocation::stopTimers()
{
    stopTimersForOneShots();
    stopTimersForWatchers();
}

void Geolocation::cancelRequests(GeoNotifierVector& notifiers)
{
    GeoNotifierVector::const_iterator end = notifiers.end();
    for (GeoNotifierVector::const_iterator it = notifiers.begin(); it != end; ++it)
        (*it)->setFatalError(PositionError::create(PositionError::POSITION_UNAVAILABLE, ASCIILiteral(framelessDocumentErrorMessage)));
}

void Geolocation::cancelAllRequests()
{
    GeoNotifierVector copy;
    copyToVector(m_oneShots, copy);
    cancelRequests(copy);
    m_watchers.getNotifiersVector(copy);
    cancelRequests(copy);
}

void Geolocation::extractNotifiersWithCachedPosition(GeoNotifierVector& notifiers, GeoNotifierVector* cached)
{
    GeoNotifierVector nonCached;
    GeoNotifierVector::iterator end = notifiers.end();
    for (GeoNotifierVector::const_iterator it = notifiers.begin(); it != end; ++it) {
        GeoNotifier* notifier = it->get();
        if (notifier->useCachedPosition()) {
            if (cached)
                cached->append(notifier);
        } else
            nonCached.append(notifier);
    }
    notifiers.swap(nonCached);
}

void Geolocation::copyToSet(const GeoNotifierVector& src, GeoNotifierSet& dest)
{
     GeoNotifierVector::const_iterator end = src.end();
     for (GeoNotifierVector::const_iterator it = src.begin(); it != end; ++it) {
         GeoNotifier* notifier = it->get();
         dest.add(notifier);
     }
}

void Geolocation::handleError(PositionError* error)
{
    ASSERT(error);
    
    GeoNotifierVector oneShotsCopy;
    copyToVector(m_oneShots, oneShotsCopy);

    GeoNotifierVector watchersCopy;
    m_watchers.getNotifiersVector(watchersCopy);

    // Clear the lists before we make the callbacks, to avoid clearing notifiers
    // added by calls to Geolocation methods from the callbacks, and to prevent
    // further callbacks to these notifiers.
    GeoNotifierVector oneShotsWithCachedPosition;
    m_oneShots.clear();
    if (error->isFatal())
        m_watchers.clear();
    else {
        // Don't send non-fatal errors to notifiers due to receive a cached position.
        extractNotifiersWithCachedPosition(oneShotsCopy, &oneShotsWithCachedPosition);
        extractNotifiersWithCachedPosition(watchersCopy, 0);
    }

    sendError(oneShotsCopy, error);
    sendError(watchersCopy, error);

    // hasListeners() doesn't distinguish between notifiers due to receive a
    // cached position and those requiring a fresh position. Perform the check
    // before restoring the notifiers below.
    if (!hasListeners())
        stopUpdating();

    // Maintain a reference to the cached notifiers until their timer fires.
    copyToSet(oneShotsWithCachedPosition, m_oneShots);
}

void Geolocation::requestPermission()
{
    if (m_allowGeolocation > Unknown)
        return;

    Page* page = this->page();
    if (!page)
        return;

    m_allowGeolocation = InProgress;

    // Ask the embedder: it maintains the geolocation challenge policy itself.
    GeolocationController::from(page)->requestPermission(this);
}

void Geolocation::makeSuccessCallbacks()
{
    ASSERT(lastPosition());
    ASSERT(isAllowed());
    
    GeoNotifierVector oneShotsCopy;
    copyToVector(m_oneShots, oneShotsCopy);
    
    GeoNotifierVector watchersCopy;
    m_watchers.getNotifiersVector(watchersCopy);
    
    // Clear the lists before we make the callbacks, to avoid clearing notifiers
    // added by calls to Geolocation methods from the callbacks, and to prevent
    // further callbacks to these notifiers.
    m_oneShots.clear();

    sendPosition(oneShotsCopy, lastPosition());
    sendPosition(watchersCopy, lastPosition());

    if (!hasListeners())
        stopUpdating();
}

void Geolocation::positionChanged()
{
    ASSERT(isAllowed());

    // Stop all currently running timers.
    stopTimers();

    if (m_isSuspended) {
        m_hasChangedPosition = true;
        return;
    }

    makeSuccessCallbacks();
}

void Geolocation::setError(GeolocationError* error)
{
    if (m_isSuspended) {
        m_errorWaitingForResume = createPositionError(error);
        return;
    }
    RefPtr<PositionError> positionError = createPositionError(error);
    handleError(positionError.get());
}

bool Geolocation::startUpdating(GeoNotifier* notifier)
{
    Page* page = this->page();
    if (!page)
        return false;

    GeolocationController::from(page)->addObserver(this, notifier->options()->enableHighAccuracy());
    return true;
}

void Geolocation::stopUpdating()
{
    Page* page = this->page();
    if (!page)
        return;

    GeolocationController::from(page)->removeObserver(this);
}

void Geolocation::handlePendingPermissionNotifiers()
{
    // While we iterate through the list, we need not worry about list being modified as the permission 
    // is already set to Yes/No and no new listeners will be added to the pending list
    GeoNotifierSet::const_iterator end = m_pendingForPermissionNotifiers.end();
    for (GeoNotifierSet::const_iterator iter = m_pendingForPermissionNotifiers.begin(); iter != end; ++iter) {
        GeoNotifier* notifier = iter->get();

        if (isAllowed()) {
            // start all pending notification requests as permission granted.
            // The notifier is always ref'ed by m_oneShots or m_watchers.
            if (startUpdating(notifier))
                notifier->startTimerIfNeeded();
            else
                notifier->setFatalError(PositionError::create(PositionError::POSITION_UNAVAILABLE, ASCIILiteral(failedToStartServiceErrorMessage)));
        } else
            notifier->setFatalError(PositionError::create(PositionError::PERMISSION_DENIED, ASCIILiteral(permissionDeniedErrorMessage)));
    }
}

} // namespace WebCore
                                                        
#endif // ENABLE(GEOLOCATION)
