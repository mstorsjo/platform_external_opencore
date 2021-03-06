/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
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
#ifndef __MEDIA_CLOCK_CONVERTER_H
#define __MEDIA_CLOCK_CONVERTER_H

#include "oscl_base.h"

#ifndef   OSCL_EXCEPTION_H_INCLUDED
#include "oscl_exception.h"
#endif

const uint32 WRAP_THRESHOLD = 0x80000000;
const uint32 MISORDER_THRESHOLD =  0x80000000;

class MediaClockConverter
{

    public:
        //As of now we cannot provide 64 bit ts in the arg as overloaded ctor.
        //Reason: if we do so, then in the existing codebase, that is passing in const number say 1000
        //as param has to be explicitly typecasted to uint32 in all over the codebase.
        //So intead we can pass in the upper 32 bits of the uint64 as uint32
        MediaClockConverter(uint32 in_timescale = 1, uint32 init_ts = 0, uint32 init_ts64_upper32 = 0)
        {
            // Timescale value cannot be zero
            OSCL_ASSERT(in_timescale != 0);
            if (0 == in_timescale)
            {
                OSCL_LEAVE(OsclErrArgument);
            }
            timescale = in_timescale;
            current_ts = init_ts;
            wrap_count = 0;
            current_ts64_upper32 = init_ts64_upper32;
        };

        MediaClockConverter(const MediaClockConverter& a)
        {
            // Timescale value cannot be zero
            OSCL_ASSERT(a.timescale != 0);
            if (0 == a.timescale)
            {
                OSCL_LEAVE(OsclErrCorrupt);
            }
            timescale = a.timescale;
            current_ts = a.current_ts;
            wrap_count = a.wrap_count;
            current_ts64_upper32 = a.current_ts64_upper32;
        };

        /**
         * The assignment operator
         */
        MediaClockConverter& operator=(const MediaClockConverter& a)
        {
            if (&a != this)
            {
                // Timescale value cannot be zero
                OSCL_ASSERT(a.timescale != 0);
                if (0 == a.timescale)
                {
                    OSCL_LEAVE(OsclErrCorrupt);
                }
                timescale = a.timescale;
                current_ts = a.current_ts;
                wrap_count = a.wrap_count;
                current_ts64_upper32 = a.current_ts64_upper32;
            }
            return *this;
        };

        void set_clock(uint32 init_ts, uint32 in_wrap_count)
        {
            current_ts = init_ts;
            current_ts64_upper32 = 0;

            // Timescale value cannot be zero
            OSCL_ASSERT(timescale != 0);
            if (0 == timescale)
            {
                OSCL_LEAVE(OsclErrCorrupt);
            }
            wrap_count = in_wrap_count % timescale;
        };

        void set_clock(uint64 init_ts, uint32 in_wrap_count)
        {
            // Timescale value cannot be zero
            OSCL_ASSERT(timescale != 0);
            if (0 == timescale)
            {
                OSCL_LEAVE(OsclErrCorrupt);
            }
            current_ts = (uint32)(init_ts & 0xFFFFFFFF);
            current_ts64_upper32 = ((uint32)(init_ts >> 32)) % timescale;
            wrap_count = in_wrap_count % timescale;
        }

        // set the clock with value from another timescale
        OSCL_IMPORT_REF void set_clock_other_timescale(uint32 value, uint32 timescale);

        OSCL_IMPORT_REF void set_timescale(uint32 new_timescale);

        OSCL_IMPORT_REF bool update_clock(uint32 new_ts);
        OSCL_IMPORT_REF bool update_clock(uint64 new_ts);
        OSCL_IMPORT_REF uint32 get_timediff_and_update_clock(uint32 value, uint32 timescale,
                uint32 output_timescale);

        OSCL_IMPORT_REF uint32 get_timediff_and_update_clock(uint32 value,
                uint32 output_timescale);

        OSCL_IMPORT_REF uint32 get_converted_ts(uint32 new_timscale) const;
        OSCL_IMPORT_REF uint64 get_converted_ts64(uint32 new_timscale) const;
        OSCL_IMPORT_REF uint32 get_wrap_count() const
        {
            return wrap_count;
        };
        OSCL_IMPORT_REF uint32 get_current_timestamp() const
        {
            return current_ts;
        };
        OSCL_IMPORT_REF uint64 get_current_timestamp64() const;

        OSCL_IMPORT_REF uint32 get_timescale() const
        {
            return timescale;
        };

        OSCL_IMPORT_REF void set_value(const MediaClockConverter& src);

    private:
        uint32 timescale;
        uint32 current_ts;
        uint32 wrap_count;
        uint32 current_ts64_upper32;
};




#endif
