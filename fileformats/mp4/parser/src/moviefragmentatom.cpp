/* ------------------------------------------------------------------
 * Copyright (C) 1998-2010 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */
/*********************************************************************************/
/*     -------------------------------------------------------------------       */
/*                            MPEG-4 Movie Fragment Atom Class                             */
/*     -------------------------------------------------------------------       */
/*********************************************************************************/
/*
*/

#define IMPLEMENT_MovieFragmentAtom

#include "moviefragmentatom.h"
#include "moviefragmentheaderatom.h"
#include "trackfragmentatom.h"
#include "atomdefs.h"
#include "atomutils.h"
#include "piffboxes.h"

class TrackExtendsAtom;

typedef Oscl_Vector<TrackFragmentAtom*, OsclMemAllocator> trackFragmentAtomVecType;
typedef Oscl_Vector<TrackIndexStruct, OsclMemAllocator> trackIndexVecType;

// Constructor
MovieFragmentAtom::MovieFragmentAtom(MP4_FF_FILE *fp,
                                     Oscl_Vector<MP4_FF_FILE *, OsclMemAllocator> *pTrackFpVec,
                                     uint32 &size,
                                     uint32 type,
                                     TrackDurationContainer *trackDurationContainer,
                                     Oscl_Vector<TrackExtendsAtom*, OsclMemAllocator> *trackExtendAtomVec,
                                     bool &parseMoofCompletely,
                                     bool &moofParsingCompleted,
                                     uint32 &countOfTrunsParsed,
                                     const TrackEncryptionBoxContainer* apTrackEncryptionBoxCntr)
        : Atom(fp, size, type)
        , ipTrackEncryptionBxCntr(apTrackEncryptionBoxCntr)
{
    _pMovieFragmentHeaderAtom       = NULL;
    _pTrackFragmentAtom             = NULL;
    _pMovieFragmentCurrentOffset    = 0;
    _pMovieFragmentBaseOffset       = 0;
    _currentTrackFragmentOffset     = 0;
    _trafIndex = 0;
    _pTrackIndexVec = NULL;

    parseTrafCompletely = true;
    trafParsingCompleted = true;
    sizeRemaining = 0;
    atomtype = UNKNOWN_ATOM;
    _ptrackFragmentArray = NULL;

    _pMovieFragmentBaseOffset = AtomUtils::getCurrentFilePosition(fp);
    _pMovieFragmentCurrentOffset = _pMovieFragmentBaseOffset;

    iLogger = PVLogger::GetLoggerObject("mp4ffparser");
    iStateVarLogger = PVLogger::GetLoggerObject("mp4ffparser_mediasamplestats");
    iParsedDataLogger = PVLogger::GetLoggerObject("mp4ffparser_parseddata");

    _pTrackFpVec = pTrackFpVec;
    uint32 count = size - DEFAULT_ATOM_SIZE;
    uint32 index = 0;
    if (_success)
    {
        PV_MP4_FF_NEW(fp->auditCB, trackFragmentAtomVecType, (), _ptrackFragmentArray);

        while (count > 0)
        {
            uint32 atomType = UNKNOWN_ATOM;
            uint32 atomSize = 0;

            AtomUtils::getNextAtomType(fp, atomSize, atomType);

            if (atomType == MOVIE_FRAGMENT_HEADER_ATOM)
            {
                if (_pMovieFragmentHeaderAtom == NULL)
                {
                    PV_MP4_FF_NEW(fp->auditCB, MovieFragmentHeaderAtom, (fp, atomSize, atomType), _pMovieFragmentHeaderAtom);
                    if (!_pMovieFragmentHeaderAtom->MP4Success())
                    {
                        _success = false;
                        _mp4ErrorCode = READ_MOVIE_FRAGMENT_HEADER_FAILED;
                        return;
                    }
                    count -= _pMovieFragmentHeaderAtom->getSize();
                }
                else
                {
                    //duplicate atom
                    count -= atomSize;
                    atomSize -= DEFAULT_ATOM_SIZE;
                    AtomUtils::seekFromCurrPos(fp, atomSize);
                }
            }
            else if (atomType == TRACK_FRAGMENT_ATOM)
            {
                if (!parseMoofCompletely)
                {
                    parseTrafCompletely = false;
                }

                PV_MP4_FF_NEW(fp->auditCB, TrackFragmentAtom, (fp, (*_pTrackFpVec)[index++], atomSize,
                              atomType, _pMovieFragmentCurrentOffset,
                              _pMovieFragmentBaseOffset,
                              size, trackDurationContainer,
                              trackExtendAtomVec,
                              parseTrafCompletely,
                              trafParsingCompleted,
                              countOfTrunsParsed, ipTrackEncryptionBxCntr),
                              _pTrackFragmentAtom);

                if (trafParsingCompleted)
                {
                    if (!_pTrackFragmentAtom->MP4Success())
                    {
                        _success = false;
                        _mp4ErrorCode = READ_TRACK_FRAGMENT_ATOM_FAILED;
                        return;
                    }
                    count -= _pTrackFragmentAtom->getSize();
                    size = count;
                    _ptrackFragmentArray->push_back(_pTrackFragmentAtom);
                    _pMovieFragmentCurrentOffset += _pTrackFragmentAtom->_trackFragmentEndOffset;
                }
                else
                {
                    _ptrackFragmentArray->push_back(_pTrackFragmentAtom);
                    size = count;
                    sizeRemaining = atomSize;
                    atomtype = atomType;

                    if (sizeRemaining == 0)
                    {
                        trafParsingCompleted = true;
                        if (!_pTrackFragmentAtom->MP4Success())
                        {
                            _success = false;
                            _mp4ErrorCode = READ_TRACK_FRAGMENT_ATOM_FAILED;
                            return;
                        }
                        count -= _pTrackFragmentAtom->getSize();
                        size = count;
                        _pMovieFragmentCurrentOffset += _pTrackFragmentAtom->_trackFragmentEndOffset;
                    }
                }

                setIndexForTrackID(_pTrackFragmentAtom->trackId);
                if (!parseMoofCompletely)
                {
                    moofParsingCompleted = false;
                    break;
                }
            }
            else
            {
                count -= atomSize;
                atomSize -= DEFAULT_ATOM_SIZE;
                AtomUtils::seekFromCurrPos(fp, atomSize);
            }
        }
        _trafIndex = _ptrackFragmentArray->size();

        if (count == 0)
        {
            moofParsingCompleted = true;
        }
    }
    else
    {
        _mp4ErrorCode = READ_MOVIE_FRAGMENT_ATOM_FAILED;
    }
}

