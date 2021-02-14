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

#include <plf_list.h>

#include <algorithm>
#include <iterator>

#ifdef __cpp_constexpr
#  ifndef GCH_CPP14_CONSTEXPR
#    if __cpp_constexpr >= 201304L
#      define GCH_CPP14_CONSTEXPR constexpr
#      ifndef GCH_HAS_CPP14_CONSTEXPR
#        define GCH_HAS_CPP14_CONSTEXPR
#      endif
#    else
#      define GCH_CPP14_CONSTEXPR
#    endif
#  endif
#  ifndef GCH_CPP17_CONSTEXPR
#    if __cpp_constexpr >= 201603L
#      define GCH_CPP17_CONSTEXPR constexpr
#      ifndef GCH_HAS_CPP17_CONSTEXPR
#        define GCH_HAS_CPP17_CONSTEXPR
#      endif
#    else
#      define GCH_CPP17_CONSTEXPR
#    endif
#  endif
#  ifndef GCH_CPP20_CONSTEXPR
#    if __cpp_constexpr >= 201907L
#      define GCH_CPP20_CONSTEXPR constexpr
#      ifndef GCH_HAS_CPP20_CONSTEXPR
#        define GCH_HAS_CPP20_CONSTEXPR
#      endif
#    else
#      define GCH_CPP20_CONSTEXPR
#    endif
#  endif
#endif

#ifndef GCH_NODISCARD
#  if defined (__has_cpp_attribute) && __has_cpp_attribute (nodiscard) >= 201603L
#    if ! defined (__clang__) || defined (GCH_CLANG_17)
#      define GCH_NODISCARD [[nodiscard]]
#    else
#      define GCH_NODISCARD
#    endif
#  else
#    define GCH_NODISCARD
#  endif
#endif

#ifndef GCH_INLINE_VAR
#  if defined (__cpp_inline_variables) && __cpp_inline_variables >= 201606
#    define GCH_INLINE_VAR inline
#  else
#    define GCH_INLINE_VAR
#  endif
#endif

