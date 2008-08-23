// Spatial Index Library
//
// Copyright (C) 2002 Navel Ltd.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  Email:
//    mhadji@gmail.com

#ifndef __storagemanager_randomevictionsbuffer_h
#define __storagemanager_randomevictionsbuffer_h

#include "Buffer.h"

namespace SpatialIndex
{
  namespace StorageManager
  {
    class RandomEvictionsBuffer : public Buffer
    {
      public:
        RandomEvictionsBuffer( IStorageManager&, Tools::PropertySet& ps );
        // see Buffer.h for available properties.

        virtual ~RandomEvictionsBuffer();

        virtual void addEntry( long id, Buffer::Entry* pEntry );
        virtual void removeEntry();
    }; // RandomEvictionsBuffer
  }
}

#endif /*__storagemanager_randomevictionsbuffer_h*/