void MovieFragmentAtom::ParseMoofAtom(MP4_FF_FILE *fp,
                                      uint32 &size,
                                      uint32 type,
                                      TrackDurationContainer *trackDurationContainer,
                                      Oscl_Vector<TrackExtendsAtom*, OsclMemAllocator> *trackExtendAtomVec,
                                      bool &moofParsingCompleted,
                                      uint32 &countOfTrunsParsed,
                                      const uint32 trackID)
{
    OSCL_UNUSED_ARG(type);
    uint32 count = size;

    if (_success)
    {
        if (count > 0)
        {
            if (!trafParsingCompleted)
            {
                _pTrackFragmentAtom->ParseTrafAtom(fp, sizeRemaining,
                                                   atomtype, _pMovieFragmentCurrentOffset,
                                                   _pMovieFragmentBaseOffset,
                                                   size, trackDurationContainer,
                                                   trackExtendAtomVec,
                                                   trafParsingCompleted,
                                                   countOfTrunsParsed);
                if (trafParsingCompleted)
                {
                    if (!_pTrackFragmentAtom->MP4Success())
                    {
                        _success = false;
                        _mp4ErrorCode = READ_TRACK_FRAGMENT_ATOM_FAILED;
                        return;
                    }
                    count -= _pTrackFragmentAtom->getSize();
                    size = count;
                    _pMovieFragmentCurrentOffset += _pTrackFragmentAtom->_trackFragmentEndOffset;
                }
                else
                {
                    if (sizeRemaining == 0)
                    {
                        trafParsingCompleted = true;
                        if (!_pTrackFragmentAtom->MP4Success())
                        {
                            _success = false;
                            _mp4ErrorCode = READ_TRACK_FRAGMENT_ATOM_FAILED;
                            return;
                        }
                        count -= _pTrackFragmentAtom->getSize();
                        size = count;
                        _pMovieFragmentCurrentOffset += _pTrackFragmentAtom->_trackFragmentEndOffset;
                    }
                }
            }
            else if (count > 0)
            {
                uint32 atomType = UNKNOWN_ATOM;
                uint32 atomSize = 0;

                AtomUtils::getNextAtomType(fp, atomSize, atomType);

                if (atomType == TRACK_FRAGMENT_ATOM)
                {


                    uint32 index = getIndexForTrackId(trackID);

                    PV_MP4_FF_NEW(fp->auditCB, TrackFragmentAtom, (fp, (*_pTrackFpVec)[index], atomSize,
                                  atomType, _pMovieFragmentCurrentOffset,
                                  _pMovieFragmentBaseOffset,
                                  size, trackDurationContainer,
                                  trackExtendAtomVec,
                                  parseTrafCompletely,
                                  trafParsingCompleted,
                                  countOfTrunsParsed, ipTrackEncryptionBxCntr),
                                  _pTrackFragmentAtom);

                    if (trafParsingCompleted)
                    {
                        if (!_pTrackFragmentAtom->MP4Success())
                        {
                            _success = false;
                            _mp4ErrorCode = READ_TRACK_FRAGMENT_ATOM_FAILED;
                            return;
                        }
                        count -= _pTrackFragmentAtom->getSize();
                        size = count;
                        _ptrackFragmentArray->push_back(_pTrackFragmentAtom);
                        _pMovieFragmentCurrentOffset += _pTrackFragmentAtom->_trackFragmentEndOffset;
                    }
                    else
                    {
                        _ptrackFragmentArray->push_back(_pTrackFragmentAtom);
                        sizeRemaining = atomSize;
                        atomtype = atomType;

                        if (sizeRemaining == 0)
                        {
                            trafParsingCompleted = true;
                            if (!_pTrackFragmentAtom->MP4Success())
                            {
                                _success = false;
                                _mp4ErrorCode = READ_TRACK_FRAGMENT_ATOM_FAILED;
                                return;
                            }
                            count -= _pTrackFragmentAtom->getSize();
                            size = count;
                            _pMovieFragmentCurrentOffset += _pTrackFragmentAtom->_trackFragmentEndOffset;
                        }
                    }
                }
                else
                {
                    count -= atomSize;
                    atomSize -= DEFAULT_ATOM_SIZE;
                    AtomUtils::seekFromCurrPos(fp, atomSize);
                }
            }
            _trafIndex = _ptrackFragmentArray->size();
        }
        if (count == 0)
        {
            moofParsingCompleted = true;
        }
    }
    else
    {
        _mp4ErrorCode = READ_MOVIE_FRAGMENT_ATOM_FAILED;
    }
}

