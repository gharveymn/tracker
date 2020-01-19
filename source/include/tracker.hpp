/** tracker.h
 * Track variable lifetimes automatically.
 *
 * Copyright © 2019 Gene Harvey
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

//! standalone
#if ! defined (tracker_hpp)
#define tracker_hpp 1

#include <algorithm>
#include <plf_list.h>
#include <iterator>
#include <optional_ref.hpp>

#if __has_cpp_attribute(nodiscard)
#  define GCH_NODISCARD [[nodiscard]]
#else
#  define GCH_NODISCARD
#endif

#if __cpp_inline_variables >= 201606
#  define GCH_INLINE_VARS inline
#else
#  define GCH_INLINE_VARS
#endif

#if __cpp_constexpr >= 201603L
#  define GCH_CPP17_CONSTEXPR constexpr
#else
#  define GCH_CPP17_CONSTEXPR
#endif

#if __cpp_constexpr >= 201304L
#  define GCH_CPP14_CONSTEXPR constexpr
#else
#  define GCH_CPP14_CONSTEXPR
#endif

namespace gch
{

  namespace tag
  {
    struct intrusive;
    struct nonintrusive;
    struct standalone;

    // for symmetric constructtors
    GCH_INLINE_VARS constexpr struct bind_t  { bind_t  (void) = default; } bind;
    GCH_INLINE_VARS constexpr struct track_t { track_t (void) = default; } track;
  }

  namespace detail
  {
    template <typename LocalBaseTag, typename RemoteBaseTag>
    class reporter_base;

    template <typename RemoteBaseTag>
    class tracker_base;

    namespace tag
    {
      struct reporter_base
      {
        reporter_base            (void)                           = delete;
        reporter_base            (const reporter_base&)           = delete;
        reporter_base            (reporter_base&& other) noexcept = delete;
        reporter_base& operator= (const reporter_base&)           = delete;
        reporter_base& operator= (reporter_base&& other) noexcept = delete;
        ~reporter_base           (void)                           = delete;

        using base = tag::reporter_base;

        template <typename LocalBaseTag, typename RemoteBaseTag>
        using type = detail::reporter_base<LocalBaseTag, RemoteBaseTag>;
      };

      struct tracker_base
      {
        tracker_base            (void)                          = delete;
        tracker_base            (const tracker_base&)           = delete;
        tracker_base            (tracker_base&& other) noexcept = delete;
        tracker_base& operator= (const tracker_base&)           = delete;
        tracker_base& operator= (tracker_base&& other) noexcept = delete;
        ~tracker_base           (void)                          = delete;

        using base = tag::tracker_base;

        template <typename LocalBaseTag, typename RemoteBaseTag>
        using type = detail::tracker_base<RemoteBaseTag>;
      };

      // for asymmetric constructors
      GCH_INLINE_VARS constexpr struct track_t { track_t (void) = default; } track;
    }
  }

  template <typename Local        = tag::standalone,
            typename RemoteTag    = tag::nonintrusive,
            typename IntrusiveTag = tag::nonintrusive>
  class reporter;

  template <typename Local        = tag::standalone,
            typename RemoteTag    = tag::nonintrusive,
            typename IntrusiveTag = tag::nonintrusive>
  class tracker;

  // acts as a tag
  template <>
  class reporter<>
    : public detail::tag::reporter_base
  { };

  // acts as a tag
  template <>
  class tracker<>
    : public detail::tag::tracker_base
  { };

  // acts as a tag
  template <typename Parent>
  class reporter<Parent, tag::intrusive>
    : public detail::tag::reporter_base
  { };

  // acts as a tag
  template <typename Parent>
  class tracker<Parent, tag::intrusive>
    : public detail::tag::tracker_base
  { };

  // acts as a tag
  template <typename Parent>
  class reporter<Parent, tag::nonintrusive>
    : public detail::tag::reporter_base
  { };

  // acts as a tag
  template <typename Parent>
  class tracker<Parent, tag::nonintrusive>
    : public detail::tag::tracker_base
  { };

  namespace detail::tag
  {
    template <typename Tag>
    struct base_tag;

    template <typename ...Ts>
    struct base_tag<reporter<Ts...>>
    {
      using type = tag::reporter_base;
    };

    template <typename ...Ts>
    struct base_tag<tracker<Ts...>>
    {
      using type = tag::tracker_base;
    };

    template <typename Tag>
    using base_tag_t = typename base_tag<Tag>::type;

  }

  namespace tag
  {
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
  }

  namespace detail
  {
    namespace tag
    {
      using gch::tag::is_reporter;
      using gch::tag::is_tracker;
      using gch::tag::enable_if_reporter_t;
      using gch::tag::enable_if_tracker_t;
    }

    template <typename ...Ts>
    using list = plf::list<Ts...>;

    template <typename LocalBaseTag, typename RemoteBaseTag>
    class reporter_base;

    template <typename RemoteBaseTag>
    class tracker_base;

    template <typename Derived, typename RemoteBase>
    class reporter_base_common
    {
    public:
      using derived_type     = Derived;
      using remote_base_type = RemoteBase;

      reporter_base_common            (void)                                  = default;
      reporter_base_common            (const reporter_base_common&)           = default;
      reporter_base_common            (reporter_base_common&& other) noexcept = default;
      reporter_base_common& operator= (const reporter_base_common&)           = default;
      reporter_base_common& operator= (reporter_base_common&& other) noexcept = default;
      ~reporter_base_common           (void)                                  = default;

      constexpr reporter_base_common (tag::track_t, remote_base_type& remote) noexcept
        : m_remote_base (remote)
      { }

      void swap (reporter_base_common& other) noexcept
      {
        using std::swap;
        swap (this->m_remote_base, other.m_remote_base);
      }

      GCH_NODISCARD
      constexpr bool is_tracked (void) const noexcept
      {
        return m_remote_base.has_value ();
      }

      GCH_NODISCARD
      constexpr bool is_tracking (remote_base_type& remote_cmp) const noexcept
      {
        return m_remote_base.contains (remote_cmp);
      }

      void track (remote_base_type& remote) noexcept
      {
        m_remote_base.emplace (remote);
      }

      GCH_NODISCARD
      constexpr bool has_remote_base (void) const noexcept
      {
        return is_tracked ();
      }

      GCH_NODISCARD
      GCH_CPP14_CONSTEXPR remote_base_type& get_remote_base (void) noexcept
      {
        return *m_remote_base;
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
        m_remote_base.reset ();
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

      gch::optional_ref<remote_base_type> m_remote_base;

    };

    template <typename LocalBaseTag>
    class reporter_base<LocalBaseTag, tag::reporter_base>
      : public reporter_base_common<reporter_base<LocalBaseTag, tag::reporter_base>,
                                    reporter_base<tag::reporter_base, LocalBaseTag>>
    {
    public:

      using local_base_tag  = LocalBaseTag;
      using remote_base_tag = tag::reporter_base;
      using remote_base_type = reporter_base<remote_base_tag, local_base_tag>;
      using remote_reporter_type = remote_base_type;

      using base = reporter_base_common<reporter_base, remote_base_type>;

      using base::base;
      reporter_base            (void)                           = default;
      reporter_base            (const reporter_base&)           = default;
      reporter_base            (reporter_base&& other) noexcept = default;
      reporter_base& operator= (const reporter_base&)           = default;
      reporter_base& operator= (reporter_base&& other) noexcept = default;
      ~reporter_base           (void)                           = default;

      reporter_base (gch::tag::bind_t, remote_base_type& remote) = delete;

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
        }
        return *this;
      }

      GCH_NODISCARD
      std::size_t get_position (void) const noexcept
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

    };

    template <typename LocalBaseTag>
    class reporter_base<LocalBaseTag, tag::tracker_base>
      : public reporter_base_common<reporter_base<LocalBaseTag, tag::tracker_base>,
                                    tracker_base<LocalBaseTag>>
    {
    public:

    using local_base_tag       = LocalBaseTag;
    using remote_base_tag      = tag::tracker_base;
    using remote_base_type     = tracker_base<local_base_tag>;
    using remote_reporter_type = reporter_base<remote_base_tag, local_base_tag>;

    using base = reporter_base_common<reporter_base, remote_base_type>;

    private:

      using self_iter  = typename list<remote_reporter_type>::iterator;
      using self_citer = typename list<remote_reporter_type>::const_iterator;

    public:

      reporter_base            (void)                           = default;
      reporter_base            (const reporter_base&)           = default;
      reporter_base            (reporter_base&& other) noexcept = default;
      reporter_base& operator= (const reporter_base&)           = default;
      reporter_base& operator= (reporter_base&& other) noexcept = default;
      ~reporter_base           (void)                           = default;

      // Note that the destructor here does NOT perform a debinding. This is
      // for optimizations higher in the hierarchy.

      reporter_base (gch::tag::bind_t, remote_base_type& remote)
        : base (tag::track, remote),
          m_self_iter (remote.track (*this))
      { }

      constexpr reporter_base (remote_base_type& remote, self_iter it) noexcept
        : base        (tag::track, remote),
          m_self_iter (it)
      { }

      // remote asymmetric debind
      GCH_CPP14_CONSTEXPR void reset_remote_tracking (void) const noexcept
      {
        if (base::is_tracked ())
          base::get_remote_base ().rptrs_erase (m_self_iter);
      }

      GCH_NODISCARD
      std::size_t get_position (void) const noexcept
      {
        if (! base::is_tracked ())
          return 0;
        return base::get_remote_base ().base_get_offset (m_self_iter);
      }

      reporter_base& rebind (remote_base_type& new_remote)
      {
        // if the remotes are the same then this is already in the remote,
        // so we shouldn't do anything unless we aren't being tracked right now.
        if (! base::is_tracking (new_remote))
          {
            // might fail
            self_iter new_iter = new_remote.track (*this);

            // if we didn't fail the rest is noexcept
            reset_remote_tracking ();
            set (new_remote, new_iter);
          }
        else if (! base::is_tracked ())
          {
            // we already point to new_remote, but we aren't tracked
            // note that that the above condition implies that has_remote () == true
            // since &new_remote cannot be nullptr.
            m_self_iter = new_remote.track (*this);
          }
        return *this;
      }

      // WARNING! This is a pure swap. Move to private when ready.
      void swap (reporter_base& other) noexcept
      {
        base::swap (other);

        using std::swap;
        swap (this->m_self_iter, other.m_self_iter);
      }

      GCH_NODISCARD
      constexpr remote_reporter_type&
      get_remote_reporter (void) const noexcept
      {
        return *m_self_iter;
      }

      GCH_NODISCARD
      constexpr const remote_reporter_type&
      get_const_remote_reporter (void) const noexcept
      {
        return *m_self_iter;
      }

  //  protected:

      reporter_base& set (remote_base_type& remote, self_iter it) noexcept
      {
        base::track (remote);
        m_self_iter = it;
        return *this;
      }

      [[nodiscard]]
      constexpr self_iter get_iter (void) const noexcept
      {
        return m_self_iter;
      }

    private:

      self_iter m_self_iter;

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
      using reporter_list   = list<reporter_type>;
      using rptr_iter       = typename reporter_list::iterator;
      using rptr_citer      = typename reporter_list::const_iterator;
      using rptr_riter      = typename reporter_list::reverse_iterator;
      using rptr_criter     = typename reporter_list::const_reverse_iterator;
      using rptr_ref        = typename reporter_list::reference;
      using rptr_cref       = typename reporter_list::const_reference;

      using local_reporter_type  = reporter_type;

      using remote_base_type     = typename reporter_type::remote_base_type;
      using remote_reporter_type = typename reporter_type::remote_reporter_type;
      using remote_reporter_list = list<remote_reporter_type>;
      using remote_rptr_iter     = typename remote_reporter_list::iterator;

    public:

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
      std::size_t num_reporters (void) const noexcept
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
      typename std::iterator_traits<rptr_citer>::difference_type
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

    private:

      reporter_list m_rptrs;

    };

    template <>
    class tracker_base<tag::reporter_base>
      : public tracker_base_common<tag::reporter_base>
    {
    public:

      tracker_base            (void)                    = default;
      tracker_base            (const tracker_base&)     = delete;
      tracker_base            (tracker_base&&) noexcept = default;
      tracker_base& operator= (const tracker_base&)     = delete;
      tracker_base& operator= (tracker_base&&) noexcept = default;
      ~tracker_base           (void)                    = default;

      template <typename ...Args>
      rptr_iter base_bind_back (Args&&...) = delete;

    };

    //! Remote is one of the non-base classes defined here
    template <>
    class tracker_base<tag::tracker_base>
      : public tracker_base_common<tag::tracker_base>
    {
    public:

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

    namespace tag
    {
      template <typename Interface>
      struct create_tag;

      template <template <typename ...> class TType,
                typename Parent, typename RemoteType, typename IntrusiveTag>
      struct create_tag<TType<Parent, RemoteType, IntrusiveTag>>
      {
        using type = TType<Parent, IntrusiveTag>;
      };

      template <template <typename ...> class TType, typename RemoteTag>
      struct create_tag<TType<TType<gch::tag::standalone, RemoteTag>,
                        RemoteTag, gch::tag::intrusive>>
      {
        using type = TType<>;
      };

      template <typename Interface>
      using create_tag_t = typename create_tag<Interface>::type;

      template <typename LocalTag, typename RemoteTag, typename = void>
      struct remote_parent;

      template <typename LocalTag, template <typename ...> class TRemoteTag,
                typename Parent, typename IntrusiveTag, typename Discard>
      struct remote_parent<LocalTag, TRemoteTag<Parent, IntrusiveTag, Discard>,
                           typename std::enable_if<
                             ! std::is_same<Parent, gch::tag::standalone>::value>::type>
      {
        using type = Parent;
      };

      template <typename LocalTag, template <typename ...> class TRemoteTag>
      struct remote_parent<LocalTag, TRemoteTag<gch::tag::standalone>>
      {
        using type = TRemoteTag<gch::tag::standalone, LocalTag>;
      };

      template <typename LocalTag, typename RemoteTag>
      using remote_parent_t = typename remote_parent<LocalTag, RemoteTag>::type;

      template <typename LocalTag, typename RemoteTag, typename = void>
      struct remote_interface;

      template <typename LocalTag, template <typename ...> class TRemoteTag,
                typename Parent, typename IntrusiveTag, typename Discard>
      struct remote_interface<LocalTag, TRemoteTag<Parent, IntrusiveTag, Discard>,
                              typename std::enable_if<
                                ! std::is_same<Parent, gch::tag::standalone>::value>::type>
      {
        using type = TRemoteTag<Parent, LocalTag, IntrusiveTag>;
      };

      template <typename LocalTag, template <typename ...> class TRemoteTag>
      struct remote_interface<LocalTag, TRemoteTag<gch::tag::standalone>>
      {
        using type = TRemoteTag<TRemoteTag<gch::tag::standalone, LocalTag>,
                                LocalTag,
                                gch::tag::intrusive>;
      };

      template <typename LocalTag, typename RemoteTag>
      using remote_interface_t = typename remote_interface<LocalTag, RemoteTag>::type;

      template <typename Interface>
      struct common;

      template <typename ...Ts>
      struct common<reporter<Ts...>>
      {
        using type = reporter_common<reporter<Ts...>>;
      };

      template <typename ...Ts>
      struct common<tracker<Ts...>>
      {
        using type = tracker_common<tracker<Ts...>>;
      };

      template <typename Interface>
      using common_t = typename common<Interface>::type;

    } // tag

    template <typename Interface>
    struct interface_traits;

    template <template <typename ...> class TInterface,
              typename Parent, typename RemoteTag, typename IntrusiveTag>
    struct interface_traits<TInterface<Parent, RemoteTag, IntrusiveTag>>
    {
      using local_interface_type  = TInterface<Parent, RemoteTag, IntrusiveTag>;

      using local_tag             = tag::create_tag_t<local_interface_type>;
      using remote_tag            = RemoteTag;

      using local_type            = Parent;
      using remote_type           = tag::remote_parent_t<local_tag, remote_tag>;

      using local_base_tag        = typename local_tag::base;
      using remote_base_tag       = typename remote_tag::base;

      using remote_interface_type = tag::remote_interface_t<local_tag, remote_tag>;
    };

    /////////////////////
    // reporter_common //
    /////////////////////

    template <typename Parent, typename RemoteTag, typename IntrusiveTag>
    class reporter_common<reporter<Parent, RemoteTag, IntrusiveTag>>
      : private reporter_base<tag::reporter_base, typename RemoteTag::base>
    {
    public:

      using base = reporter_base<tag::reporter_base, typename RemoteTag::base>;

      using local_interface_type = reporter<Parent, RemoteTag, IntrusiveTag>;

      using local_tag             = tag::create_tag_t<local_interface_type>;
      using remote_tag            = RemoteTag;

      using local_type            = Parent;
      using remote_type           = tag::remote_parent_t<local_tag, remote_tag>;

      using local_base_tag        = typename local_tag::base;
      using remote_base_tag       = typename remote_tag::base;

      using remote_interface_type = tag::remote_interface_t<local_tag, remote_tag>;

      using remote_base_type      = typename base::remote_base_type;

//      friend remote_interface_type;

      using  remote_common_type   = tag::common_t<remote_interface_type>;

      friend remote_common_type;

    private:

      using local_reporter_type  = base;
      using remote_reporter_type = typename base::remote_reporter_type;

    public:

      using base::debind;
      using base::get_position;
      using base::is_tracked;
      using base::is_tracking;
      using base::reset;
      using base::wipe;

      reporter_common            (void)                       = default;
      reporter_common            (const reporter_common&)     = delete;
//    reporter_common            (reporter_common&&) noexcept = impl;
      reporter_common& operator= (const reporter_common&)     = delete;
//    reporter_common& operator= (reporter_common&&) noexcept = impl;
//    ~reporter_common           (void)                       = impl;

      static_assert (std::is_trivially_copyable<base>::value, "");

      reporter_common (reporter_common&& other) noexcept
        : base (other) // the base is trivially copyable
      {
        if (is_tracked ())
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
        if (is_tracked ())
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

      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      reporter_common (gch::tag::bind_t, remote_interface_type& remote) = delete;

      //! enabled for trackers
      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      constexpr reporter_common (gch::tag::bind_t, remote_interface_type& remote) noexcept
        : base (gch::tag::bind, remote)
      { }

      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      reporter_common clone (void) const = delete;

      //! enabled for trackers
      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      reporter_common clone (void) const
      {
        if (is_tracked ())
          return reporter_common (gch::tag::bind, get_remote_interface ());
        return { };
      }

      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      local_interface_type& replace_binding (const local_interface_type& other) = delete;

      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      local_interface_type& replace_binding (const local_interface_type& other)
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
      constexpr remote_type& get_remote (void) const noexcept
      {
        return get_remote_interface ().get_parent ();
      }

      GCH_NODISCARD
      constexpr optional_ref<remote_type> get_maybe_remote (void) const noexcept
      {
        return has_remote () ? get_remote () : optional_ref<remote_type> (nullopt);
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

      template <typename ...Args>
      remote_interface_type create_interface (Args&&... args)
      {
        remote_interface_type ret (std::forward<Args> (args)...);
        base tmp = ret.exchange_reporter ({ *this });
        base::reset_remote_tracking ();
        base::operator= (tmp);
        return ret;
      }

    private:

      remote_reporter_type exchange_reporter (local_reporter_type rep)
      {
        base::operator= (rep);
        return { tag::track, *this };
      }

    }; // reporter_common

    ////////////////////
    // tracker_common //
    ////////////////////

    template <typename Parent, typename RemoteTag, typename IntrusiveTag>
    class tracker_common<tracker<Parent, RemoteTag, IntrusiveTag>>
      : private tracker_base<typename RemoteTag::base>
    {
    public:

      using base                  = tracker_base<typename RemoteTag::base>;

      using local_interface_type  = tracker<Parent, RemoteTag, IntrusiveTag>;

      using local_tag             = tag::create_tag_t<local_interface_type>;
      using remote_tag            = RemoteTag;

      using local_type            = Parent;
      using remote_type           = tag::remote_parent_t<local_tag, remote_tag>;

      using local_base_tag        = typename local_tag::base;
      using remote_base_tag       = typename remote_tag::base;

      using remote_interface_type = tag::remote_interface_t<local_tag, remote_tag>;

      //      friend remote_interface_type;

      using remote_common_type    = tag::common_t<remote_interface_type>;

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

        iter_common (void)                                = default;
        iter_common (const iter_common&)                  = default;
        iter_common (iter_common&&) noexcept              = default;
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

        reporter_iter get_iterator (void) const noexcept
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

        using base::base;

        citer            (void)               = default;
        citer            (const citer&)       = default;
        citer            (citer&&) noexcept   = default;
        citer& operator= (const citer&) &     = default;
        citer& operator= (citer&&) & noexcept = default;
        ~citer           (void)               = default;

        constexpr /* implicit */ citer (iter it) noexcept
          : base (it.get_iterator ())
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

      // using iter   = remote_iterator<rptr_iter, remote_interface_type, remote_type>;
      // using citer  = remote_iterator<rptr_citer, remote_interface_type, const remote_type>;
      using riter  = std::reverse_iterator<iter>;
      using criter = std::reverse_iterator<citer>;
      using ref    = remote_type&;
      using cref   = const remote_type&;

      using init_list = std::initializer_list<std::reference_wrapper<remote_interface_type>>;

      using base::base;
      using base::clear;
      using base::wipe;
      using base::num_reporters;

      tracker_common            (void)                      = default;
      tracker_common            (const tracker_common&)     = default;
      tracker_common            (tracker_common&&) noexcept = default;
      tracker_common& operator= (const tracker_common&)     = default;
      tracker_common& operator= (tracker_common&&) noexcept = default;