#if defined (__cpp_impl_three_way_comparison) && __cpp_impl_three_way_comparison >= 201907L
#  ifndef GCH_IMPL_THREE_WAY_COMPARISON
#    define GCH_IMPL_THREE_WAY_COMPARISON
#  endif
#  if __has_include(<compare>)
#    include <compare>
#    if defined (__cpp_lib_three_way_comparison) && __cpp_lib_three_way_comparison >= 201907L
#      ifndef GCH_LIB_THREE_WAY_COMPARISON
#        define GCH_LIB_THREE_WAY_COMPARISON
#      endif
#    endif
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

    struct bind_t
    {
      static constexpr struct create_tag { } create { };
      constexpr explicit bind_t (create_tag) noexcept { }
    };

    // for symmetric constructors
    GCH_INLINE_VAR constexpr bind_t bind { bind_t::create };

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
        using interface_type = gch::reporter<gch::standalone_reporter<LocalTag>, LocalTag,
                                             gch::tag::intrusive>;

        template <typename LocalTag>
        using common_type = detail::reporter_common<interface_type<LocalTag>>;
      };

      struct standalone_tracker : tracker_base
      {
        using reduced_tag = standalone_tracker;

        template <typename LocalTag>
        using parent_type = gch::standalone_tracker<LocalTag>;

        template <typename LocalTag>
        using interface_type = gch::tracker<gch::standalone_tracker<LocalTag>, LocalTag,
                                            gch::tag::intrusive>;

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
      struct is_reporter<Tag, typename std::enable_if<std::is_base_of<detail::tag::reporter_base,
                                                                      Tag>::value>::type>
        : std::true_type
      { };

      template <typename Tag, typename = void>
      struct is_tracker : std::false_type { };

      template <typename Tag>
      struct is_tracker<Tag, typename std::enable_if<std::is_base_of<detail::tag::tracker_base,
                                                                     Tag>::value>::type>
        : std::true_type
      { };

      template <typename Tag, typename Type = void>
      using enable_if_reporter_t = typename std::enable_if<is_reporter<Tag>::value, Type>::type;

      template <typename Tag, typename Type = void>
      using enable_if_tracker_t = typename std::enable_if<is_tracker<Tag>::value, Type>::type;

      // for asymmetric constructors
      GCH_INLINE_VAR constexpr struct track_t { track_t (void) = default; } track;

    }

  }

  namespace remote
  {

    template <typename Parent>
    using reporter = detail::tag::reporter<Parent, tag::nonintrusive>;

    template <typename Parent>
    using tracker = detail::tag::tracker<Parent, tag::nonintrusive>;

    using standalone_reporter = detail::tag::standalone_reporter;
    using standalone_tracker  = detail::tag::standalone_tracker;

    template <typename Derived>
    using intrusive_reporter = detail::tag::reporter<Derived, tag::intrusive>;

    template <typename Derived>
    using intrusive_tracker = detail::tag::tracker<Derived, tag::intrusive>;

    namespace intrusive
    {

      template <typename Derived>
      using reporter = detail::tag::reporter<Derived, tag::intrusive>;

      template <typename Derived>
      using tracker = detail::tag::tracker<Derived, tag::intrusive>;

    }

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

      constexpr
      reporter_base_common (tag::track_t, remote_base_type& remote) noexcept
        : m_remote_base (&remote)
      { }

      void
      swap (reporter_base_common& other) noexcept
      {
        using std::swap;
        swap (this->m_remote_base, other.m_remote_base);
      }

      void
      track (remote_base_type& remote) noexcept
      {
        m_remote_base = &remote;
      }

      GCH_NODISCARD constexpr
      bool
      is_tracked (void) const noexcept
      {
        return nullptr != m_remote_base;
      }

      GCH_NODISCARD constexpr
      bool
      is_tracking (remote_base_type& remote) const noexcept
      {
        return &remote == m_remote_base;
      }

      GCH_NODISCARD constexpr
      remote_base_type&
      get_remote_base (void) const noexcept
      {
        return *m_remote_base;
      }

      // Explanation: while a reference to the remote base will not
      //              affect *this, we still may want to ensure we
      //              are getting a const remote.
      GCH_NODISCARD constexpr
      const remote_base_type&
      get_const_remote_base (void) const noexcept
      {
        return *m_remote_base;
      }

      GCH_NODISCARD constexpr
      remote_base_ptr
      get_remote_base_ptr (void) const noexcept
      {
        return m_remote_base;
      }

      // local asymmetric debind (unsafe)
      void
      wipe (void) noexcept
      {
        m_remote_base = nullptr;
      }

      // symmetric debind (safe)
      GCH_CPP14_CONSTEXPR
      void
      debind (void) noexcept
      {
        static_cast<derived_type *> (this)->reset_remote_tracking ();
        wipe ();
      }

      GCH_CPP14_CONSTEXPR
      void
      reset (void) noexcept
      {
        debind ();
      }

    private:
      remote_base_ptr m_remote_base { nullptr };
    };

    template <typename Derived, typename RemoteBase>
    bool
    operator== (const reporter_base_common<Derived, RemoteBase>& lhs,
                const reporter_base_common<Derived, RemoteBase>& rhs) noexcept
    {
      return lhs.get_remote_base_ptr () == rhs.get_remote_base_ptr ();
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator!= (const reporter_base_common<Derived, RemoteBase>& lhs,
                const reporter_base_common<Derived, RemoteBase>& rhs) noexcept
    {
      return ! (lhs == rhs);
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator< (const reporter_base_common<Derived, RemoteBase>& lhs,
               const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (
        std::less<RemoteBase *> { } (lhs.get_remote_base_ptr (), rhs.get_remote_base_ptr ())))
    {
      return std::less<RemoteBase *> { } (lhs.get_remote_base_ptr (), rhs.get_remote_base_ptr ());
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator>= (const reporter_base_common<Derived, RemoteBase>& lhs,
                const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (! (lhs < rhs)))
    {
      return ! (lhs < rhs);
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator> (const reporter_base_common<Derived, RemoteBase>& lhs,
               const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (rhs < lhs))
    {
      return rhs < lhs;
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator<= (const reporter_base_common<Derived, RemoteBase>& lhs,
                const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (rhs >= lhs))
    {
      return rhs >= lhs;
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator== (const reporter_base_common<Derived, RemoteBase>& lhs,
                const RemoteBase *rhs) noexcept
    {
      return lhs.get_remote_base_ptr () == rhs;
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator== (const RemoteBase *lhs,
                const reporter_base_common<Derived, RemoteBase>& rhs) noexcept
    {
      return lhs == rhs.get_remote_base_ptr ();
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator!= (const reporter_base_common<Derived, RemoteBase>& lhs,
                const RemoteBase *rhs) noexcept
    {
      return lhs.get_remote_base_ptr () != rhs;
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator!= (const RemoteBase *lhs,
                const reporter_base_common<Derived, RemoteBase>& rhs) noexcept
    {
      return lhs != rhs.get_remote_base_ptr ();
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator< (const reporter_base_common<Derived, RemoteBase>& lhs, const RemoteBase *rhs)
      noexcept (noexcept (std::less<const RemoteBase *> { } (lhs.get_remote_base_ptr (), rhs)))
    {
      return std::less<const RemoteBase *> { } (lhs.get_remote_base_ptr (), rhs);
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator< (const RemoteBase *lhs, const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (std::less<const RemoteBase *> { } (lhs, rhs.get_remote_base_ptr ())))
    {
      return std::less<const RemoteBase *> { } (lhs, rhs.get_remote_base_ptr ());
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator>= (const reporter_base_common<Derived, RemoteBase>& lhs, const RemoteBase *rhs)
      noexcept (noexcept (! (lhs < rhs)))
    {
      return ! (lhs < rhs);
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator>= (const RemoteBase *lhs, const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (! (lhs < rhs)))
    {
      return ! (lhs < rhs);
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator> (const reporter_base_common<Derived, RemoteBase>& lhs, const RemoteBase *rhs)
      noexcept (noexcept (rhs < lhs))
    {
      return rhs < lhs;
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator> (const RemoteBase *lhs, const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (rhs < lhs))
    {
      return rhs < lhs;
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator<= (const reporter_base_common<Derived, RemoteBase>& lhs, const RemoteBase *rhs)
      noexcept (noexcept (rhs >= lhs))
    {
      return rhs >= lhs;
    }

    template <typename Derived, typename RemoteBase>
    bool
    operator<= (const RemoteBase *lhs, const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (rhs >= lhs))
    {
      return rhs >= lhs;
    }

    template <typename Derived, typename RemoteBase>
    void
    swap (reporter_base_common<Derived, RemoteBase>& lhs,
          reporter_base_common<Derived, RemoteBase>& rhs) noexcept
    {
      lhs.swap (rhs);
    }

    // with remote reporter
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
      void
      reset_remote_tracking (void) const noexcept
      {
        if (base::is_tracked ())
          base::get_remote_base ().wipe ();
      }

      reporter_base&
      rebind (remote_base_type& new_remote) noexcept
      {
        if (! base::is_tracking (new_remote))
        {
          reset_remote_tracking ();
          base::track (new_remote);
          new_remote.track (*this);
        }
        return *this;
      }

      GCH_NODISCARD constexpr
      std::size_t
      get_position (void) const noexcept
      {
        return 0;
      }

      GCH_NODISCARD constexpr
      remote_base_type&
      get_remote_reporter (void) const noexcept
      {
        return base::get_remote_base ();
      }

      GCH_NODISCARD constexpr
      const remote_base_type&
      get_const_remote_reporter (void) const noexcept
      {
        return base::get_remote_base ();
      }

      reporter_base&
      reset (remote_base_type& remote) noexcept
      {
        reset_remote_tracking ();
        return set (remote);
      }

      reporter_base&
      set (remote_base_type& remote) noexcept
      {
        base::track (remote);
        return *this;
      }
    };

    // with remote tracker
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
      using base::base;
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
          m_self (remote.track (remote.rptrs_cend (), *this))
      { }

      reporter_base (gch::tag::bind_t, remote_base_type& remote,
                     typename remote_base_type::caccess_type pos)
        : base   (tag::track, remote),
          m_self (remote.track (pos, *this))
      { }

      constexpr
      reporter_base (remote_base_type& remote, remote_access_type it) noexcept
        : base   (tag::track, remote),
          m_self (it)
      { }

      // remote asymmetric debind
      GCH_CPP14_CONSTEXPR
      void
      reset_remote_tracking (void) const noexcept
      {
        if (base::is_tracked ())
          base::get_remote_base ().rptrs_erase (m_self);
      }

      GCH_NODISCARD
      std::size_t
      get_position (void) const noexcept
      {
        if (! base::is_tracked ())
          return 0;
        return base::get_remote_base ().get_reporter_offset (m_self);
      }

      reporter_base&
      rebind (remote_base_type& new_remote)
      {
        // if the remotes are the same then this is already in the remote,
        // so we shouldn't do anything unless we aren't being tracked right now.
        if (! base::is_tracking (new_remote))
        {
          // might throw
          remote_access_type new_iter = new_remote.track (new_remote.rptrs_cend (), *this);

          // if we didn't throw the rest is noexcept
          reset_remote_tracking ();
          base::track (new_remote);
          m_self = new_iter;
        }
        else if (! base::is_tracked ())
        {
          // we already point to new_remote, but we aren't tracked
          // note that that the above condition implies that has_remote () == true
          // since &new_remote cannot be nullptr.
          m_self = new_remote.track (new_remote.rptrs_cend (), *this);
        }
        return *this;
      }

      // WARNING! This is a pure swap. Move to private when ready.
      void
      swap (reporter_base& other) noexcept
      {
        base::swap (other);

        using std::swap;
        swap (this->m_self, other.m_self);
      }

      GCH_NODISCARD constexpr
      remote_reporter_type&
      get_remote_reporter (void) const noexcept
      {
        return *m_self;
      }

      GCH_NODISCARD constexpr
      const remote_reporter_type&
      get_const_remote_reporter (void) const noexcept
      {
        return *m_self;
      }

  //  protected:

      reporter_base&
      reset (remote_base_type& remote, remote_access_type it) noexcept
      {
        reset_remote_tracking ();
        return set (remote, it);
      }

      reporter_base&
      set (remote_base_type& remote, remote_access_type it) noexcept
      {
        base::track (remote);
        m_self = it;
        return *this;
      }

      reporter_base&
      set_access (remote_access_type it) noexcept
      {
        m_self = it;
        return *this;
      }

    private:
      remote_access_type m_self;
    };

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
      friend remote_base_type;

      template <typename It, typename T, typename I>
      friend class tracker_iterator;

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

      static_assert (std::is_trivially_copyable<base>::value, "base was not trivially copyable");

      reporter_common (reporter_common&& other) noexcept
        : base (other) // the base is trivially copyable
      {
        if (has_remote ())
          base::get_remote_reporter ().track (*this);
        other.wipe ();
      }

      reporter_common&
      operator= (reporter_common&& other) noexcept
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
            wipe ();
        }
        return *this;
      }

      ~reporter_common (void)
      {
        if (has_remote ())
          base::reset_remote_tracking ();
      }

      reporter_common (gch::tag::bind_t, remote_interface_type& remote) noexcept
        : base (gch::tag::bind, remote)
      { }

      template <typename It, typename Tag = remote_tag, tag::enable_if_tracker_t<Tag> * = nullptr>
      reporter_common (gch::tag::bind_t, remote_interface_type& remote, It pos) noexcept
        : base (gch::tag::bind, remote, pos)
      { }

      void
      swap (reporter_common& other) noexcept
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

      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag> * = nullptr>
      GCH_NODISCARD
      reporter_common
      clone (void) const
      {
        if (has_remote ())
          return { gch::tag::bind, get_remote_interface () };
        return { };
      }

      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag> * = nullptr>
      void
      clone (void) const = delete;

      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag> * = nullptr>
      local_interface_type&
      replace_binding (const local_interface_type& other)
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

      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag> * = nullptr>
      void
      replace_binding (const local_interface_type& other) = delete;

      local_interface_type&
      replace_binding (local_interface_type&& other)
      {
        return static_cast<local_interface_type&> (operator= (std::move (other)));
      }

      GCH_NODISCARD constexpr
      bool
      has_remote (void) const noexcept
      {
        return base::is_tracked ();
      }

      GCH_NODISCARD constexpr
      bool
      has_remote (remote_interface_type& remote) const noexcept
      {
        return base::is_tracking (remote);
      }

      GCH_NODISCARD constexpr
      bool
      has_remote (remote_type& remote) const noexcept
      {
        return has_remote () && (&remote == &get_remote ());
      }

      GCH_NODISCARD constexpr
      remote_type&
      get_remote (void) const noexcept
      {
        return get_remote_interface ().get_parent ();
      }

      GCH_NODISCARD constexpr
      remote_type *
      get_remote_ptr (void) const noexcept
      {
        return has_remote () ? &get_remote_interface ().get_parent () : nullptr;
      }

      GCH_NODISCARD constexpr
      remote_interface_type&
      get_remote_interface (void) const noexcept
      {
        return static_cast<remote_interface_type&> (base::get_remote_base ());
      }

      local_interface_type&
      rebind (remote_interface_type& new_remote)
      {
        return static_cast<local_interface_type&> (base::rebind (new_remote));
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
      using iterator       = tracker_iterator<rptrs_iter,  remote_type, remote_interface_type>;
      using const_iterator = tracker_iterator<rptrs_citer, remote_type, remote_interface_type>;

      using reverse_iterator        = std::reverse_iterator<iterator>;
      using const_reverse_iterator  = std::reverse_iterator<const_iterator>;
      using value_type              = remote_type;
      using reference               = remote_type&;
      using const_reference         = const remote_type&;
      using size_type               = typename base::rptrs_size_ty;
      using difference_type         = typename base::rptrs_diff_ty;
      using allocator_type          = typename base::rptrs_alloc_t;

      using iter   = iterator;
      using citer  = const_iterator;
      using riter  = reverse_iterator;
      using criter = const_reverse_iterator;
      using ref    = reference;
      using cref   = const_reference;

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
      debind (const remote_type& r)
      {
        auto find_pred = [&r](const remote_type& e) { return &e == &r; };
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

      constexpr explicit
      nonintrusive_common (parent_type& parent)
        : m_parent (parent)
      { }

      GCH_NODISCARD GCH_CPP14_CONSTEXPR
      parent_type&
      get_parent (void) noexcept
      {
        return m_parent;
      }

      GCH_NODISCARD constexpr
      const parent_type&
      get_parent (void) const noexcept
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

    using parent_type           = Derived;
    using remote_type           = typename base::remote_type;

    using local_interface_type  = reporter;
    using remote_interface_type = typename base::remote_interface_type;

    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;

    using derived_type          = parent_type;

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

    using parent_type           = Derived;
    using remote_type           = typename base::remote_type;

    using local_interface_type  = tracker;
    using remote_interface_type = typename base::remote_interface_type;

    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;

    using derived_type          = parent_type;

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

    void
    swap (reporter& other)
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

    explicit
    reporter (parent_type& parent)
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

  public:
    using local_tag             = typename base::local_tag;
    using remote_tag            = RemoteTag;

    using parent_type           = Parent;
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
             std::initializer_list<std::reference_wrapper<remote_interface_type>> init) = delete;

    template <typename Tag = remote_tag,
              detail::tag::enable_if_tracker_t<Tag> * = nullptr>
    tracker (parent_type& parent,
             std::initializer_list<std::reference_wrapper<remote_interface_type>> init)
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
  using multireporter = tracker<Parent, remote::tracker<Remote>>;

  template <typename Derived, typename RemoteTag>
  using intrusive_reporter = reporter<Derived, RemoteTag, tag::intrusive>;

  template <typename Derived, typename RemoteTag>
  using intrusive_tracker = tracker<Derived, RemoteTag, tag::intrusive>;

  namespace intrusive
  {

    template <typename Derived, typename RemoteTag>
    using reporter = gch::reporter<Derived, RemoteTag, tag::intrusive>;

    template <typename Derived, typename RemoteTag>
    using tracker = gch::tracker<Derived, RemoteTag, tag::intrusive>;

  }

  template <typename ...Ts, typename ...RemoteTypes>
  void
  bind (tracker<Ts...>& l, RemoteTypes&... remotes)
  {
    l.bind (remotes...);
  }

}

#endif
