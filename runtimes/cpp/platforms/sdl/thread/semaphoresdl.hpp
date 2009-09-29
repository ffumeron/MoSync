/* Copyright (C) 2009 Mobile Sorcery AB

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License, version 2, as published by
the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA.
*/

/* 
 * File:   semaphoresdl.hpp
 * Author: Ali Mosavian
 *
 * Created on July 14, 2009
 */

#ifndef __SEMAPHORESDL_HPP__
#define	__SEMAPHORESDL_HPP__

#include <SDL/SDL_thread.h>
#include "thread/semaphore.hpp"

namespace Base
{
    namespace Thread
    {
        class SemaphoreSDL : public Semaphore
        {
        private:
            friend class SemaphoreFactorySDL;

            SDL_sem *   m_sem;

            /**
             * Constructor
             *
             * @param v     Initial semaphore valure
             *
             */
            SemaphoreSDL ( int v, SemaphoreFactory *i = NULL );

            /**
             * Checks if the OS mutex was created, for internal use.
             *
             * @return true if it the OS mutex was created and this is
             *         a valid mutex.
             */
            virtual bool isValid ( void );

        public:

            /**
             * Destructor
             *
             */
            ~SemaphoreSDL ( void );

            /**
             * Wait for a "signal", if there's one available, it will
             * return immediatly, if not it will block until some other
             * thread performs a notify.
             *
             */
            virtual void wait ( void );

            /**
             * Posts a notification
             *
             */
            virtual void post ( void );
        };
    }
}


#endif	/* _SEMAPHORESDL_HPP */