//    ~tracker_common           (void)                      = impl;

      //! safe by default
      ~tracker_common (void)
      {
        base::clear ();
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

      GCH_NODISCARD bool   has_reporters (void) const noexcept { return ! base::rptrs_empty (); }

      GCH_CPP17_CONSTEXPR std::size_t get_offset (citer pos) const noexcept
      {
        return base::base_get_offset (pos);
      }

      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      tracker_common (citer first, citer last) = delete;

      //! enabled for trackers
      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      tracker_common (citer first, citer last)
        : base (first.get_iterator (), last.get_iterator ())
      { }

      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      tracker_common (gch::tag::bind_t, init_list init) = delete;

      //! enabled for trackers
      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      tracker_common (gch::tag::bind_t, init_list init)
      {
        if (init.size () == 0)
          return;

        const rptr_iter pivot = base_bind_back (*init.begin ());
        try
        {
          std::for_each (++init.begin (), init.end (),
                         [this] (remote_interface_type& remote)
                         {
                           base_bind_back (remote);
                         });
        }
        catch (...)
        {
          base_debind (pivot, init.end ());
          throw;
        }
      }

      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      local_interface_type& replace_bindings (const local_interface_type& other) = delete;

      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      local_interface_type& replace_bindings (const local_interface_type& other)
      {
        if (&other != this)
        {
          const rptr_iter pivot = base::base_bind_back (other.rptrs_cbegin (), other.rptrs_cend ());
          base::base_debind (base::rptrs_cbegin (), pivot);
        }
        return static_cast<local_interface_type&> (*this);
      }

      local_interface_type& replace_bindings (local_interface_type&& other)
      {
        return static_cast<local_interface_type&> (base::operator= (std::move (other)));
      }

      //! transfers all bindings from `other`; no overwriting
      GCH_CPP14_CONSTEXPR iter
      transfer_bindings (local_interface_type& other, citer after) noexcept
      {
        return base::base_transfer_bindings (other, after.get_iterator ());
      }

      GCH_CPP14_CONSTEXPR iter
      transfer_bindings (local_interface_type& other) noexcept
      {
        return transfer_bindings (other, cend ());
      }

      template <typename ...Args>
      GCH_CPP14_CONSTEXPR iter
      transfer_bindings (local_interface_type&& other, Args&&... args) noexcept
      {
        return transfer_bindings (other, std::forward<Args> (args)...);
      }

      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      iter copy_bindings (const local_interface_type& other) = delete;

      //! enabled for trackers
      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      iter copy_bindings (const local_interface_type& other)
      {
        return bind (other.begin (), other.end ());
      }

      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      tracker_common clone (void) const = delete;

      //! enabled for trackers
      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      tracker_common clone (void) const
      {
        return tracker_common (begin (), end ());
      }

      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      iter bind (citer first, citer last) = delete;

      //! enabled for trackers
      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      iter bind (citer first, citer last)
      {
        return base::base_bind_back (rptr_citer (first), rptr_citer (last));
      }

      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      iter bind (remote_interface_type& r) = delete;

      //! enabled for trackers
      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      iter bind (remote_interface_type& r)
      {
        return base::base_bind_back (r);
      }

      //! disabled for reporters
      template <typename ...Args,
                typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      iter bind (remote_interface_type& r, Args&... args) = delete;

      //! enabled for trackers
      template <typename ...Args,
                typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      iter bind (remote_interface_type& r, Args&... args)
      {
        bind (args...);
        return bind (r);
      }

      template <typename ...Args>
      remote_interface_type create_interface (Args&&... args)
      {
        remote_interface_type ret (std::forward<Args> (args)...);
        rptr_iter local_it = base::rptrs_emplace (base::rptrs_end ());
        try
        {
          *local_it = ret.exchange_reporter ({ *this, local_it });
        }
        catch (...)
        {
          base::rptrs_erase (local_it);
          throw;
        }
        return ret;
      }

    private:

      remote_reporter_type exchange_reporter (local_reporter_type rep)
      {
        return { *this, rptrs_emplace (base::rptrs_end (), rep) };
      }

    }; // tracker_common

    //////////////////////
    // intrusive_common //
    //////////////////////

    template <typename Derived>
    class intrusive_common
    {

      using derived_type = Derived;

    public:

      intrusive_common            (void)                        = default;
      intrusive_common            (const intrusive_common&)     = default;
      intrusive_common            (intrusive_common&&) noexcept = default;
      intrusive_common& operator= (const intrusive_common&)     = default;
      intrusive_common& operator= (intrusive_common&&) noexcept = default;
      ~intrusive_common           (void)                        = default;

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

    }; // intrusive_common

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

      constexpr nonintrusive_common (parent_type& parent)
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
    : public detail::reporter_common<reporter<Derived, RemoteTag, tag::intrusive>>,
      public detail::intrusive_common<Derived>
  {
  public:

    using base = detail::reporter_common<reporter<Derived, RemoteTag, tag::intrusive>>;
    using access_base = detail::intrusive_common<Derived>;

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
    reporter            (const reporter&)     = default;
    reporter            (reporter&&) noexcept = default;
    reporter& operator= (const reporter&)     = default;
    reporter& operator= (reporter&&) noexcept = default;
    ~reporter           (void)                = default;

    reporter (base&& other) noexcept
      : base (std::move (other))
    { }

  }; // reporter

  /////////////////////////
  // tracker (intrusive) //
  /////////////////////////

  template <typename Derived, typename RemoteTag>
  class tracker<Derived, RemoteTag, tag::intrusive>
    : public detail::tracker_common<tracker<Derived, RemoteTag, tag::intrusive>>,
      public detail::intrusive_common<Derived>
  {
    using base = detail::tracker_common<tracker<Derived, RemoteTag, tag::intrusive>>;
    using access_base = detail::intrusive_common<Derived>;
  public:

    using local_tag             = tracker<Derived, tag::intrusive>;
    using remote_tag            = RemoteTag;

    using derived_type          = Derived;
    using remote_type           = typename base::remote_type;

    using local_interface_type  = tracker;
    using remote_interface_type = typename base::remote_interface_type;

    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;

    using base::base;
    tracker            (void)               = default;
    tracker            (const tracker&)     = default;
    tracker            (tracker&&) noexcept = default;
    tracker& operator= (const tracker&)     = default;
    tracker& operator= (tracker&&) noexcept = default;
    ~tracker           (void)               = default;

    tracker (base&& other) noexcept
      : base (std::move (other))
    { }

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
    reporter            (const reporter&)     = default;
    reporter            (reporter&&) noexcept = default;
    reporter& operator= (const reporter&)     = default;
    reporter& operator= (reporter&&) noexcept = default;
    ~reporter           (void)                = default;

    void swap (reporter& other)
    {
      base::swap (other);
    }

    explicit reporter (parent_type& parent)
      : access_base (parent)
    { }

    reporter (base&& other, parent_type& parent) noexcept
      : base        (std::move (other)),
        access_base (parent)
    { }

    template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
    reporter (parent_type& parent, remote_interface_type& remote) = delete;

    template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
    reporter (parent_type& parent, remote_interface_type& remote)
      : base        (tag::bind, remote),
        access_base (parent)
    { }

//    template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
//    reporter (const reporter& other, parent_type& parent) = delete;

    //! enabled for trackers
//    template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
//    reporter (const reporter& other, parent_type& parent)
//      : base        (other),
//        access_base (parent)
//    { }

    reporter (reporter&& other, parent_type& parent) noexcept
      : base        (std::move (other)),
        access_base (parent)
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

    using local_tag             = tracker<Parent, tag::nonintrusive>;
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
    tracker            (const tracker&)     = default;
    tracker            (tracker&&) noexcept = default;
    tracker& operator= (const tracker&)     = default;
    tracker& operator= (tracker&&) noexcept = default;
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

    template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
    tracker (parent_type& parent, init_list init) = delete;

    template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
    tracker (parent_type& parent, init_list init)
      : base        (tag::bind, init),
        access_base (parent)
    { }

  }; // tracker

  template <typename RemoteTag>
  class reporter<tag::standalone, RemoteTag>
    : public reporter<reporter<tag::standalone, RemoteTag>, RemoteTag, tag::intrusive>
  {
    using base = reporter<reporter, RemoteTag, tag::intrusive>;
  public:
    using base::base;
  };

  template <typename RemoteTag>
  class tracker<tag::standalone, RemoteTag>
      : public tracker<tracker<tag::standalone, RemoteTag>, RemoteTag, tag::intrusive>
  {
    using base = tracker<tracker, RemoteTag, tag::intrusive>;
  public:
    using base::base;
  };

  template <typename RemoteTag>
  using standalone_reporter = reporter<tag::standalone, RemoteTag>;

  template <typename RemoteTag>
  using standalone_tracker = tracker<tag::standalone, RemoteTag>;

  template <typename Parent, typename Remote = Parent>
  using multireporter = tracker<Parent,
                                tracker<Remote, tag::nonintrusive>,
                                tag::nonintrusive>;

  template <typename Derived, typename RemoteTag>
  using intrusive_reporter = reporter<Derived, RemoteTag, tag::intrusive>;

  template <typename Derived, typename RemoteTag>
  using intrusive_tracker = reporter<Derived, RemoteTag, tag::intrusive>;

  struct tag::intrusive
  {
    template <typename Derived>
    using reporter = reporter<Derived, intrusive>;

    template <typename Derived>
    using tracker = tracker<Derived, intrusive>;
  };

  struct tag::nonintrusive
  {
    template <typename Derived>
    using reporter = reporter<Derived, nonintrusive>;

    template <typename Derived>
    using tracker = tracker<Derived, nonintrusive>;
  };

  template <typename ...Ts, typename ...RemoteTypes>
  void bind (tracker<Ts...>& l, RemoteTypes&... Remotes)
  {
    l.bind (Remotes...);
  }

}

#undef GCH_NODISCARD

#endif
