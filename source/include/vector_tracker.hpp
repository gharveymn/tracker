/** vector_tracker.hpp
 * Short description here. 
 * 
 * Copyright © 2020 Gene Harvey
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef TRACKER_VECTOR_TRACKER_HPP
#define TRACKER_VECTOR_TRACKER_HPP

#include "tracker.hpp"

#include <algorithm>
#include <vector>

namespace gch
{
  namespace detail
  {
    template <typename RemoteBaseTag, template <typename ...> class Backend>
    class tracker_base_common_vector
    {
      using derived_type = tracker_base<RemoteBaseTag, Backend>;
    public:
      
      using local_base_tag  = tag::tracker_base<Backend>;
      using remote_base_tag = RemoteBaseTag;
      
      // Store pointers to the base types. Downcast when needed.
      using reporter_type   = reporter_base<local_base_tag, remote_base_tag>;
      using reporter_cont   = Backend<reporter_type>;
      using rptr_iter       = typename reporter_cont::iterator;
      using rptr_citer      = typename reporter_cont::const_iterator;
      using rptr_riter      = typename reporter_cont::reverse_iterator;
      using rptr_criter     = typename reporter_cont::const_reverse_iterator;
      using rptr_ref        = typename reporter_cont::reference;
      using rptr_cref       = typename reporter_cont::const_reference;
      
      using local_reporter_type  = reporter_type;
  
      using access_type = typename reporter_cont::difference_type;
      
      using remote_base_type     = typename remote_base_tag::template type<remote_base_tag, local_base_tag>;
  
      tracker_base_common_vector (void)                           = default;
      tracker_base_common_vector (const tracker_base_common_vector&)     = delete;
  //    tracker_base_common            (tracker_base_common&&) noexcept = impl;
      tracker_base_common_vector& operator= (const tracker_base_common_vector&)     = delete;
  //    tracker_base_common& operator= (tracker_base_common&&) noexcept = impl;
      ~tracker_base_common_vector (void)                           = default;
  
      tracker_base_common_vector (tracker_base_common_vector&& other) noexcept
      {
        base_transfer_bindings (other, rptrs_cend ());
      }
  
      tracker_base_common_vector& operator= (tracker_base_common_vector&& other) noexcept
      {
        rptr_iter pivot = base_transfer_bindings (other, rptrs_cend ());
        base_debind (rptrs_cbegin (), pivot);
        return *this;
      }
  
      GCH_CPP14_CONSTEXPR reporter_type& get_reporter (access_type n)
      {
        return m_rptrs[n];
      }
      
      //! unsafe if needed
      void wipe (void) noexcept
      {
        m_rptrs.clear ();
      }
      
      void reset (void) noexcept
      {
        clear ();
      }
      
      void clear (void) noexcept
      {
        if (! m_rptrs.empty ())
        {
          for (reporter_type& p : m_rptrs)
            p.reset_remote_tracking ();
          m_rptrs.clear ();
        }
      }
      
      void swap (tracker_base_common_vector& other) noexcept
      {
        // kinda expensive
        m_rptrs.swap (other.m_rptrs);
        this->repoint_reporters (this->rptrs_begin (), this->rptrs_end ());
        other.repoint_reporters (other.rptrs_begin (), other.rptrs_end ());
      }
      
      GCH_NODISCARD
      std::size_t num_reporters (void) const noexcept
      {
        return m_rptrs.size ();
      }
      
      rptr_iter base_transfer_bindings (tracker_base_common_vector& src, rptr_citer pos) noexcept
      {
        auto ret = m_rptrs.insert (pos, src.rptrs_cbegin (), src.rptrs_cend ());
        for (auto it = ret; it != rptrs_cend (); ++it)
        {
          it->get_remote_reporter ().set (static_cast<derived_type&> (*this), create_access (it));
        }
        src.wipe ();
        return ret;
      }
      
      GCH_NODISCARD rptr_iter   rptrs_begin   (void)       noexcept { return m_rptrs.begin ();   }
      GCH_NODISCARD rptr_citer  rptrs_begin   (void) const noexcept { return m_rptrs.begin ();   }
      GCH_NODISCARD rptr_citer  rptrs_cbegin  (void) const noexcept { return m_rptrs.cbegin ();  }
      
      GCH_NODISCARD rptr_iter   rptrs_end     (void)       noexcept { return m_rptrs.end ();     }
      GCH_NODISCARD rptr_citer  rptrs_end     (void) const noexcept { return m_rptrs.end ();     }
      GCH_NODISCARD rptr_citer  rptrs_cend    (void) const noexcept { return m_rptrs.cend ();    }
      
      GCH_NODISCARD rptr_riter  rptrs_rbegin  (void)       noexcept { return m_rptrs.rbegin ();  }
      GCH_NODISCARD rptr_criter rptrs_rbegin  (void) const noexcept { return m_rptrs.rbegin ();  }
      GCH_NODISCARD rptr_criter rptrs_crbegin (void) const noexcept { return m_rptrs.crbegin (); }
      
      GCH_NODISCARD rptr_riter  rptrs_rend    (void)       noexcept { return m_rptrs.rend ();    }
      GCH_NODISCARD rptr_criter rptrs_rend    (void) const noexcept { return m_rptrs.rend ();    }
      GCH_NODISCARD rptr_criter rptrs_crend   (void) const noexcept { return m_rptrs.crend ();   }
      
      GCH_NODISCARD rptr_ref    rptrs_front   (void)                { return m_rptrs.front ();   }
      GCH_NODISCARD rptr_cref   rptrs_front   (void) const          { return m_rptrs.front ();   }
      
      GCH_NODISCARD rptr_ref    rptrs_back    (void)                { return m_rptrs.back ();    }
      GCH_NODISCARD rptr_cref   rptrs_back    (void) const          { return m_rptrs.back ();    }
      
      GCH_NODISCARD bool        rptrs_empty   (void) const noexcept { return m_rptrs.empty ();   }
  
      rptr_iter rptrs_emplace_empty (rptr_citer pos)
      {
        auto ret = m_rptrs.emplace (pos);
        for (auto it = std::next (ret); it != rptrs_cend (); ++it)
        {
          it->get_remote_reporter ().set_access (create_access (it));
        }
        return ret;
      }
      
      rptr_iter const_cast_iter (rptr_citer cit) noexcept
      {
        return m_rptrs.erase (cit, cit);
      }
      
      rptr_iter rptrs_erase (access_type at) noexcept
      {
        auto ret = m_rptrs.erase (rptrs_cbegin () + at);
        for (auto it = ret; it != m_rptrs.end (); ++it)
        {
          it->get_remote_reporter ().set_access (create_access (it));
        }
        return ret;
      }
      
      // safe to use, but may throw
      access_type track (remote_base_type& remote)
      {
        m_rptrs.emplace (rptrs_end (), tag::track, remote);
        return m_rptrs.size () - 1;
      }
  
      GCH_NODISCARD
      constexpr access_type create_access (rptr_citer pos) const noexcept
      {
        return pos - m_rptrs.cbegin ();
      }
  
      GCH_NODISCARD
      GCH_CPP14_CONSTEXPR rptr_iter create_iter (access_type at) noexcept
      {
        return m_rptrs.begin () + at;
      }
      
      GCH_NODISCARD
      static constexpr access_type base_create_access (access_type at) noexcept
      {
        return at;
      }
  
      GCH_NODISCARD
      GCH_CPP17_CONSTEXPR typename std::iterator_traits<rptr_citer>::difference_type
      base_get_offset (access_type at) const noexcept
      {
        return at;
      }
      
      void repoint_reporters (rptr_iter first, const rptr_iter last) noexcept
      {
        for (; first != last; ++first)
          first->get_remote_reporter ().track (static_cast<derived_type&> (*this));
      }
      
      //! safe, symmetric
      rptr_iter base_debind (rptr_citer pos)
      {
        pos->reset_remote_tracking ();
        return rptrs_erase (create_access (pos));
      }
      
      //! safe
      rptr_iter base_debind (rptr_citer first, const rptr_citer last)
      {
        const auto beg_off = first - m_rptrs.cbegin ();
        const auto num = last - first;
        for (auto i = 0; i < num; ++i)
          base_debind (m_rptrs.cbegin () + beg_off);
        return m_rptrs.begin () + beg_off;
      }
    
    private:
      
      reporter_cont m_rptrs;
      
    };
  
    template <typename RemoteBaseTag>
    class tracker_base_common<RemoteBaseTag, std::vector>
      : public tracker_base_common_vector<RemoteBaseTag, std::vector>
    { };
    
  }
}
#endif // TRACKER_VECTOR_TRACKER_HPP