int32
MovieFragmentAtom::getNextBundledAccessUnits(uint32 id,
        uint32 *n, uint32 totalSampleRead,
        GAU    *pgau,
        Oscl_Vector<PVPIFFProtectedSampleDecryptionInfo, OsclMemAllocator>*  apSampleDecryptionInfoVect)
{
    int32 nReturn = -1;

    TrackFragmentAtom *trackfragment = getTrackFragmentforID(id);

    if (trackfragment != NULL)
    {
        nReturn =  trackfragment->getNextBundledAccessUnits(n, totalSampleRead, pgau, apSampleDecryptionInfoVect);
    }
    return (nReturn);
}

int32
MovieFragmentAtom::peekNextBundledAccessUnits(uint32 id,
        uint32 *n, uint32 totalSampleRead,
        MediaMetaInfo *mInfo)
{
    int32 nReturn = -1;

    TrackFragmentAtom *trackfragment = getTrackFragmentforID(id);

    if (trackfragment != NULL)
    {
        nReturn =  trackfragment->peekNextBundledAccessUnits(n, totalSampleRead, mInfo);
    }
    return (nReturn);
}

uint64 MovieFragmentAtom::resetPlayback(uint32 trackID, uint64 time, uint32 traf_number, uint32 trun_number, uint32 sample_num)
{
    uint64 nReturn = 0;

    PVMF_MP4FFPARSER_LOGMEDIASAMPELSTATEVARIABLES((0, "MovieFragmentAtom::resetPlayback Called TrackID %d", trackID));
    if (traf_number > 0)
    {
        TrackFragmentAtom *trackfragment = (*_ptrackFragmentArray)[traf_number-1];
        if (trackfragment != NULL)
        {
            if (trackfragment->getTrackId() == trackID)
            {
                nReturn =  trackfragment->resetPlayback(time, trun_number, sample_num);
                PVMF_MP4FFPARSER_LOGMEDIASAMPELSTATEVARIABLES((0, "MovieFragmentAtom::resetPlayback Return Time %d", Oscl_Int64_Utils::get_uint64_lower32(nReturn)));
            }
        }
    }
    else
    {
        TrackFragmentAtom *trackfragment;
        for (uint32 i = 0; i < _ptrackFragmentArray->size(); i++)
        {
            trackfragment = (*_ptrackFragmentArray)[i];
            if (trackfragment != NULL)
            {
                if (trackfragment->getTrackId() == trackID)
                {
                    nReturn = trackfragment->resetPlayback(time);
                    PVMF_MP4FFPARSER_LOGMEDIASAMPELSTATEVARIABLES((0, "MovieFragmentAtom::resetPlayback Return Time %d", Oscl_Int64_Utils::get_uint64_lower32(nReturn)));
                    break;
                }
            }
        }
    }
    return (nReturn);
}

