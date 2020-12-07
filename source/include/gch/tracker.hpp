/** tracker.h
 * Track variable lifetimes automatically.
 *
 * Copyright © 2019 Gene Harvey
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef GCH_TRACKER_HPP
#define GCH_TRACKER_HPP

#include <plf_list.h>

#include <algorithm>
#include <iterator>

#ifndef GCH_NODISCARD
#  if __has_cpp_attribute(nodiscard)
#    define GCH_NODISCARD [[nodiscard]]
#  else
#    define GCH_NODISCARD
#  endif
#endif

#ifndef GCH_INLINE_VARS
#  if __cpp_inline_variables >= 201606
#    define GCH_INLINE_VARS inline
#  else
#    define GCH_INLINE_VARS
#  endif
#endif

#ifndef GCH_CPP17_CONSTEXPR
#if   __cpp_constexpr >= 201603L
#    define GCH_CPP17_CONSTEXPR constexpr
#  else
#    define GCH_CPP17_CONSTEXPR
#  endif
#endif

#ifndef GCH_CPP14_CONSTEXPR
#  if __cpp_constexpr >= 201304L
#    define GCH_CPP14_CONSTEXPR constexpr
#  else
#    define GCH_CPP14_CONSTEXPR
#  endif
#endif

namespace gch
{

  template <typename Parent, typename RemoteTag, typename IntrusiveTag>
  class reporter;

  template <typename Parent, typename RemoteTag, typename IntrusiveTag>
  class tracker;

  template <typename RemoteTag>
  struct standalone_reporter;

  template <typename RemoteTag>
  struct standalone_tracker;

  namespace tag
  {
    
    struct intrusive;
    struct nonintrusive;
    struct standalone;

    // for symmetric constructtors
    GCH_INLINE_VARS constexpr struct bind_t  { bind_t  (void) = default; } bind;
    
  }

  //////////////
  // defaults //
  //////////////

  template <typename Parent,
            typename RemoteTag,
            typename IntrusiveTag = tag::nonintrusive>
  class reporter;

  template <typename Parent,
            typename RemoteTag,
            typename IntrusiveTag = tag::nonintrusive>
  class tracker;

  template <typename RemoteTag>
  struct standalone_reporter;

  template <typename RemoteTag>
  struct standalone_tracker;

  namespace detail
  {

    template <typename LocalBaseTag, typename RemoteBaseTag>
    class reporter_base;

    template <typename RemoteBaseTag>
    class tracker_base;

    template <typename Interface>
    class reporter_common;

    template <typename Interface>
    class tracker_common;

    namespace tag
    {
      struct reporter_base
      {
        using base_tag = reporter_base;

        template <typename LocalBaseTag, typename RemoteBaseTag>
        using type = detail::reporter_base<LocalBaseTag, RemoteBaseTag>;
      };

      struct tracker_base
      {
        using base_tag = tracker_base;

        template <typename, typename RemoteBaseTag>
        using type = detail::tracker_base<RemoteBaseTag>;
      };

      template <typename Parent, typename IntrusiveTag>
      struct reporter : reporter_base
      {
        using reduced_tag = reporter;

        template <typename LocalTag>
        using parent_type = Parent;

        template <typename LocalTag>
        using interface_type = gch::reporter<Parent, LocalTag, IntrusiveTag>;

        template <typename LocalTag>
        using common_type = detail::reporter_common<interface_type<LocalTag>>;
      };

      template <typename Parent, typename IntrusiveTag>
      struct tracker : tracker_base
      {
        using reduced_tag = tracker;

        template <typename LocalTag>
        using parent_type = Parent;

        template <typename LocalTag>
        using interface_type = gch::tracker<Parent, LocalTag, IntrusiveTag>;

        template <typename LocalTag>
        using common_type = detail::tracker_common<interface_type<LocalTag>>;
      };

      struct standalone_reporter : reporter_base
      {
        using reduced_tag = standalone_reporter;

        template <typename LocalTag>
        using parent_type = gch::standalone_reporter<LocalTag>;

        template <typename LocalTag>
        using interface_type = gch::reporter<gch::standalone_reporter<LocalTag>,
                                             LocalTag, gch::tag::intrusive>;

        template <typename LocalTag>
        using common_type = detail::reporter_common<interface_type<LocalTag>>;
      };

      struct standalone_tracker : tracker_base
      {
        using reduced_tag = standalone_tracker;

        template <typename LocalTag>
        using parent_type = gch::standalone_tracker<LocalTag>;

        template <typename LocalTag>
        using interface_type = gch::tracker<gch::standalone_tracker<LocalTag>,
                                            LocalTag, gch::tag::intrusive>;

        template <typename LocalTag>
        using common_type = detail::tracker_common<interface_type<LocalTag>>;
      };

      template <typename RemoteTag>
      struct reporter<gch::standalone_reporter<RemoteTag>, gch::tag::intrusive>
      {
        using reduced_tag = standalone_reporter;
      };

      template <typename RemoteTag>
      struct tracker<gch::standalone_tracker<RemoteTag>, gch::tag::intrusive>
      {
        using reduced_tag = standalone_tracker;
      };

      template <typename Tag, typename = void>
      struct is_reporter : std::false_type { };

      template <typename Tag>
      struct is_reporter<Tag,
                         typename std::enable_if<
                           std::is_base_of<detail::tag::reporter_base, Tag>::value>::type>
        : std::true_type
      { };

      template <typename Tag, typename = void>
      struct is_tracker : std::false_type { };

      template <typename Tag>
      struct is_tracker<Tag,
                        typename std::enable_if<
                          std::is_base_of<detail::tag::tracker_base, Tag>::value>::type>
        : std::true_type
      { };

      template <typename Tag, typename Type = void>
      using enable_if_reporter_t = typename std::enable_if<is_reporter<Tag>::value, Type>::type;

      template <typename Tag, typename Type = void>
      using enable_if_tracker_t = typename std::enable_if<is_tracker<Tag>::value, Type>::type;

      template <typename Tag>
      using enable_if_reporter_b = enable_if_reporter_t<Tag, bool>;

      template <typename Tag, typename = void>
      using enable_if_tracker_b = enable_if_tracker_t<Tag, bool>;

      // for asymmetric constructors
      GCH_INLINE_VARS constexpr struct track_t { track_t (void) = default; } track;
    }
  }

  namespace remote
  {
    template <typename Parent,
      typename IntrusiveTag = tag::nonintrusive>
    using reporter = detail::tag::reporter<Parent, IntrusiveTag>;

    template <typename Parent,
      typename IntrusiveTag = tag::nonintrusive>
    using tracker = detail::tag::tracker<Parent, IntrusiveTag>;

    using standalone_reporter = detail::tag::standalone_reporter;
    using standalone_tracker  = detail::tag::standalone_tracker;

    template <typename Parent>
    using intrusive_reporter = remote::reporter<Parent, tag::intrusive>;

    template <typename Parent>
    using intrusive_tracker = remote::tracker<Parent, tag::intrusive>;
  }

  namespace detail
  {
    template <typename ...Ts>
    using tracker_container = plf::list<Ts...>;

    template <typename Derived, typename RemoteBase>
    class reporter_base_common
    {
    public:
      using derived_type     = Derived;
      using remote_base_type = RemoteBase;
      using remote_base_ptr  = remote_base_type *;

      reporter_base_common            (void)                            = default;
      reporter_base_common            (const reporter_base_common&)     = default;
      reporter_base_common            (reporter_base_common&&) noexcept = default;
      reporter_base_common& operator= (const reporter_base_common&)     = default;
      reporter_base_common& operator= (reporter_base_common&&) noexcept = default;
      ~reporter_base_common           (void)                            = default;

      constexpr reporter_base_common (tag::track_t, remote_base_type& remote) noexcept
        : m_remote_base (&remote)
      { }

      void swap (reporter_base_common& other) noexcept
      {
        using std::swap;
        swap (this->m_remote_base, other.m_remote_base);
      }

      void track (remote_base_type& remote) noexcept
      {
        m_remote_base = &remote;
      }

      GCH_NODISCARD
      constexpr bool is_tracked (void) const noexcept
      {
        return nullptr != m_remote_base;
      }

      GCH_NODISCARD
      constexpr bool is_tracking (remote_base_type& remote) const noexcept
      {
        return &remote == m_remote_base;
      }

      GCH_NODISCARD
      constexpr remote_base_type& get_remote_base (void) const noexcept
      {
        return *m_remote_base;
      }

      // explanation: while a reference to the remote base will not
      //              affect *this, we still may want to ensure we
      //              are getting a const remote.
      GCH_NODISCARD
      constexpr const remote_base_type& get_const_remote_base (void) const noexcept
      {
        return *m_remote_base;
      }

      // local asymmetric debind (unsafe)
      void wipe (void) noexcept
      {
        m_remote_base = nullptr;
      }

      // symmetric debind (safe)
      GCH_CPP14_CONSTEXPR void debind (void) noexcept
      {
        static_cast<derived_type *> (this)->reset_remote_tracking ();
        wipe ();
      }

      GCH_CPP14_CONSTEXPR void reset (void) noexcept
      {
        debind ();
      }

    private:
      remote_base_ptr m_remote_base = nullptr;
    };

    template <typename LocalBaseTag>
    class reporter_base<LocalBaseTag, tag::reporter_base>
      : public reporter_base_common<reporter_base<LocalBaseTag, tag::reporter_base>,
                                    reporter_base<tag::reporter_base, LocalBaseTag>>
    {
    public:
      using local_base_tag       = LocalBaseTag;
      using remote_base_tag      = tag::reporter_base;

      using local_base_type      = reporter_base;
      using remote_base_type     = reporter_base<remote_base_tag, local_base_tag>;

      using local_reporter_type  = reporter_base;
      using remote_reporter_type = remote_base_type;

      using base = reporter_base_common<reporter_base, remote_base_type>;

      using base::base;
      reporter_base            (void)                           = default;
      reporter_base            (const reporter_base&)           = default;
      reporter_base            (reporter_base&& other) noexcept = default;
      reporter_base& operator= (const reporter_base&)           = default;
      reporter_base& operator= (reporter_base&& other) noexcept = default;
      ~reporter_base           (void)                           = default;

      reporter_base (gch::tag::bind_t, remote_base_type& remote)
        : base (tag::track, remote)
      {
        remote.reset_remote_tracking ();
        remote.track (*this);
      }

      // asymmetric remote debinding
      void reset_remote_tracking (void) const noexcept
      {
        if (base::is_tracked ())
          base::get_remote_base ().wipe ();
      }

      reporter_base& rebind (remote_base_type& new_remote) noexcept
      {
        if (! base::is_tracking (new_remote))
        {
          reset_remote_tracking ();
          base::track (new_remote);
          new_remote.track (*this);
        }
        return *this;
      }

      GCH_NODISCARD
      constexpr std::size_t get_position (void) const noexcept
      {
        return 0;
      }

      GCH_NODISCARD
      constexpr remote_base_type& get_remote_reporter (void) const noexcept
      {
        return base::get_remote_base ();
      }

      GCH_NODISCARD
      constexpr const remote_base_type& get_const_remote_reporter (void) const noexcept
      {
        return base::get_remote_base ();
      }

      reporter_base& set (remote_base_type& remote) noexcept
      {
        base::track (remote);
        return *this;
      }
    };

    template <typename LocalBaseTag>
    class reporter_base<LocalBaseTag, tag::tracker_base>
      : public reporter_base_common<reporter_base<LocalBaseTag, tag::tracker_base>,
                                    tracker_base<LocalBaseTag>>
    {
    public:
      using local_base_tag       = LocalBaseTag;
      using remote_base_tag      = tag::tracker_base;

      using local_base_type      = reporter_base;
      using remote_base_type     = tracker_base<local_base_tag>;

      using local_reporter_type  = reporter_base;
      using remote_reporter_type = reporter_base<remote_base_tag, local_base_tag>;

      using base = reporter_base_common<reporter_base, remote_base_type>;

    private:
      using remote_access_type  = typename remote_base_type::access_type;

    public:
      reporter_base            (void)                     = default;
      reporter_base            (const reporter_base&)     = default;
      reporter_base            (reporter_base&&) noexcept = default;
      reporter_base& operator= (const reporter_base&)     = default;
      reporter_base& operator= (reporter_base&&) noexcept = default;
      ~reporter_base           (void)                     = default;

      // Note that the destructor here does NOT perform a debinding. This is
      // for optimizations higher in the hierarchy.

      reporter_base (gch::tag::bind_t, remote_base_type& remote)
        : base   (tag::track, remote),
          m_self (remote.track (*this))
      { }

      constexpr reporter_base (remote_base_type& remote, remote_access_type it) noexcept
        : base   (tag::track, remote),
          m_self (it)
      { }

      // remote asymmetric debind
      GCH_CPP14_CONSTEXPR void reset_remote_tracking (void) const noexcept
      {
        if (base::is_tracked ())
          base::get_remote_base ().rptrs_erase (m_self);
      }

      GCH_NODISCARD
      std::size_t get_position (void) const noexcept
      {
        if (! base::is_tracked ())
          return 0;
        return base::get_remote_base ().base_get_offset (m_self);
      }

      reporter_base& rebind (remote_base_type& new_remote)
      {
        // if the remotes are the same then this is already in the remote,
        // so we shouldn't do anything unless we aren't being tracked right now.
        if (! base::is_tracking (new_remote))
          {
            // might fail
            remote_access_type new_iter = new_remote.track (*this);

            // if we didn't fail the rest is noexcept
            reset_remote_tracking ();
            set (new_remote, new_iter);
          }
        else if (! base::is_tracked ())
          {
            // we already point to new_remote, but we aren't tracked
            // note that that the above condition implies that has_remote () == true
            // since &new_remote cannot be nullptr.
            m_self = new_remote.track (*this);
          }
        return *this;
      }

      // WARNING! This is a pure swap. Move to private when ready.
      void swap (reporter_base& other) noexcept
      {
        base::swap (other);

        using std::swap;
        swap (this->m_self, other.m_self);
      }

      GCH_NODISCARD
      constexpr remote_reporter_type&
      get_remote_reporter (void) const noexcept
      {
        return *m_self;
      }

      GCH_NODISCARD
      constexpr const remote_reporter_type&
      get_const_remote_reporter (void) const noexcept
      {
        return *m_self;
      }

  //  protected:

      reporter_base& set (remote_base_type& remote, remote_access_type it) noexcept
      {
        base::track (remote);
        m_self = it;
        return *this;
      }

      reporter_base& set_access (remote_access_type it) noexcept
      {
        m_self = it;
        return *this;
      }

    private:
      remote_access_type m_self;
    };

    template <typename RemoteBaseTag>
    class tracker_base_common
    {
      using derived_type = tracker_base<RemoteBaseTag>;
    public:

      using local_base_tag  = tag::tracker_base;
      using remote_base_tag = RemoteBaseTag;

    protected:
      // Store pointers to the base types. Downcast when needed.
      using reporter_type   = reporter_base<local_base_tag, remote_base_tag>;
      using reporter_list   = tracker_container<reporter_type>;
      using rptr_iter       = typename reporter_list::iterator;
      using rptr_citer      = typename reporter_list::const_iterator;
      using rptr_riter      = typename reporter_list::reverse_iterator;
      using rptr_criter     = typename reporter_list::const_reverse_iterator;
      using rptr_ref        = typename reporter_list::reference;
      using rptr_cref       = typename reporter_list::const_reference;

    public:

      using access_type = rptr_iter;

      using local_reporter_type = reporter_type;

      using remote_base_type = typename remote_base_tag
                                          ::template type<remote_base_tag, local_base_tag>;

      tracker_base_common            (void)                           = default;
      tracker_base_common            (const tracker_base_common&)     = delete;
//    tracker_base_common            (tracker_base_common&&) noexcept = impl;
      tracker_base_common& operator= (const tracker_base_common&)     = delete;
//    tracker_base_common& operator= (tracker_base_common&&) noexcept = impl;
      ~tracker_base_common           (void)                           = default;

      tracker_base_common (tracker_base_common&& other) noexcept
      {
        base_transfer_bindings (other, rptrs_cend ());
      }

      tracker_base_common& operator= (tracker_base_common&& other) noexcept
      {
        rptr_iter pivot = base_transfer_bindings (other, rptrs_cend ());
        base_debind (rptrs_begin (), pivot);
        return *this;
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

      void swap (tracker_base_common& other) noexcept
      {
        // kinda expensive
        m_rptrs.swap (other.m_rptrs);
        this->repoint_reporters (this->rptrs_begin (), this->rptrs_end ());
        other.repoint_reporters (other.rptrs_begin (), other.rptrs_end ());
      }

      GCH_NODISCARD
      std::size_t num_remotes (void) const noexcept
      {
        return m_rptrs.size ();
      }

      rptr_iter base_transfer_bindings (tracker_base_common& src, rptr_citer pos) noexcept
      {
        rptr_iter pivot = src.rptrs_begin ();
        repoint_reporters (src.rptrs_begin (), src.rptrs_end ());
        m_rptrs.splice (pos, src.m_rptrs);
        return pivot;
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

      template <typename ...Args>
      rptr_iter rptrs_emplace (rptr_citer pos, Args&&... args)
      {
        return m_rptrs.emplace (pos, std::forward<Args> (args)...);
      }

      template <typename ...Args>
      rptr_iter rptrs_erase (Args&&... args) noexcept
      {
        return m_rptrs.erase (std::forward<Args> (args)...);
      }

      // safe to use, but may throw
      rptr_iter track (remote_base_type& remote)
      {
        return m_rptrs.emplace (rptrs_end (), tag::track, remote);
      }

      GCH_NODISCARD
      GCH_CPP17_CONSTEXPR typename std::iterator_traits<rptr_citer>::difference_type
      base_get_offset (rptr_citer pos) const noexcept
      {
        return std::distance (rptrs_cbegin (), pos);
      }

      void repoint_reporters (rptr_iter first, rptr_iter last) noexcept
      {
        std::for_each (first, last,
          [this] (reporter_type& rptr)
          {
            rptr.get_remote_reporter ().track (static_cast<derived_type&> (*this));
          });
      }

      //! safe, symmetric
      rptr_iter base_debind (rptr_citer pos)
      {
        pos->reset_remote_tracking ();
        return rptrs_erase (pos);
      }

      //! safe
      rptr_iter base_debind (rptr_citer first, const rptr_citer last)
      {
        while (first != last)
          first = base_debind (first);
        return rptrs_erase (last, last);
      }
  
      //! safe
      template <typename Pred>
      void base_remove_if (Pred pred)
      {
        std::for_each (m_rptrs.begin (), m_rptrs.end (),
                       [&pred] (const reporter_type& e)
                       {
                         if (pred (e))
                           e.reset_remote_tracking ();
                       });
        m_rptrs.remove_if (pred);
      }

    private:
      reporter_list m_rptrs;
    };

    template <>
    class tracker_base<tag::reporter_base>
      : public tracker_base_common<tag::reporter_base>
    {
    public:
      using remote_reporter_type = remote_base_type;

      tracker_base            (void)                    = default;
      tracker_base            (const tracker_base&)     = delete;
      tracker_base            (tracker_base&&) noexcept = default;
      tracker_base& operator= (const tracker_base&)     = delete;
      tracker_base& operator= (tracker_base&&) noexcept = default;
      ~tracker_base           (void)                    = default;

      tracker_base (const rptr_citer first, const rptr_citer last)
      {
        base_bind_back (first, last);
      }

      rptr_iter base_bind_before (const rptr_citer pos, remote_base_type& remote)
      {
        const rptr_iter local_it = rptrs_emplace (pos, tag::track, remote);
        remote.debind ();
        remote.set (*this, local_it);
        return local_it;
      }

      rptr_iter base_bind_before (const rptr_citer pos,
                                  const rptr_citer first, const rptr_citer last)
      {
        if (first == last)
          return rptrs_erase (pos, pos);

        const rptr_iter pivot = base_bind_before (pos, first->get_remote_base ());
        try
        {
          std::for_each (std::next (first), last,
                         [this, pos] (const reporter_type& reporter)
                         {
                           base_bind_before (pos, reporter.get_remote_base ());
                         });
        }
        catch (...)
        {
          base_debind (pivot, pos);
          throw;
        }
        return pivot;
      }

      rptr_iter base_bind_front (remote_base_type& remote)
      {
        return base_bind_before (rptrs_cbegin (), remote);
      }

      rptr_iter base_bind_front (const rptr_citer first, const rptr_citer last)
      {
        return base_bind_before (rptrs_cbegin (), first, last);
      }

      rptr_iter base_bind_back (remote_base_type& remote)
      {
        return base_bind_before (rptrs_cend (), remote);
      }

      rptr_iter base_bind_back (const rptr_citer first, const rptr_citer last)
      {
        return base_bind_before (rptrs_cend (), first, last);
      }
    };

    //! Remote is one of the non-base classes defined here
    template <>
    class tracker_base<tag::tracker_base>
      : public tracker_base_common<tag::tracker_base>
    {
    public:
      using remote_reporter_type = typename remote_base_type::local_reporter_type;

      tracker_base            (void)                    = default;
      tracker_base            (const tracker_base&)     = delete;
      tracker_base            (tracker_base&&) noexcept = default;
      tracker_base& operator= (const tracker_base&)     = delete;
      tracker_base& operator= (tracker_base&&) noexcept = default;
      ~tracker_base           (void)                    = default;

      tracker_base (const rptr_citer first, const rptr_citer last)
      {
        base_bind_back (first, last);
      }

      rptr_iter base_bind_before (const rptr_citer pos, remote_base_type& remote)
      {
        const rptr_iter local_it = rptrs_emplace (pos);
        try
        {
          const rptr_iter remote_it = remote.rptrs_emplace (remote.rptrs_end ());
          local_it ->set (remote, remote_it);
          remote_it->set (*this , local_it );
        }
        catch (...)
        {
          rptrs_erase (local_it);
          throw;
        }
        return local_it;
      }

      rptr_iter base_bind_before (const rptr_citer pos,
                                  const rptr_citer first, const rptr_citer last)
      {
        if (first == last)
          return rptrs_erase (pos, pos);

        const rptr_iter pivot = base_bind_before (pos, first->get_remote_base ());
        try
        {
          std::for_each (std::next (first), last,
                         [this, pos] (const reporter_type& reporter)
                         {
                           base_bind_before (pos, reporter.get_remote_base ());
                         });
        }
        catch (...)
        {
          base_debind (pivot, pos);
          throw;
        }
        return pivot;
      }

      rptr_iter base_bind_front (remote_base_type& remote)
      {
        return base_bind_before (rptrs_cbegin (), remote);
      }

      rptr_iter base_bind_front (const rptr_citer first, const rptr_citer last)
      {
        return base_bind_before (rptrs_cbegin (), first, last);
      }

      rptr_iter base_bind_back (remote_base_type& remote)
      {
        return base_bind_before (rptrs_cend (), remote);
      }

      rptr_iter base_bind_back (const rptr_citer first, const rptr_citer last)
      {
        return base_bind_before (rptrs_cend (), first, last);
      }
    }; // tracker_base

    template <typename Interface>
    class reporter_common;

    template <typename Interface>
    class tracker_common;

    /////////////////////
    // reporter_common //
    /////////////////////

    template <typename Parent, typename RemoteTag, typename IntrusiveTag>
    class reporter_common<reporter<Parent, RemoteTag, IntrusiveTag>>
      : private reporter_base<tag::reporter_base, typename RemoteTag::base_tag>
    {
    public:
      using base = reporter_base<tag::reporter_base, typename RemoteTag::base_tag>;

      using local_interface_type = reporter<Parent, RemoteTag, IntrusiveTag>;

      using local_tag             = typename tag::reporter<Parent, IntrusiveTag>::reduced_tag;
      using remote_tag            = RemoteTag;

      using local_type            = Parent;
      using remote_type           = typename remote_tag::template parent_type<local_tag>;

      using local_base_tag        = typename local_tag::base_tag;
      using remote_base_tag       = typename remote_tag::base_tag;

      using remote_interface_type = typename remote_tag::template interface_type<local_tag>;
      using remote_common_type    = typename remote_tag::template common_type<local_tag>;
      using remote_base_type      = typename base::remote_base_type;

      friend remote_common_type;

    private:
      using local_reporter_type  = base;
      using remote_reporter_type = typename base::remote_reporter_type;

    public:
      using base::debind;
      using base::get_position;
      using base::reset;
      using base::wipe;

      reporter_common            (void)                       = default;
      reporter_common            (const reporter_common&)     = delete;
//    reporter_common            (reporter_common&&) noexcept = impl;
      reporter_common& operator= (const reporter_common&)     = delete;
//    reporter_common& operator= (reporter_common&&) noexcept = impl;
//    ~reporter_common           (void)                       = impl;

//      static_assert (std::is_trivially_copy_constructible<base>::value, "");
//      static_assert (std::is_trivially_move_constructible<base>::value, "");
//      static_assert (std::is_trivially_copy_assignable<base>::value, "");
//      static_assert (std::is_trivially_move_assignable<base>::value, "");
//
//      static_assert (std::is_trivially_copyable<base>::value, "base was not trivially copyable");

      reporter_common (reporter_common&& other) noexcept
        : base (other) // the base is trivially copyable
      {
        if (has_remote ())
          base::get_remote_reporter ().track (*this);
        other.wipe ();
      }

      reporter_common& operator= (reporter_common&& other) noexcept
      {
        if (&other != this)
        {
          base::reset_remote_tracking ();
          if (other.is_tracked ())
          {
            base::operator= (other); // the base is trivially copyable
            base::get_remote_reporter ().track (*this);
            other.wipe ();
          }
          else
          {
            wipe ();
          }
        }
        return *this;
      }

      ~reporter_common (void)
      {
        if (has_remote ())
          base::reset_remote_tracking ();
      }

      void swap (reporter_common& other) noexcept
      {
        if (&other.get_remote_base () != &this->get_remote_base ())
        {
          using std::swap;
          base::swap (other);

          if (this->is_tracked ())
            this->get_remote_reporter ().track (*this);

          if (other.is_tracked ())
            other.get_remote_reporter ().track (other);
        }
      }

      local_interface_type& replace_binding (local_interface_type&& other)
      {
        return static_cast<local_interface_type&> (operator= (std::move (other)));
      }

      GCH_NODISCARD
      constexpr bool has_remote (void) const noexcept
      {
        return base::is_tracked ();
      }

      GCH_NODISCARD
      constexpr bool has_remote (remote_interface_type& remote) const noexcept
      {
        return base::is_tracking (remote);
      }

      GCH_NODISCARD
      constexpr bool has_remote (remote_type& remote) const noexcept
      {
        return has_remote () && (&remote == &get_remote ());
      }

      GCH_NODISCARD
      constexpr remote_type& get_remote (void) const noexcept
      {
        return get_remote_interface ().get_parent ();
      }
  
      GCH_NODISCARD
      constexpr remote_type * get_remote_ptr (void) const noexcept
      {
        return has_remote () ? &get_remote_interface ().get_parent () : nullptr;
      }

      GCH_NODISCARD
      constexpr remote_interface_type& get_remote_interface (void) const noexcept
      {
        return static_cast<remote_interface_type&> (base::get_remote_base ());
      }

      local_interface_type& rebind (remote_interface_type& new_remote)
      {
        return static_cast<local_interface_type&> (base::rebind (new_remote));
      }

      //! disabled for reporters

      template <typename Tag = remote_tag, tag::enable_if_reporter_b<Tag> = false>
      reporter_common (gch::tag::bind_t, remote_interface_type& remote)
        : base (gch::tag::bind, remote)
      { }

      local_interface_type& move_binding (local_interface_type& other)
      {
        return static_cast<local_interface_type&> (operator= (std::move (other)));
      }

      local_interface_type& move_binding (local_interface_type&& other)
      {
        return move_binding (other);
      }

      template <typename Tag = remote_tag, tag::enable_if_reporter_b<Tag> = false>
      reporter_common clone (void) const = delete;

      template <typename Tag = remote_tag, tag::enable_if_reporter_b<Tag> = false>
      local_interface_type& copy_binding (const local_interface_type& other) = delete;

      //! enabled for trackers

      template <typename Tag = remote_tag, tag::enable_if_tracker_b<Tag> = true>
      constexpr reporter_common (gch::tag::bind_t, remote_interface_type& remote) noexcept
        : base (gch::tag::bind, remote)
      { }

      template <typename Tag = remote_tag,
                tag::enable_if_tracker_b<Tag> = true>
      GCH_NODISCARD
      reporter_common clone (void) const
      {
        if (has_remote ())
          return reporter_common (gch::tag::bind, get_remote_interface ());
        return { };
      }

      template <typename Tag = remote_tag,
                tag::enable_if_tracker_b<Tag> = true>
      local_interface_type& copy_binding (const local_interface_type& other)
      {
        if (&other != this)
        {
          if (other.is_tracked ())
            base::rebind (other.get_remote_base ());
          else
            base::debind ();
        }
        return static_cast<local_interface_type&> (*this);
      }

    private:
    }; // reporter_common

    ////////////////////
    // tracker_common //
    ////////////////////

    template <typename Parent, typename RemoteTag, typename IntrusiveTag>
    class tracker_common<tracker<Parent, RemoteTag, IntrusiveTag>>
      : private tracker_base<typename RemoteTag::base_tag>
    {
    public:
      using base                  = tracker_base<typename RemoteTag::base_tag>;

      using local_interface_type  = tracker<Parent, RemoteTag, IntrusiveTag>;

      using local_tag             = typename tag::tracker<Parent, IntrusiveTag>::reduced_tag;
      using remote_tag            = RemoteTag;

      using local_type            = Parent;
      using remote_type           = typename remote_tag::template parent_type<local_tag>;

      using local_base_tag        = typename local_tag::base_tag;
      using remote_base_tag       = typename remote_tag::base_tag;

      using remote_interface_type = typename remote_tag::template interface_type<local_tag>;
      using remote_common_type    = typename remote_tag::template common_type<local_tag>;
      using remote_base_type      = typename base::remote_base_type;

      friend remote_common_type;

      class iter;
      class citer;

    private:
      using local_reporter_type  = typename base::local_reporter_type;
      using remote_reporter_type = typename base::remote_reporter_type;

      using rptr_iter            = typename base::rptr_iter;
      using rptr_citer           = typename base::rptr_citer;

      // pretend like reporter_base doesn't exist
      template <typename ReporterIt>
      class iter_common
      {
        using reporter_iter         = ReporterIt;
        using reporter_type         = typename ReporterIt::value_type;

      public:
        using difference_type   = typename reporter_iter::difference_type;
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = remote_type;
        using pointer           = remote_type *;
        using reference         = remote_type&;

        friend tracker_common;

        iter_common            (void)                     = default;
        iter_common            (const iter_common&)       = default;
        iter_common            (iter_common&&) noexcept   = default;
        iter_common& operator= (const iter_common&) &     = default;
        iter_common& operator= (iter_common&&) & noexcept = default;
        ~iter_common (void)                               = default;

      protected:
        constexpr /* implicit */ iter_common (reporter_iter it)
            : m_iter (it)
        { }

      public:
        iter_common& operator++ (void) noexcept
        {
          ++m_iter;
          return *this;
        }

        iter_common operator++ (int) noexcept
        {
          const iter_common tmp = *this;
          ++m_iter;
          return tmp;
        }

        iter_common& operator-- (void) noexcept
        {
          --m_iter;
          return *this;
        }

        iter_common operator-- (int) noexcept
        {
          const iter_common tmp = *this;
          --m_iter;
          return tmp;
        }

        friend bool operator== (const iter_common& l,
                                const iter_common& r) noexcept
        {
          return l.m_iter == r.m_iter;
        }

        friend bool operator!= (const iter_common& l,
                                const iter_common& r) noexcept
        {
          return l.m_iter != r.m_iter;
        }

        constexpr remote_interface_type& get_remote_interface (void) const noexcept
        {
          return static_cast<remote_interface_type&> (m_iter->get_remote_base ());
        }

      protected:
        /* implicit */ operator reporter_iter (void) const noexcept
        {
          return m_iter;
        }

      private:
        reporter_iter m_iter;
      }; // iter_common

    public:

      class iter : public iter_common<rptr_iter>
      {
        using base = iter_common<rptr_iter>;
      public:

        friend tracker_common;
        friend citer;

        using base::base;

        iter            (void)              = default;
        iter            (const iter&)       = default;
        iter            (iter&&) noexcept   = default;
        iter& operator= (const iter&) &     = default;
        iter& operator= (iter&&) & noexcept = default;
        ~iter           (void)              = default;

        constexpr remote_type& operator* (void) const noexcept
        {
          return get_remote ();
        }

        constexpr remote_type * operator-> (void) const noexcept
        {
          return &get_remote ();
        }

        constexpr remote_type& get_remote (void) const noexcept
        {
          return base::get_remote_interface ().get_parent ();
        }
      }; // iter

      class citer : public iter_common<rptr_citer>
      {
        using base = iter_common<rptr_citer>;
      public:

        friend tracker_common;

        using base::base;

        citer            (void)               = default;
        citer            (const citer&)       = default;
        citer            (citer&&) noexcept   = default;
        citer& operator= (const citer&) &     = default;
        citer& operator= (citer&&) & noexcept = default;
        ~citer           (void)               = default;

        constexpr /* implicit */ citer (iter it) noexcept
          : base (rptr_iter (it))
        { }

        constexpr const remote_type& operator* (void) const noexcept
        {
          return get_remote ();
        }

        constexpr const remote_type * operator-> (void) const noexcept
        {
          return &get_remote ();
        }

        constexpr const remote_type& get_remote (void) const noexcept
        {
          return base::get_remote_interface ().get_parent ();
        }

      }; // citer

      using riter  = std::reverse_iterator<iter>;
      using criter = std::reverse_iterator<citer>;
      using ref    = remote_type&;
      using cref   = const remote_type&;

      using init_list = std::initializer_list<
        std::reference_wrapper<remote_interface_type>>;

      using base::clear;
      using base::wipe;
      using base::num_remotes;

      tracker_common            (void)                      = default;
      tracker_common            (const tracker_common&)     = delete;
      tracker_common            (tracker_common&&) noexcept = default;
      tracker_common& operator= (const tracker_common&)     = delete;
      tracker_common& operator= (tracker_common&&) noexcept = default;
