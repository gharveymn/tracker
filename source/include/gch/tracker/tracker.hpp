/** tracker.h
 * Track variable lifetimes automatically.
 *
 * Copyright © 2019-2021 Gene Harvey
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef GCH_TRACKER_HPP
#define GCH_TRACKER_HPP

#include "detail/common.hpp"
#include "reporter.hpp"

namespace gch
{

  template <typename Parent, typename RemoteTag, typename ...Ts>
  struct tracker_traits<tracker<Parent, RemoteTag, Ts...>>
  {
    using local_tag = typename detail::tag::tracker<Parent, Ts...>::reduced_tag;
    using remote_tag = RemoteTag;

    using local_base_tag  = typename local_tag::base_tag;
    using remote_base_tag = typename remote_tag::base_tag;

    using local_parent_type  = Parent;
    using remote_parent_type = typename remote_tag::template parent_type<local_tag>;

    using local_interface_type  = tracker<Parent, RemoteTag, Ts...>;
    using remote_interface_type = typename remote_tag::template interface_type<local_tag>;

    using local_common_type  = typename local_tag::template common_type<remote_tag>;
    using remote_common_type = typename remote_tag::template common_type<local_tag>;

    using local_base_type  = typename local_tag::template base_type<local_base_tag,
                                                                    remote_base_tag>;
    using remote_base_type = typename remote_tag::template base_type<remote_base_tag,
                                                                     local_base_tag>;
  };

  namespace detail
  {

    template <typename ...Ts>
    using tracker_container = plf::list<Ts...>;

    template <typename RemoteBaseTag>
    class tracker_base
    {
      using derived_type = tracker_base<RemoteBaseTag>;
    public:

      using local_base_tag  = tag::tracker_base;
      using remote_base_tag = RemoteBaseTag;

    protected:
      // Store pointers to the base types. Downcast when needed.
      using reporter_type  = reporter_base<tag::tracker_base, remote_base_tag>;
      using reporter_list  = tracker_container<reporter_type>;
      using rptrs_iter     = typename reporter_list::iterator;
      using rptrs_citer    = typename reporter_list::const_iterator;
      using rptrs_riter    = typename reporter_list::reverse_iterator;
      using rptrs_criter   = typename reporter_list::const_reverse_iterator;
      using rptrs_ref      = typename reporter_list::reference;
      using rptrs_cref     = typename reporter_list::const_reference;
      using rptrs_size_ty  = typename reporter_list::size_type;
      using rptrs_diff_ty  = typename reporter_list::difference_type;
      using rptrs_alloc_t  = typename reporter_list::allocator_type;

    public:
      using access_type  = rptrs_iter;
      using caccess_type = rptrs_citer;

      using local_reporter_type = reporter_type;

      using remote_base_type = typename remote_base_tag::template type<remote_base_tag,
                                                                       tag::tracker_base>;

      using remote_reporter_type = typename remote_base_type::local_reporter_type;

      tracker_base            (void)                           = default;
      tracker_base            (const tracker_base&)     = delete;
//    tracker_base            (tracker_base&&) noexcept = impl;
      tracker_base& operator= (const tracker_base&)     = delete;
//    tracker_base& operator= (tracker_base&&) noexcept = impl;
      ~tracker_base           (void)                           = default;

      tracker_base (tracker_base&& other) noexcept
      {
        splice_reporters (rptrs_cend (), other);
      }

      tracker_base&
      operator= (tracker_base&& other) noexcept
      {
        // clear is noexcept, so it's be safe to do that first
        reset ();
        m_rptrs = std::move (other.m_rptrs);
        repoint_reporters (rptrs_begin (), rptrs_end ());
        return *this;
      }

      tracker_base (const rptrs_citer first, const rptrs_citer last)
      {
        rebind_remote (rptrs_cend (), first, last);
      }

      //! unsafe if needed
      void
      wipe_reporters (void) noexcept
      {
        m_rptrs.clear ();
      }

      void
      reset (void) noexcept
      {
        if (! m_rptrs.empty ())
        {
          for (reporter_type& p : m_rptrs)
            p.reset_remote_tracking ();
          m_rptrs.clear ();
        }
      }

      void
      swap (tracker_base& other) noexcept
      {
        // kinda expensive
        m_rptrs.swap (other.m_rptrs);
        this->repoint_reporters (this->rptrs_begin (), this->rptrs_end ());
        other.repoint_reporters (other.rptrs_begin (), other.rptrs_end ());
      }

      rptrs_iter
      splice_reporters (const rptrs_citer pos, tracker_base& src)
      {
        rptrs_iter pivot = src.rptrs_begin ();
        // splice is unsafe
        m_rptrs.splice (pos, src.m_rptrs);
        // repoint_reporters is safe
        repoint_reporters (rptrs_begin (), rptrs_end ());
        return pivot;
      }

      rptrs_iter
      transfer_reporters (const rptrs_citer pos, tracker_base& src,
                               const rptrs_citer first, const rptrs_citer last)
      {
        rptrs_iter ret = m_rptrs.insert (pos, first, last);
        std::for_each (rptrs_citer { ret }, pos,
                       [this, ret](const reporter_type& r)
                       {
                         r.get_remote_reporter ().set (static_cast<derived_type&> (*this), ret);
                       });
        src.m_rptrs.erase (first, last);
        return ret;
      }

      GCH_NODISCARD rptrs_iter   rptrs_begin   (void)       noexcept { return m_rptrs.begin ();   }
      GCH_NODISCARD rptrs_citer  rptrs_begin   (void) const noexcept { return m_rptrs.begin ();   }
      GCH_NODISCARD rptrs_citer  rptrs_cbegin  (void) const noexcept { return m_rptrs.cbegin ();  }

      GCH_NODISCARD rptrs_iter   rptrs_end     (void)       noexcept { return m_rptrs.end ();     }
      GCH_NODISCARD rptrs_citer  rptrs_end     (void) const noexcept { return m_rptrs.end ();     }
      GCH_NODISCARD rptrs_citer  rptrs_cend    (void) const noexcept { return m_rptrs.cend ();    }

      GCH_NODISCARD rptrs_riter  rptrs_rbegin  (void)       noexcept { return m_rptrs.rbegin ();  }
      GCH_NODISCARD rptrs_criter rptrs_rbegin  (void) const noexcept { return m_rptrs.rbegin ();  }
      GCH_NODISCARD rptrs_criter rptrs_crbegin (void) const noexcept { return m_rptrs.crbegin (); }

      GCH_NODISCARD rptrs_riter  rptrs_rend    (void)       noexcept { return m_rptrs.rend ();    }
      GCH_NODISCARD rptrs_criter rptrs_rend    (void) const noexcept { return m_rptrs.rend ();    }
      GCH_NODISCARD rptrs_criter rptrs_crend   (void) const noexcept { return m_rptrs.crend ();   }

      GCH_NODISCARD rptrs_ref    rptrs_front   (void)                { return m_rptrs.front ();   }
      GCH_NODISCARD rptrs_cref   rptrs_front   (void) const          { return m_rptrs.front ();   }

      GCH_NODISCARD rptrs_ref    rptrs_back    (void)                { return m_rptrs.back ();    }
      GCH_NODISCARD rptrs_cref   rptrs_back    (void) const          { return m_rptrs.back ();    }

      GCH_NODISCARD
      bool rptrs_empty (void) const noexcept
      {
        return m_rptrs.empty ();
      }

      GCH_NODISCARD
      rptrs_size_ty
      rptrs_size (void) const noexcept
      {
        return m_rptrs.size ();
      }

      GCH_NODISCARD
      rptrs_size_ty
      rptrs_max_size (void) const noexcept
      {
        return m_rptrs.max_size ();
      }

      template <typename ...Args>
      rptrs_iter
      rptrs_emplace (rptrs_citer pos, Args&&... args)
      {
        return m_rptrs.emplace (pos, std::forward<Args> (args)...);
      }

      template <typename ...Args>
      rptrs_iter
      rptrs_erase (Args&&... args) noexcept
      {
        return m_rptrs.erase (std::forward<Args> (args)...);
      }

      // safe to use, but may throw
      rptrs_iter
      track (rptrs_citer pos, remote_base_type& remote)
      {
        return m_rptrs.emplace (pos, tag::track, remote);
      }

      GCH_NODISCARD GCH_CPP17_CONSTEXPR
      typename std::iterator_traits<rptrs_citer>::difference_type
      get_reporter_offset (rptrs_citer pos) const noexcept
      {
        return std::distance (rptrs_cbegin (), pos);
      }

      void
      repoint_reporters (rptrs_iter first, rptrs_iter last) noexcept
      {
        std::for_each (first, last,
                       [this](reporter_type& rptr)
                       {
                         rptr.get_remote_reporter ().track (static_cast<derived_type&> (*this));
                       });
      }

      //! safe, symmetric
      rptrs_iter
      debind_remote (rptrs_citer pos) noexcept
      {
        pos->reset_remote_tracking ();
        return rptrs_erase (pos);
      }

      //! safe
      rptrs_iter
      debind_remote (rptrs_citer first, const rptrs_citer last) noexcept
      {
        while (first != last)
          first = debind_remote (first);
        return rptrs_erase (last, last);
      }

      //! safe
      template <typename Pred>
      void
      base_remove_if (Pred pred)
      {
        m_rptrs.remove_if ([&](const reporter_type& e)
                           {
                             if (pred (e))
                             {
                               e.reset_remote_tracking ();
                               return true;
                             }
                             return false;
                           });
      }

      rptrs_iter
      rebind_remote (rptrs_citer pos, remote_base_type& r);

      rptrs_iter
      rebind_remote (rptrs_citer pos, remote_base_type&& r)
      {
        return rebind_remote (pos, r);
      }

      rptrs_iter
      rebind_remote (const rptrs_citer pos, rptrs_citer first, const rptrs_citer last)
      {
        if (first == last)
          return rptrs_erase (pos, pos);

        const rptrs_iter pivot = rebind_remote (pos, first->get_remote_base ());
        try
        {
          while (++first != last)
            rebind_remote (pos, first->get_remote_base ());
        }
        catch (...)
        {
          debind_remote (pivot, pos);
          throw;
        }
        return pivot;
      }

      template <typename InterfaceRef, typename Iterator>
      rptrs_iter
      rebind_remote (const rptrs_citer pos, Iterator first, const Iterator last)
      {
        if (first == last)
          return rptrs_erase (pos, pos);

        const rptrs_iter pivot = rebind_remote (pos, static_cast<InterfaceRef> (*first));
        try
        {
          while (++first != last)
            rebind_remote (pos, static_cast<InterfaceRef> (*first));
        }
        catch (...)
        {
          debind_remote (pivot, pos);
          throw;
        }
        return pivot;
      }

      void
      replace_remote (rptrs_iter pos, remote_base_type& r);

      void
      merge_reporters (tracker_base& other)
      {
        assert (has_sorted_reporters () && "`*this` must be sorted in order to merge");
        assert (other.has_sorted_reporters () && "`other` must be sorted in order to merge");
        m_rptrs.merge (other.m_rptrs);
      }

      GCH_NODISCARD
      bool
      has_sorted_reporters (void) const
      {
        return std::is_sorted (rptrs_begin (), rptrs_end ());
      }

      void
      sort_reporters (void)
      {
        m_rptrs.sort ();
      }

      rptrs_citer
      find_sorted_pos (const remote_base_type& r) const
      {
        return std::lower_bound (rptrs_begin (), rptrs_end (), &r);
      }

    private:
      reporter_list m_rptrs;
    };

    template <>
    inline
    auto
    tracker_base<tag::reporter_base>::
    rebind_remote (const rptrs_citer pos, reporter_base<tag::reporter_base, tag::tracker_base>& r)
      -> rptrs_iter
    {
      const rptrs_iter local_it = rptrs_emplace (pos, tag::track, r);
      r.reset (*this, local_it);
      return local_it;
    }

    template <>
    inline
    auto
    tracker_base<tag::tracker_base>::
    rebind_remote (const rptrs_citer pos, tracker_base<tag::tracker_base>& r)
      -> rptrs_iter
    {
      const rptrs_iter local_it = rptrs_emplace (pos);
      try
      {
        const rptrs_iter remote_it = r.rptrs_emplace (r.rptrs_end ());
        local_it ->set (r, remote_it);
        remote_it->set (*this,  local_it );
      }
      catch (...)
      {
        rptrs_erase (local_it);
        throw;
      }
      return local_it;
    }

    template <>
    inline
    void
    tracker_base<tag::reporter_base>::
    replace_remote (const rptrs_iter pos, reporter_base<tag::reporter_base, tag::tracker_base>& r)
    {
      r.reset (*this, pos);
      pos->reset (r);
    }

    template <>
    inline
    void
    tracker_base<tag::tracker_base>::
    replace_remote (const rptrs_iter pos, tracker_base<tag::tracker_base>& r)
    {
      const rptrs_iter remote_it = r.rptrs_emplace (r.rptrs_end ());
      pos->reset (r, remote_it);
      remote_it->set (*this, pos);
    }

    template class tracker_base<tag::reporter_base>;
    template class tracker_base<tag::tracker_base>;

    template <typename Interface>
    class reporter_common;

    template <typename Interface>
    class tracker_common;

    // pretend like reporter_base doesn't exist
    template <typename ReporterIt, typename RemoteType, typename RemoteInterfaceType>
    class tracker_iterator
    {
      using reporter_iter         = ReporterIt;
      using reporter_type         = typename ReporterIt::value_type;

      static constexpr
      bool
      is_const = std::is_const<
        typename std::remove_pointer<
          typename std::iterator_traits<reporter_iter>::pointer>>::value;

      using remote_type = typename std::conditional<is_const,
                                                      const RemoteType,
                                                            RemoteType>::type;

      using remote_interface_type = typename std::conditional<is_const,
                                                              const RemoteInterfaceType,
                                                              RemoteInterfaceType>::type;

    public:
      using difference_type   = typename reporter_iter::difference_type;
      using value_type        = typename std::remove_const<remote_type>::type;
      using pointer           = remote_type *;
      using reference         = remote_type&;
      using iterator_category = typename reporter_iter::iterator_category;

      tracker_iterator            (void)                          = default;
      tracker_iterator            (const tracker_iterator&)       = default;
      tracker_iterator            (tracker_iterator&&) noexcept   = default;
      tracker_iterator& operator= (const tracker_iterator&) &     = default;
      tracker_iterator& operator= (tracker_iterator&&) & noexcept = default;
      ~tracker_iterator (void)                                    = default;

      template <typename It, typename T, typename I,
                typename std::enable_if<! tracker_iterator<It, T, I>::is_const>::type * = nullptr>
      /* implicit */
      tracker_iterator (const tracker_iterator<It, T, I>& other) noexcept
        : m_iter (other.base ())
      { }

    private:
      template <typename Interface>
      friend class tracker_common;

      template <typename It, typename T, typename I>
      friend class tracker_iterator;

      explicit
      tracker_iterator (reporter_iter it)
          : m_iter (it)
      { }

      reporter_iter
      base (void) const noexcept
      {
        return m_iter;
      }

    public:
      tracker_iterator&
      operator++ (void) noexcept
      {
        ++m_iter;
        return *this;
      }

      tracker_iterator
      operator++ (int) noexcept
      {
        return iter_common (m_iter++);
      }

      tracker_iterator&
      operator-- (void) noexcept
      {
        --m_iter;
        return *this;
      }

      tracker_iterator
      operator-- (int) noexcept
      {
        return iter_common (m_iter--);
      }

      friend
      bool
      operator== (const tracker_iterator& lhs, const tracker_iterator& rhs) noexcept
      {
        return lhs.base () == rhs.base ();
      }