void MovieFragmentAtom::resetPlayback()
{
    uint32 i;
    TrackFragmentAtom *trackfragment;
    for (i = 0; i < _ptrackFragmentArray->size(); i++)
    {
        trackfragment = (*_ptrackFragmentArray)[i];;

        if (trackfragment != NULL)
        {
            trackfragment->resetPlayback();
        }
    }
}
MovieFragmentAtom::~MovieFragmentAtom()
{
    if (_pMovieFragmentHeaderAtom != NULL)
    {
        PV_MP4_FF_DELETE(NULL, MovieFragmentHeaderAtom, _pMovieFragmentHeaderAtom);
        _pMovieFragmentHeaderAtom = NULL;
    }
    for (uint32 i = 0; i < _ptrackFragmentArray->size(); i++)
    {
        PV_MP4_FF_DELETE(NULL, TrackFragmentAtom, (*_ptrackFragmentArray)[i]);
    }
    PV_MP4_FF_TEMPLATED_DELETE(NULL, trackFragmentAtomVecType, Oscl_Vector, _ptrackFragmentArray);

    if (_pTrackFpVec != NULL)
    {
        _pTrackFpVec = NULL;
    }
    if (_pTrackIndexVec != NULL)
    {
        PV_MP4_FF_TEMPLATED_DELETE(NULL, trackIndexVecType, Oscl_Vector, _pTrackIndexVec);
        _pTrackIndexVec = NULL;
    }

}

uint64
MovieFragmentAtom::getCurrentTrafDuration(uint32 id)
{
    uint64 nReturn = 0;

    TrackFragmentAtom *trackfragment = getTrackFragmentforID(id);

    if (trackfragment != NULL)
    {
        nReturn =  trackfragment->getCurrentTrafDuration();
    }
    return (nReturn);
}

uint32
MovieFragmentAtom::getTotalSampleInTraf(uint32 id)
{
    int32 nTotalSamples = 0;

    TrackFragmentAtom *trackfragment = getTrackFragmentforID(id);

    if (trackfragment != NULL)
    {
        nTotalSamples =  trackfragment->getTotalNumSampleInTraf();
    }
    return (nTotalSamples);
}

int32
MovieFragmentAtom::getOffsetByTime(uint32 id, uint64 ts, TOsclFileOffset* sampleFileOffset)
{
    int32 nReturn = DEFAULT_ERROR;

    TrackFragmentAtom *trackfragment = getTrackFragmentforID(id);

    if (trackfragment != NULL)
    {
        nReturn =  trackfragment->getOffsetByTime(id, ts, sampleFileOffset);
    }
    return (nReturn);
}

TrackFragmentAtom *
MovieFragmentAtom::getTrackFragmentforID(uint32 id)
{
    TrackFragmentAtom *trackFragmentAtom = NULL;

    uint32 numTrafs = _ptrackFragmentArray->size();
    for (uint32 idx = 0; idx < numTrafs; idx++)
    {
        trackFragmentAtom = (*_ptrackFragmentArray)[idx];
        if (trackFragmentAtom != NULL)
        {
            if (trackFragmentAtom->getTrackId() == id)
            {
                return trackFragmentAtom;
            }
        }
    }
    return NULL;
}

uint32 MovieFragmentAtom::getIndexForTrackId(const uint32 trackID)
{
    if (_pTrackIndexVec == NULL)
        return 0;

    uint32 size  = _pTrackIndexVec->size();

    if (!size)
        return 0;

    for (uint32 i = 0; i < size; i++)
    {
        if ((*_pTrackIndexVec)[i].iTrackId == trackID)
            return (*_pTrackIndexVec)[i].iIndex;
    }
    return (*_pTrackIndexVec)[size-1].iIndex + 1;

}
void MovieFragmentAtom::setIndexForTrackID(uint32 trackID)
{
    if (_pTrackIndexVec == NULL)
    {
        PV_MP4_FF_NEW(fp->auditCB, trackIndexVecType, (), _pTrackIndexVec);
    }

    uint32 size  = _pTrackIndexVec->size();

    TrackIndexStruct temp;
    temp.iTrackId = trackID;

    if (size == 0)
        temp.iIndex = 0;
    else
        temp.iIndex = (*_pTrackIndexVec)[size-1].iIndex + 1;

    _pTrackIndexVec->push_back(temp);
}

