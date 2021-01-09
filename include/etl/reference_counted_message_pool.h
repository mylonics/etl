///\file

/******************************************************************************
The MIT License(MIT)

Embedded Template Library.
https://github.com/ETLCPP/etl
https://www.etlcpp.com

Copyright(c) 2020 jwellbelove

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#ifndef ETL_REFERENCE_COUNTED_MESSAGE_POOL_INCLUDED
#define ETL_REFERENCE_COUNTED_MESSAGE_POOL_INCLUDED

#include "platform.h"
#include "message.h"
#include "ireference_counted_message_pool.h"
#include "reference_counted_message.h"
#include "static_assert.h"
#include "error_handler.h"
#include "utility.h"
#include "atomic.h"
#include "memory.h"
#include "largest.h"

#undef ETL_FILE
#define ETL_FILE ETL_REFERENCE_COUNTER_MESSAGE_POOL_ID

#undef ETL_MAX
#define ETL_MAX(a, b) ((a) < (b) ? (b) : (a))

namespace etl
{
  //***************************************************************************
  /// Exception type for etl::reference_counted_message_pool
  //***************************************************************************
  class reference_counted_message_pool_exception : public etl::exception
  {
  public:

    reference_counted_message_pool_exception(string_type reason_, string_type file_name_, numeric_type line_number_)
      : exception(reason_, file_name_, line_number_)
    {
    }
  };

  //***************************************************************************
  /// Exception if the allocation failed.
  //***************************************************************************
  class reference_counted_message_pool_allocation_failure : etl::reference_counted_message_pool_exception
  {
  public:

    reference_counted_message_pool_allocation_failure(string_type file_name_, numeric_type line_number_)
      : reference_counted_message_pool_exception(ETL_ERROR_TEXT("reference_counted_message_pool:allocation failure", ETL_FILE"A"), file_name_, line_number_)
    {
    }
  };

  //***************************************************************************
  /// Exception if the release failed.
  //***************************************************************************
  class reference_counted_message_pool_release_failure : etl::reference_counted_message_pool_exception
  {
  public:

    reference_counted_message_pool_release_failure(string_type file_name_, numeric_type line_number_)
      : reference_counted_message_pool_exception(ETL_ERROR_TEXT("reference_counted_message_pool:release failure", ETL_FILE"B"), file_name_, line_number_)
    {
    }
  };

  //***************************************************************************
  /// A pool for allocating reference counted messages.
  //***************************************************************************
  template <typename TCounter>
  class reference_counted_message_pool : public ireference_counted_message_pool
  {
  public:

    //*************************************************************************
    /// Constructor
    //*************************************************************************
    reference_counted_message_pool(imemory_block_allocator& memory_block_allocator_)
      : memory_block_allocator(memory_block_allocator_)
    {
    }

    //*************************************************************************
    /// Allocate a reference counted message from the pool.
    //*************************************************************************
    template <typename TMessage>
    etl::ipool_message* allocate(const TMessage& message)
    {
      ETL_STATIC_ASSERT((etl::is_base_of<etl::imessage, TMessage>::value), "Not a message type");

      typedef etl::pool_message<TMessage, TCounter> rcm_t;
      typedef rcm_t* prcm_t;

      prcm_t p = ETL_NULLPTR;

      p = static_cast<prcm_t>(memory_block_allocator.allocate(sizeof(rcm_t)));

      if (p != ETL_NULLPTR)
      {
        ::new(p) rcm_t(message, *this);
      }

      ETL_ASSERT((p != ETL_NULLPTR), ETL_ERROR(etl::reference_counted_message_pool_allocation_failure));

      return p;
    }

    //*************************************************************************
    /// Destruct a message and send it back to the pool.
    //*************************************************************************
    void release(const etl::ipool_message& rcmessage)
    {
      rcmessage.~ipool_message();
      bool released = memory_block_allocator.release(&rcmessage);

      ETL_ASSERT(released, ETL_ERROR(etl::reference_counted_message_pool_release_failure));
    }

#if ETL_CPP11_SUPPORTED
    //*****************************************************
    template <typename TMessage1, typename... TMessages>
    struct max_pool_message_size
    {
    private:

      // Size of the first pool message type.
      static constexpr size_t size1 = sizeof(etl::pool_message<TMessage1, TCounter>);

      // Maximum size of the the rest of the pool message types.
      static constexpr size_t size2 = max_pool_message_size<TMessages...>::size;

      // Size of the first pool message type.
      static constexpr size_t alignment1 = etl::alignment_of<etl::pool_message<TMessage1, TCounter>>::value;

      // Maximum size of the the rest of the pool message types.
      static constexpr size_t alignment2 = max_pool_message_size<TMessages...>::alignment;

    public:

      // The maximum size.
      static constexpr size_t size = (size1 < size2) ? size2 : size1;

      // The maximum alignment.
      static constexpr size_t alignment = (alignment1 < alignment2) ? alignment2 : alignment1;
    };

    //*****************************************************
    template <typename TMessage1>
    struct max_pool_message_size<TMessage1>
    {
    public:

      ETL_STATIC_ASSERT((etl::is_base_of<etl::imessage, TMessage1>::value), "TMessage not derived from etl::imessage");

      // The size of this pool message type.
      static constexpr size_t size = sizeof(etl::pool_message<TMessage1, TCounter>);

      // The maximum alignment.
      static constexpr size_t alignment = etl::alignment_of<etl::pool_message<TMessage1, TCounter>>::value;
    };
#else
    template <typename TMessage1,              typename TMessage2  = TMessage1, typename TMessage3  = TMessage1, typename TMessage4  = TMessage1,
              typename TMessage5  = TMessage1, typename TMessage6  = TMessage1, typename TMessage7  = TMessage1, typename TMessage8  = TMessage1>
    struct max_pool_message_size
    {
      static const size_t size = etl::largest<etl::pool_message<TMessage1, TCounter>,
                                              etl::pool_message<TMessage2, TCounter>,
                                              etl::pool_message<TMessage3, TCounter>,
                                              etl::pool_message<TMessage4, TCounter>,
                                              etl::pool_message<TMessage5, TCounter>,
                                              etl::pool_message<TMessage6, TCounter>,
                                              etl::pool_message<TMessage7, TCounter>,
                                              etl::pool_message<TMessage8, TCounter> >::size;


      static const size_t alignment = etl::largest<etl::pool_message<TMessage1, TCounter>,
                                                   etl::pool_message<TMessage2, TCounter>,
                                                   etl::pool_message<TMessage3, TCounter>,
                                                   etl::pool_message<TMessage4, TCounter>,
                                                   etl::pool_message<TMessage5, TCounter>,
                                                   etl::pool_message<TMessage6, TCounter>,
                                                   etl::pool_message<TMessage7, TCounter>,
                                                   etl::pool_message<TMessage8, TCounter> >::alignment;
    };
#endif

  private:

    /// The raw memory block pool.
    imemory_block_allocator& memory_block_allocator;

    // Should not be copied.
    reference_counted_message_pool(const reference_counted_message_pool&) ETL_DELETE;
    reference_counted_message_pool& operator =(const reference_counted_message_pool&) ETL_DELETE;
  };

#if ETL_CPP11_SUPPORTED && ETL_HAS_ATOMIC
  using  atomic_counted_message_pool = reference_counted_message_pool<etl::atomic_int>;
#endif
}

#undef ETL_FILE
#undef ETL_MAX

#endif