//    ~tracker_common           (void)                      = impl;

      //! safe by default
      ~tracker_common (void)
      {
        base::clear ();
      }

      tracker_common (citer first, citer last)
        : base (first, last)
      { }

      tracker_common (gch::tag::bind_t, init_list init)
      {
        try
        {
          for (auto&& remote : init)
            base::base_bind_back (remote);
        }
        catch (...)
        {
          base::base_debind (base::rptrs_begin (), base::rptrs_cend ());
          throw;
        }
      }

      void swap (tracker_common& other) noexcept
      {
        return base::swap (other);
      }

      GCH_NODISCARD iter   begin         (void)       noexcept { return base::rptrs_begin ();   }
      GCH_NODISCARD citer  begin         (void) const noexcept { return base::rptrs_begin ();   }
      GCH_NODISCARD citer  cbegin        (void) const noexcept { return base::rptrs_cbegin ();  }

      GCH_NODISCARD iter   end           (void)       noexcept { return base::rptrs_end ();     }
      GCH_NODISCARD citer  end           (void) const noexcept { return base::rptrs_end ();     }
      GCH_NODISCARD citer  cend          (void) const noexcept { return base::rptrs_cend ();    }

      GCH_NODISCARD riter  rbegin        (void)       noexcept { return riter (end ());         }
      GCH_NODISCARD criter rbegin        (void) const noexcept { return riter (end ());         }
      GCH_NODISCARD criter crbegin       (void) const noexcept { return criter (cend ());       }

      GCH_NODISCARD riter  rend          (void)       noexcept { return riter (begin ());       }
      GCH_NODISCARD criter rend          (void) const noexcept { return riter (begin ());       }
      GCH_NODISCARD criter crend         (void) const noexcept { return criter (cbegin ());     }

      GCH_NODISCARD ref    front         (void)                { return *begin ();              }
      GCH_NODISCARD cref   front         (void) const          { return *begin ();              }

      GCH_NODISCARD ref    back          (void)                { return *--end ();              }
      GCH_NODISCARD cref   back          (void) const          { return *--end ();              }

      GCH_NODISCARD bool   has_remotes   (void) const noexcept { return ! base::rptrs_empty (); }

      GCH_NODISCARD
      GCH_CPP17_CONSTEXPR std::size_t get_offset (citer pos) const noexcept
      {
        return base::base_get_offset (pos);
      }
      
      void debind (const remote_type& r)
      {
        auto find_pred = [&r] (const remote_type& e) { return &e == &r; };
        iter pos = std::find_if (begin (), end (), find_pred);
        while (pos != end ())
          pos = std::find_if (debind (pos), end (), find_pred);
      }

      iter debind (citer pos)
      {
        return base::base_debind (pos);
      }

      iter debind (citer first, citer last)
      {
        return base::base_debind (first, last);
      }

      //! transfers all bindings from `other`; no overwriting
      GCH_CPP14_CONSTEXPR iter
      move_bindings (local_interface_type& other, citer after) noexcept
      {
        return base::base_transfer_bindings (other, after);
      }

      GCH_CPP14_CONSTEXPR iter
      move_bindings (local_interface_type& other) noexcept
      {
        return move_bindings (other, cend ());
      }

      GCH_CPP14_CONSTEXPR iter
      move_bindings (local_interface_type&& other, citer after) noexcept
      {
        return move_bindings (other, after);
      }

      GCH_CPP14_CONSTEXPR iter
      move_bindings (local_interface_type&& other) noexcept
      {
        return move_bindings (other, cend ());
      }

      local_interface_type& move_replace_bindings (local_interface_type& other)
      {
        return static_cast<local_interface_type&> (base::operator= (std::move (other)));
      }

      local_interface_type& move_replace_bindings (local_interface_type&& other)
      {
        return move_replace_bindings (other);
      }

      //! disabled for reporters

      template <typename Tag = remote_tag,
        tag::enable_if_reporter_b<Tag> = false>
      local_interface_type& copy_replace_bindings (const local_interface_type& other) = delete;

      template <typename Tag = remote_tag,
                tag::enable_if_reporter_b<Tag> = false>
      iter copy_bindings (const local_interface_type& other) = delete;

      template <typename Tag = remote_tag,
                tag::enable_if_reporter_b<Tag> = false>
      tracker_common clone (void) const = delete;

      template <typename ...Args,
                typename Tag = remote_tag,
                tag::enable_if_reporter_b<Tag> = false>
      iter bind (Args&&...) = delete;

      //! enabled for trackers


      template <typename Tag = remote_tag,
                tag::enable_if_tracker_b<Tag> = true>
      local_interface_type& copy_replace_bindings (const local_interface_type& other)
      {
        if (&other != this)
        {
          auto pivot = base::base_bind_back (other.rptrs_cbegin (), other.rptrs_cend ());
          base::base_debind (base::rptrs_cbegin (), pivot);
        }
        return static_cast<local_interface_type&> (*this);
      }

      template <typename Tag = remote_tag,
                tag::enable_if_tracker_b<Tag> = true>
      iter copy_bindings (const local_interface_type& other)
      {
        return bind (other.begin (), other.end ());
      }

      template <typename Tag = remote_tag,
                tag::enable_if_tracker_b<Tag> = true>
      GCH_NODISCARD
      tracker_common clone (void) const
      {
        return tracker_common (begin (), end ());
      }

      template <typename Tag = remote_tag,
                tag::enable_if_tracker_b<Tag> = true>
      iter bind (citer first, citer last)
      {
        return base::base_bind_back (rptr_citer (first), rptr_citer (last));
      }

      template <typename Tag = remote_tag,
                tag::enable_if_tracker_b<Tag> = true>
      iter bind (remote_interface_type& r)
      {
        return base::base_bind_back (r);
      }

      template <typename ...Args,
                typename Tag = remote_tag,
                tag::enable_if_tracker_b<Tag> = true>
      iter bind (remote_interface_type& r, Args&... args)
      {
        bind (args...);
        return bind (r);
      }
      
    private:
    }; // tracker_common

    /////////////////////////
    // nonintrusive_common //
    /////////////////////////

    template <typename Parent>
    class nonintrusive_common
    {
      using parent_type = Parent;
      
    public:
      nonintrusive_common            (void)                           = default;
      nonintrusive_common            (const nonintrusive_common&)     = default;
      nonintrusive_common            (nonintrusive_common&&) noexcept = default;
      nonintrusive_common& operator= (const nonintrusive_common&)     = delete;
      nonintrusive_common& operator= (nonintrusive_common&&) noexcept = delete;
      ~nonintrusive_common           (void)                           = default;

      void swap (nonintrusive_common& other) = delete;

      constexpr explicit nonintrusive_common (parent_type& parent)
        : m_parent (parent)
      { }

      GCH_NODISCARD
      GCH_CPP14_CONSTEXPR parent_type& get_parent (void) noexcept
      {
        return m_parent;
      }

      GCH_NODISCARD
      constexpr const parent_type& get_parent (void) const noexcept
      {
        return m_parent;
      }

    private:
      parent_type& m_parent;
    }; // nonintrusive_common
    
  } // detail

  //////////////////////////
  // reporter (intrusive) //
  //////////////////////////

  template <typename Derived, typename RemoteTag>
  class reporter<Derived, RemoteTag, tag::intrusive>
    : public detail::reporter_common<reporter<Derived, RemoteTag, tag::intrusive>>
  {
  public:
    using base = detail::reporter_common<reporter<Derived, RemoteTag, tag::intrusive>>;

    using local_tag             = typename base::local_tag;
    using remote_tag            = RemoteTag;

    using derived_type          = Derived;
    using remote_type           = typename base::remote_type;

    using local_interface_type  = reporter;
    using remote_interface_type = typename base::remote_interface_type;

    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;

    using base::base;
    reporter            (void)                = default;
    reporter            (const reporter&)     = delete;
    reporter            (reporter&&) noexcept = default;
    reporter& operator= (const reporter&)     = delete;
    reporter& operator= (reporter&&) noexcept = default;
    ~reporter           (void)                = default;

    /* implicit */ reporter (base&& other) noexcept
      : base (std::move (other))
    { }

    GCH_NODISCARD
    GCH_CPP14_CONSTEXPR derived_type& get_parent (void) noexcept
    {
      return static_cast<derived_type&> (*this);
    }

    GCH_NODISCARD
    constexpr const derived_type& get_parent (void) const noexcept
    {
      return static_cast<const derived_type&> (*this);
    }
  }; // reporter

  /////////////////////////
  // tracker (intrusive) //
  /////////////////////////

  template <typename Derived, typename RemoteTag>
  class tracker<Derived, RemoteTag, tag::intrusive>
    : public detail::tracker_common<tracker<Derived, RemoteTag, tag::intrusive>>
  {
    using base = detail::tracker_common<tracker<Derived, RemoteTag, tag::intrusive>>;
    
  public:
    using local_tag             = typename base::local_tag;
    using remote_tag            = RemoteTag;

    using derived_type          = Derived;
    using remote_type           = typename base::remote_type;

    using local_interface_type  = tracker;
    using remote_interface_type = typename base::remote_interface_type;

    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;

    using base::base;
    tracker            (void)               = default;
    tracker            (const tracker&)     = delete;
    tracker            (tracker&&) noexcept = default;
    tracker& operator= (const tracker&)     = delete;
    tracker& operator= (tracker&&) noexcept = default;
    ~tracker           (void)               = default;

    /* implicit */ tracker (base&& other) noexcept
      : base (std::move (other))
    { }

    GCH_NODISCARD
    GCH_CPP14_CONSTEXPR derived_type& get_parent (void) noexcept
    {
      return static_cast<derived_type&> (*this);
    }

    GCH_NODISCARD
    constexpr const derived_type& get_parent (void) const noexcept
    {
      return static_cast<const derived_type&> (*this);
    }
  }; // tracker

  /////////////////////////////
  // reporter (nonintrusive) //
  /////////////////////////////

  template <typename Parent, typename RemoteTag>
  class reporter<Parent, RemoteTag, tag::nonintrusive>
    : public detail::reporter_common<reporter<Parent, RemoteTag, tag::nonintrusive>>,
      public detail::nonintrusive_common<Parent>
  {
    using base = detail::reporter_common<reporter<Parent, RemoteTag, tag::nonintrusive>>;
    using access_base = detail::nonintrusive_common<Parent>;

  public:
    using local_tag             = typename base::local_tag;
    using remote_tag            = RemoteTag;

    using parent_type           = Parent;
    using remote_type           = typename base::remote_type;

    using local_interface_type  = reporter;
    using remote_interface_type = typename base::remote_interface_type;

    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;

    using base::base;
    reporter            (void)                = default;
    reporter            (const reporter&)     = delete;
    reporter            (reporter&&) noexcept = delete;
    reporter& operator= (const reporter&)     = delete;
    reporter& operator= (reporter&&) noexcept = delete;
    ~reporter           (void)                = default;

    void swap (reporter& other)
    {
      base::swap (other);
    }

    reporter (base&& other, parent_type& parent) noexcept
      : base        (std::move (other)),
        access_base (parent)
    { }

    reporter (parent_type& parent, remote_interface_type& remote)
      : base        (tag::bind, remote),
        access_base (parent)
    { }

    reporter (reporter&& other, parent_type& parent) noexcept
      : base        (std::move (other)),
        access_base (parent)
    { }

    explicit reporter (parent_type& parent)
      : access_base (parent)
    { }
  }; // reporter

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

    using init_list = typename base::init_list;

  public:
    using local_tag             = typename base::local_tag;
    using remote_tag            = RemoteTag;

    using parent_type           = Parent;
    using remote_type           = typename base::remote_type;

    using local_interface_type  = tracker;
    using remote_interface_type = typename base::remote_interface_type;

    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;

    using iter                  = typename base::iter;
    using citer                 = typename base::citer;
    using riter                 = typename base::riter;
    using criter                = typename base::criter;
    using ref                   = typename base::ref;
    using cref                  = typename base::cref;

    using base::base;
    tracker            (void)               = default;
    tracker            (const tracker&)     = delete;
    tracker            (tracker&&) noexcept = delete;
    tracker& operator= (const tracker&)     = delete;
    tracker& operator= (tracker&&) noexcept = delete;
    ~tracker           (void)               = default;

    void swap (tracker& other) noexcept
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

    explicit tracker (parent_type& parent)
      : access_base (parent)
    { }

    template <typename Tag = remote_tag,
              detail::tag::enable_if_reporter_b<Tag> = false>
    tracker (parent_type& parent, init_list init) = delete;

    template <typename Tag = remote_tag,
              detail::tag::enable_if_tracker_b<Tag> = true>
    tracker (parent_type& parent, init_list init)
      : base        (tag::bind, init),
        access_base (parent)
    { }
  }; // tracker

  template <typename RemoteTag>
  struct standalone_reporter
    : public reporter<standalone_reporter<RemoteTag>, RemoteTag, tag::intrusive>
  {
    using base = reporter<standalone_reporter, RemoteTag, tag::intrusive>;
  public:
    using base::base;
  };

  template <typename RemoteTag>
  struct standalone_tracker
    : public tracker<standalone_tracker<RemoteTag>, RemoteTag, tag::intrusive>
  {
    using base = tracker<standalone_tracker, RemoteTag, tag::intrusive>;
  public:
    using base::base;
  };

  template <typename Parent, typename Remote = Parent>
  using multireporter = tracker<Parent,
                                remote::tracker<Remote, tag::nonintrusive>,
                                tag::nonintrusive>;

  template <typename Derived, typename RemoteTag>
  using intrusive_reporter = reporter<Derived, RemoteTag, tag::intrusive>;

  template <typename Derived, typename RemoteTag>
  using intrusive_tracker = tracker<Derived, RemoteTag, tag::intrusive>;

  struct tag::intrusive
  {
    template <typename Derived>
    using reporter = remote::reporter<Derived, intrusive>;

    template <typename Derived>
    using tracker = remote::tracker<Derived, intrusive>;
  };

  struct tag::nonintrusive
  {
    template <typename Derived>
    using reporter = remote::reporter<Derived, nonintrusive>;

    template <typename Derived>
    using tracker = remote::tracker<Derived, nonintrusive>;
  };

  template <typename ...Ts, typename ...RemoteTypes>
  void bind (tracker<Ts...>& l, RemoteTypes&... Remotes)
  {
    l.bind (Remotes...);
  }

}

#endif