#ifndef GCH_IMPL_THREE_WAY_COMPARISON

      friend
      bool
      operator!= (const tracker_iterator& lhs, const tracker_iterator& rhs) noexcept
      {
        return lhs.base () != rhs.base ();
      }

#endif

      constexpr
      reference
      operator* (void) const noexcept
      {
        return get_remote ();
      }

      constexpr
      pointer
      operator-> (void) const noexcept
      {
        return &get_remote ();
      }

      constexpr
      reference
      get_remote (void) const noexcept
      {
        return get_remote_interface ().get_parent ();
      }

      constexpr
      remote_interface_type&
      get_remote_interface (void) const noexcept
      {
        return static_cast<remote_interface_type&> (m_iter->get_remote_base ());
      }

    private:
      reporter_iter m_iter;
    };

    ////////////////////
    // tracker_common //
    ////////////////////

    template <typename Parent, typename RemoteTag, typename IntrusiveTag>
    class tracker_common<tracker<Parent, RemoteTag, IntrusiveTag>>
      : private tracker_base<typename RemoteTag::base_tag>,
        public tracker_traits<tracker<Parent, RemoteTag, IntrusiveTag>>
    {
      using base = tracker_base<typename RemoteTag::base_tag>;
      using traits = tracker_traits<tracker<Parent, RemoteTag, IntrusiveTag>>;

    public:
      using local_tag  = typename traits::local_tag;
      using remote_tag = typename traits::remote_tag;

      using local_base_tag  = typename traits::local_base_tag;
      using remote_base_tag = typename traits::remote_base_tag;

      using local_parent_type  = typename traits::local_parent_type;
      using remote_parent_type = typename traits::remote_parent_type;

      using local_interface_type  = typename traits::local_interface_type;
      using remote_interface_type = typename traits::remote_interface_type;

      using local_common_type  = typename traits::local_common_type;
      using remote_common_type = typename traits::remote_common_type;

      using local_base_type  = typename traits::local_base_type;
      using remote_base_type = typename traits::remote_base_type;

      static_assert (std::is_same<local_base_type, base>::value);

      friend remote_common_type;
      friend remote_base_type;

      template <typename It, typename T, typename I>
      friend class tracker_iterator;

      // class iterator;
      // class const_iterator;

    private:
      using local_reporter_type  = typename base::local_reporter_type;
      using remote_reporter_type = typename base::remote_reporter_type;

      using rptrs_iter            = typename base::rptrs_iter;
      using rptrs_citer           = typename base::rptrs_citer;

    public:
      using value_type              = remote_parent_type;
      using allocator_type          = typename base::rptrs_alloc_t;
      using size_type               = typename base::rptrs_size_ty;
      using difference_type         = typename base::rptrs_diff_ty;
      using reference               = value_type&;
      using const_reference         = const value_type&;
      using pointer                 = value_type *;
      using const_pointer           = const value_type *;

      using iterator       = tracker_iterator<rptrs_iter, value_type, remote_interface_type>;
      using const_iterator = tracker_iterator<rptrs_citer, value_type, remote_interface_type>;

      using reverse_iterator       = std::reverse_iterator<iterator>;
      using const_reverse_iterator = std::reverse_iterator<const_iterator>;

      using val_t   = value_type;
      using alloc_t = allocator_type;
      using size_ty = size_type;
      using diff_ty = difference_type;
      using ref     = reference;
      using cref    = const_reference;
      using ptr     = pointer;
      using cptr    = const_pointer;

      using iter    = iterator;
      using citer   = const_iterator;
      using riter   = reverse_iterator;
      using criter  = const_reverse_iterator;

    public:
      tracker_common            (void)                      = default;
      tracker_common            (const tracker_common&)     = delete;
      tracker_common            (tracker_common&&) noexcept = default;
      tracker_common& operator= (const tracker_common&)     = delete;
      tracker_common& operator= (tracker_common&&) noexcept = default;
//    ~tracker_common           (void)                      = impl;

      //! safe by default
      ~tracker_common (void)
      {
        clear ();
      }

      explicit
      tracker_common (gch::tag::bind_t)
      { }

      template <typename ...RemoteInterface, typename Tag = remote_tag,
                tag::enable_if_tracker_t<Tag> * = nullptr>
      tracker_common (gch::tag::bind_t, remote_interface_type& remote, RemoteInterface&&... rest)
        : tracker_common (gch::tag::bind, std::forward<RemoteInterface> (rest)...)
      {
        try
        {
          push_front (remote);
        }
        catch (...)
        {
          clear ();
          throw;
        }
      }

      template <typename ...RemoteInterface>
      tracker_common (gch::tag::bind_t, remote_interface_type&& remote, RemoteInterface&&... rest)
        : tracker_common (gch::tag::bind, std::forward<RemoteInterface> (rest)...)
      {
        try
        {
          push_front (std::move (remote));
        }
        catch (...)
        {
          clear ();
          throw;
        }
      }

      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag> * = nullptr>
      tracker_common (const const_iterator first, const const_iterator last)
        : base (first.base (), last.base ())
      { }

      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag> * = nullptr>
      tracker_common (const_iterator first, const_iterator last) = delete;

      template <typename Iterator,
        typename std::enable_if<
          std::is_constructible<remote_interface_type&,
                                decltype (*std::declval<Iterator> ())>::value>::type * = nullptr>
      tracker_common (gch::tag::bind_t, const Iterator first, const Iterator last)
      {
        using ref_type = remote_interface_type&;
        base::template rebind_remote<ref_type> (end ().base (), first, last);
      }

      template <typename Iterator,
        typename std::enable_if<
          std::is_constructible<remote_interface_type&&,
                                decltype (*std::declval<Iterator> ())>::value>::type * = nullptr>
      tracker_common (gch::tag::bind_t, const Iterator first, const Iterator last)
      {
        using ref_type = remote_interface_type&&;
        base::template rebind_remote<ref_type> (end ().base (), first, last);
      }

      tracker_common (gch::tag::bind_t,
                      std::initializer_list<std::reference_wrapper<remote_interface_type>> init)
        : tracker_common (gch::tag::bind, init.begin (), init.end ())
      { }

      void
      swap (tracker_common& other) noexcept
      {
        return base::swap (other);
      }

      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag> * = nullptr>
      GCH_CPP14_CONSTEXPR
      tracker_common
      clone (void) const
      {
        return { begin (), end () };
      }

      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag> * = nullptr>
      void
      clone (void) const = delete;

      void
      replace (const iterator pos, remote_interface_type& remote)
      {
        base::replace_remote (pos.base (), remote);
      }

      //! transfers all bindings from `other`; no overwriting
      GCH_CPP14_CONSTEXPR
      iterator
      splice (const const_iterator pos, local_interface_type& other)
      {
        return iterator { base::splice_reporters (pos.base (), other) };
      }

      GCH_CPP14_CONSTEXPR
      iterator
      splice (const const_iterator pos, local_interface_type&& other)
      {
        return splice (pos, other);
      }

      GCH_CPP14_CONSTEXPR
      iterator
      splice_front (local_interface_type& other)
      {
        return splice (cbegin (), other);
      }

      GCH_CPP14_CONSTEXPR
      iterator
      splice_front (local_interface_type&& other)
      {
        return splice (cbegin (), other);
      }

      GCH_CPP14_CONSTEXPR
      iterator
      splice_back (local_interface_type& other)
      {
        return splice (cend (), other);
      }

      GCH_CPP14_CONSTEXPR
      iterator
      splice_back (local_interface_type&& other)
      {
        return splice (cend (), other);
      }

      // moves the binding from other (may throw)
      GCH_CPP14_CONSTEXPR
      iterator
      transfer (const const_iterator pos, local_interface_type& other, const const_iterator it)
      {
        return base::transfer_reporters (pos.base (), other, it, std::next (it).base ());
      }

      GCH_CPP14_CONSTEXPR
      iterator
      transfer (const const_iterator pos, local_interface_type&& other, const const_iterator it)
      {
        return transfer (pos, other, it);
      }

      // move multiple bindings
      GCH_CPP14_CONSTEXPR
      iterator
      transfer (const const_iterator pos, local_interface_type& other,
                const const_iterator first, const const_iterator last)
      {
        return base::transfer_reporters (pos.base (), other, first.base (), last.base ());
      }

      GCH_CPP14_CONSTEXPR
      iterator
      transfer (const const_iterator pos, local_interface_type&& other,
                const const_iterator first, const const_iterator last)
      {
        return transfer (pos, other, first, last);
      }

      GCH_CPP14_CONSTEXPR
      iterator
      transfer_front (local_interface_type& other, const const_iterator it)
      {
        return transfer (cbegin (), other, it);
      }

      GCH_CPP14_CONSTEXPR
      iterator
      transfer_front (local_interface_type&& other, const const_iterator it)
      {
        return transfer (cbegin (), other, it);
      }

      GCH_CPP14_CONSTEXPR
      iterator
      transfer_front (local_interface_type& other,
                      const const_iterator first, const const_iterator last)
      {
        return transfer (cbegin (), other, first, last);
      }

      GCH_CPP14_CONSTEXPR
      iterator
      transfer_front (local_interface_type&& other,
                      const const_iterator first, const const_iterator last)
      {
        return transfer (cbegin (), other, first, last);
      }

      GCH_CPP14_CONSTEXPR
      iterator
      transfer_back (local_interface_type& other, const const_iterator it)
      {
        return transfer (cend (), other, it);
      }

      GCH_CPP14_CONSTEXPR
      iterator
      transfer_back (local_interface_type&& other, const const_iterator it)
      {
        return transfer (cend (), other, it);
      }

      GCH_CPP14_CONSTEXPR
      iterator
      transfer_back (local_interface_type& other,
                     const const_iterator first, const const_iterator last)
      {
        return transfer (cend (), other, first, last);
      }

      GCH_CPP14_CONSTEXPR
      iterator
      transfer_back (local_interface_type&& other,
                     const const_iterator first, const const_iterator last)
      {
        return transfer (cend (), other, first, last);
      }

      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag> * = nullptr>
      local_interface_type&
      replace_bindings (const local_interface_type& other)
      {
        if (&other != this)
        {
          citer pivot = insert (end (), other.begin (), other.end ());
          erase (begin (), pivot);
        }
        return static_cast<local_interface_type&> (*this);
      }

      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag> * = nullptr>
      void
      replace_bindings (const local_interface_type& other) = delete;

      local_interface_type&
      replace_bindings (local_interface_type&& other)
      {
        return static_cast<local_interface_type&> (base::operator= (std::move (other)));
      }

      GCH_NODISCARD
      iterator
      begin (void) noexcept
      {
        return iter { base::rptrs_begin () };
      }

      GCH_NODISCARD
      const_iterator
      begin (void) const noexcept
      {
        return citer { base::rptrs_begin () };
      }

      GCH_NODISCARD
      const_iterator
      cbegin (void) const noexcept
      {
        return begin ();
      }

      GCH_NODISCARD
      iterator
      end (void) noexcept
      {
        return iter { base::rptrs_end () };
      }

      GCH_NODISCARD
      const_iterator
      end (void) const noexcept
      {
        return citer { base::rptrs_end () };
      }

      GCH_NODISCARD
      const_iterator
      cend (void) const noexcept
      {
        return end ();
      }

      GCH_NODISCARD
      reverse_iterator
      rbegin (void) noexcept
      {
        return riter { end () };
      }

      GCH_NODISCARD
      const_reverse_iterator
      rbegin (void) const noexcept
      {
        return criter { end () };
      }

      GCH_NODISCARD
      const_reverse_iterator
      crbegin (void) const noexcept
      {
        return rbegin ();
      }

      GCH_NODISCARD
      reverse_iterator
      rend (void) noexcept
      {
        return riter { begin () };
      }

      GCH_NODISCARD
      const_reverse_iterator
      rend (void) const noexcept
      {
        return criter { begin () };
      }

      GCH_NODISCARD
      const_reverse_iterator
      crend (void) const noexcept
      {
        return rend ();
      }

      GCH_NODISCARD
      reference
      front (void)
      {
        return *begin ();
      }

      GCH_NODISCARD
      const_reference
      front (void) const
      {
        return *begin ();
      }

      GCH_NODISCARD
      reference
      back (void)
      {
        return *rbegin ();
      }

      GCH_NODISCARD
      const_reference
      back (void) const
      {
        return *rbegin ();
      }

      GCH_NODISCARD
      bool
      empty (void) const noexcept
      {
        return base::rptrs_empty ();
      }

      GCH_NODISCARD
      bool
      size (void) const noexcept
      {
        return base::rptrs_size ();
      }

      GCH_NODISCARD
      bool
      has_remotes (void) const noexcept
      {
        return ! empty ();
      }

      GCH_NODISCARD
      size_type
      num_remotes (void) const noexcept
      {
        return size ();
      }

      void
      clear (void) noexcept
      {
        base::reset ();
      }

      void
      wipe (void) noexcept
      {
        base::wipe_reporters ();
      }

      GCH_NODISCARD
      bool
      is_sorted (void) const
      {
        return base::has_sorted_reporters ();
      }

      GCH_NODISCARD GCH_CPP17_CONSTEXPR
      size_type
      get_offset (const const_iterator pos) const noexcept
      {
        return base::get_reporter_offset (pos.base ());
      }

      void
      debind (const remote_parent_type& r)
      {
        auto find_pred = [&r](const remote_parent_type& e) { return &e == &r; };
        auto pos = std::find_if (begin (), end (), find_pred);
        while (pos != end ())
          pos = std::find_if (erase (pos), end (), find_pred);
      }

      void
      debind (const remote_interface_type& r)
      {
        auto find_pred = [&](const local_reporter_type& e) { return &e.get_remote_base () == &r; };
        citer pos { std::find_if (begin ().base (), end ().base (), find_pred) };
        while (pos != end ())
          pos = std::find_if (erase (pos).base (), end ().base (), find_pred);
      }

      iterator
      erase (const_iterator pos)
      {
        return iterator { base::debind_remote (pos.base ()) };
      }

      iterator
      erase (const const_iterator first, const const_iterator last)
      {
        return iterator { base::debind_remote (first.base (), last.base ()) };
      }

      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag> * = nullptr>
      iterator
      insert (const const_iterator pos, remote_interface_type& r)
      {
        return iterator { base::rebind_remote (pos.base (), r) };
      }

      iterator
      insert (const const_iterator pos, remote_interface_type&& r)
      {
        return iterator { base::rebind_remote (pos.base (), r) };
      }

      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag> * = nullptr>
      iterator
      insert (const const_iterator pos, const const_iterator first, const const_iterator last)
      {
        return iterator { base::rebind_remote (pos.base (), first.base (), last.base ()) };
      }

      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag> * = nullptr>
      void
      insert (const_iterator pos, const_iterator first, const_iterator last) = delete;

      template <typename Iterator>
      auto
      insert (const const_iterator pos, Iterator first, const Iterator last)
        -> typename std::enable_if<
             std::is_constructible<remote_interface_type&&, decltype (*first)>::value,
             iterator>::type
      {
        using ref_type = remote_interface_type&&;
        return iterator { base::template rebind_remote<ref_type> (pos.base (), first, last) };
      }

      template <typename Iterator, typename Tag = remote_tag,
                typename std::enable_if<tag::is_tracker<Tag>::value> * = nullptr>
      auto
      insert (const const_iterator pos, const Iterator first, const Iterator last)
        -> typename std::enable_if<
                 tag::is_tracker<remote_tag>::value
             &&! std::is_constructible<remote_interface_type&&, decltype (*first)>::value
             &&  std::is_constructible<remote_interface_type&, decltype (*first)>::value,
             iterator>::type
      {
        using ref_type = remote_interface_type&;
        return iterator { base::template rebind_remote<ref_type> (pos.base (), first, last) };
      }

      template <typename Iterator, typename Tag = remote_tag,
                typename std::enable_if<tag::is_reporter<Tag>::value> * = nullptr>
      auto
      insert (const_iterator pos, Iterator first, Iterator last)
        -> typename std::enable_if<
                 tag::is_reporter<remote_tag>::value
             &&! std::is_constructible<remote_interface_type&&, decltype (*first)>::value
             &&  std::is_constructible<remote_interface_type&, decltype (*first)>::value>::type
      = delete;

      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag> * = nullptr>
      iterator
      insert (const const_iterator pos,
              std::initializer_list<std::reference_wrapper<remote_interface_type>> ilist)
      {
        return insert (pos, ilist.begin (), ilist.end ());
      }

      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag> * = nullptr>
      void
      insert (const_iterator pos,
              std::initializer_list<std::reference_wrapper<remote_interface_type>> ilist) = delete;

      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag> * = nullptr>
      void
      push_front (remote_interface_type& r)
      {
        insert (cbegin (), r);
      }

      void
      push_front (remote_interface_type&& r)
      {
        insert (cbegin (), std::move (r));
      }

      void
      pop_front (void)
      {
        erase (cbegin ());
      }

      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag> * = nullptr>
      void
      push_back (remote_interface_type& r)
      {
        insert (cend (), r);
      }

      void
      push_back (remote_interface_type&& r)
      {
        insert (cend (), std::move (r));
      }

      void
      pop_back (void)
      {
        erase (std::prev (cend ()));
      }

      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag> * = nullptr>
      iterator
      bind (const const_iterator first, const const_iterator last)
      {
        if (first == last)
          return end ();
        return insert (sorted_position (first), first, last);
      }

      // only available for trackers since it modifies the argument
      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag> * = nullptr>
      iterator
      bind (remote_interface_type& remote)
      {
        return insert (sorted_position (remote), remote);
      }

      // if remote_interface_type is
      //   reporter: rebinds r to this
      //   tracker:  adds r to this
      iterator
      bind (remote_interface_type&& remote)
      {
        return bind (remote);
      }

      template <typename Iterator>
      auto
      bind (const Iterator first, const Iterator last)
        -> typename std::enable_if<
             std::is_constructible<remote_interface_type&&, decltype (*first)>::value,
             iterator>::type
      {
        if (first == last)
          return end ();
        citer pos { sorted_position (static_cast<remote_interface_type&&> (*first)) };
        return insert (pos, first, last);
      }

      template <typename Iterator>
      auto
      bind (const Iterator first, const Iterator last)
        -> typename std::enable_if<
                 tag::is_tracker<remote_tag>::value
             &&! std::is_constructible<remote_interface_type&&, decltype (*first)>::value
             &&  std::is_constructible<remote_interface_type&, decltype (*first)>::value,
             iterator>::type
      {
        if (first == last)
          return end ();
        citer pos { sorted_position (static_cast<remote_interface_type&> (*first)) };
        return insert (pos, first, last);
      }

      template <typename Iterator>
      auto
      bind (Iterator first, Iterator last)
        -> typename std::enable_if<
                 tag::is_reporter<remote_tag>::value
             &&! std::is_constructible<remote_interface_type&&, decltype (*first)>::value
             &&  std::is_constructible<remote_interface_type&, decltype (*first)>::value>::type
      = delete;

      // only available for trackers since it modifies r
      template <typename ...Args, typename Tag = remote_tag,
                tag::enable_if_tracker_t<Tag> * = nullptr>
      iterator
      bind (remote_interface_type& remote, Args&&... args)
      {
        iter ret = bind (remote);
        bind (std::forward<Args> (args)...);
        return ret;
      }

      // if remote_interface_type is
      //   reporter: rebinds r to this
      //   tracker: adds this to r
      template <typename ...Args>
      iterator
      bind (remote_interface_type&& remote, Args&&... args)
      {
        iter ret = bind (std::move (remote));
        bind (std::forward<Args> (args)...);
        return ret;
      }

      void
      merge (local_interface_type& other)
      {
        merge_reporters (other);
      }

      void
      merge (local_interface_type&& other)
      {
        merge (other);
      }

      const_iterator
      sorted_position (const remote_interface_type& remote)
      {
        return citer { base::find_sorted_pos (remote) };
      }

      const_iterator
      sorted_position (const remote_interface_type&& remote)
      {
        return citer { base::find_sorted_pos (remote) };
      }

      const_iterator
      sorted_position (const const_iterator cit)
      {
        return citer { base::find_sorted_pos (cit.get_remote_interface ()) };
      }

    private:
    }; // tracker_common

  } // gch::detail

  /////////////////////////
  // tracker (intrusive) //
  /////////////////////////

  template <typename Derived, typename RemoteTag>
  class tracker<Derived, RemoteTag, tag::intrusive>
    : public detail::tracker_common<tracker<Derived, RemoteTag, tag::intrusive>>
  {
    using base = detail::tracker_common<tracker<Derived, RemoteTag, tag::intrusive>>;

  public:
    using derived_type  = Derived;
    using remote_tag    = RemoteTag;
    using intrusive_tag = tag::intrusive;

    using base::base;
    tracker            (void)               = default;
    tracker            (const tracker&)     = delete;
    tracker            (tracker&&) noexcept = default;
    tracker& operator= (const tracker&)     = delete;
    tracker& operator= (tracker&&) noexcept = default;
    ~tracker           (void)               = default;

    /* implicit */
    tracker (base&& other) noexcept
      : base (std::move (other))
    { }

    GCH_NODISCARD GCH_CPP14_CONSTEXPR
    derived_type&
    get_parent (void) noexcept
    {
      return static_cast<derived_type&> (*this);
    }

    GCH_NODISCARD constexpr
    const derived_type&
    get_parent (void) const noexcept
    {
      return static_cast<const derived_type&> (*this);
    }
  }; // tracker

  ////////////////////////////
  // tracker (nonintrusive) //
  ////////////////////////////

  template <typename Parent, typename RemoteTag>
  class tracker<Parent, RemoteTag, tag::nonintrusive>
    : public detail::tracker_common<tracker<Parent, RemoteTag, tag::nonintrusive>>,
      public detail::nonintrusive_common<Parent>
  {
    using base = detail::tracker_common<tracker<Parent, RemoteTag, tag::nonintrusive>>;
    using access_base = detail::nonintrusive_common<Parent>;

  public:
    using parent_type   = Parent;
    using remote_tag    = RemoteTag;
    using intrusive_tag = tag::nonintrusive;

    using base::base;
    tracker            (void)               = default;
    tracker            (const tracker&)     = delete;
    tracker            (tracker&&) noexcept = default;
    tracker& operator= (const tracker&)     = delete;
    tracker& operator= (tracker&&) noexcept = delete;
    ~tracker           (void)               = default;

    void
    swap (tracker& other) noexcept
    {
      // no usage of access_base::operator=
      base::swap (other);
    }

    tracker (base&& other, parent_type& parent) noexcept
      : base        (std::move (other)),
        access_base (parent)
    { }

    tracker (tracker&& other, parent_type& parent) noexcept
      : base        (std::move (other)),
        access_base (parent)
    { }

    explicit
    tracker (parent_type& parent)
      : access_base (parent)
    { }

    template <typename Tag = remote_tag,
              detail::tag::enable_if_reporter_t<Tag> * = nullptr>
    tracker (parent_type& parent,
             std::initializer_list<std::reference_wrapper<
               typename base::remote_interface_type>> init) = delete;

    template <typename Tag = remote_tag,
              detail::tag::enable_if_tracker_t<Tag> * = nullptr>
    tracker (parent_type& parent,
             std::initializer_list<std::reference_wrapper<
               typename base::remote_interface_type>> init)
      : base        (tag::bind, init),
        access_base (parent)
    { }
  }; // tracker

  template <typename RemoteTag>
  struct standalone_tracker
    : public tracker<standalone_tracker<RemoteTag>, RemoteTag, tag::intrusive>
  {
    using base = tracker<standalone_tracker, RemoteTag, tag::intrusive>;
  public:
    using base::base;
  };

  template <typename Parent, typename Remote = Parent>
  using multireporter = tracker<Parent, remote::tracker<Remote>>;

  template <typename Derived, typename RemoteTag>
  using intrusive_tracker = tracker<Derived, RemoteTag, tag::intrusive>;

  namespace intrusive
  {

    template <typename Derived, typename RemoteTag>
    using tracker = gch::tracker<Derived, RemoteTag, tag::intrusive>;

  } // namespace gch::intrusive

  template <typename ...Ts, typename ...RemoteTypes>
  void
  bind (tracker<Ts...>& l, RemoteTypes&... remotes)
  {
    l.bind (remotes...);
  }

} // namespace gch

#endif
