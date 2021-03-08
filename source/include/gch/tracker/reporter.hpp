/** reporter.hpp
 * Copyright © 2021 Gene Harvey
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef OCTAVE_IR_REPORTER_HPP
#define OCTAVE_IR_REPORTER_HPP

#include "detail/common.hpp"

namespace gch
{

  template <typename LocalBaseTag, typename RemoteBaseTag>
  struct tracker_traits<detail::reporter_base<LocalBaseTag, RemoteBaseTag>>
  {
    using local_base_tag  = LocalBaseTag;
    using remote_base_tag = RemoteBaseTag;

    using local_reporter_type  = typename local_base_tag::template reporter_type<local_base_tag,
                                                                                 remote_base_tag>;
    using remote_reporter_type = typename remote_base_tag::template reporter_type<remote_base_tag,
                                                                                  local_base_tag>;

    using local_access_type  = typename local_base_tag::template access_type<remote_base_tag>;
    using remote_access_type = typename remote_base_tag::template access_type<local_base_tag>;

    using local_const_access_type  = typename local_base_tag::template
                                     const_access_type<remote_base_tag>;
    using remote_const_access_type = typename remote_base_tag::template
                                       const_access_type<local_base_tag>;

    using local_base_type  = typename local_base_tag::template base_type<local_base_tag,
                                                                         remote_base_tag>;
    using remote_base_type = typename remote_base_tag::template base_type<remote_base_tag,
                                                                          local_base_tag>;
  };

  template <typename Parent, typename RemoteTag, typename ...Ts>
  struct tracker_traits<reporter<Parent, RemoteTag, Ts...>>
  {
    using local_tag  = typename detail::tag::reporter<Parent, Ts...>::reduced_tag;
    using remote_tag = RemoteTag;

    using local_base_tag  = typename local_tag::base_tag;
    using remote_base_tag = typename remote_tag::base_tag;

    using local_parent_type  = Parent;
    using remote_parent_type = typename remote_tag::template parent_type<local_tag>;

    using local_interface_type  = reporter<Parent, RemoteTag, Ts...>;
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
    constexpr
    bool
    operator== (const reporter_base_common<Derived, RemoteBase>& lhs,
                const reporter_base_common<Derived, RemoteBase>& rhs) noexcept
    {
      return lhs.get_remote_base_ptr () == rhs.get_remote_base_ptr ();
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator!= (const reporter_base_common<Derived, RemoteBase>& lhs,
                const reporter_base_common<Derived, RemoteBase>& rhs) noexcept
    {
      return ! (lhs == rhs);
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator< (const reporter_base_common<Derived, RemoteBase>& lhs,
               const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (
        std::less<RemoteBase *> { } (lhs.get_remote_base_ptr (), rhs.get_remote_base_ptr ())))
    {
      return std::less<RemoteBase *> { } (lhs.get_remote_base_ptr (), rhs.get_remote_base_ptr ());
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator>= (const reporter_base_common<Derived, RemoteBase>& lhs,
                const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (! (lhs < rhs)))
    {
      return ! (lhs < rhs);
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator> (const reporter_base_common<Derived, RemoteBase>& lhs,
               const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (rhs < lhs))
    {
      return rhs < lhs;
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator<= (const reporter_base_common<Derived, RemoteBase>& lhs,
                const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (rhs >= lhs))
    {
      return rhs >= lhs;
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator== (const reporter_base_common<Derived, RemoteBase>& lhs,
                const RemoteBase *rhs) noexcept
    {
      return lhs.get_remote_base_ptr () == rhs;
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator== (const RemoteBase *lhs,
                const reporter_base_common<Derived, RemoteBase>& rhs) noexcept
    {
      return lhs == rhs.get_remote_base_ptr ();
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator!= (const reporter_base_common<Derived, RemoteBase>& lhs,
                const RemoteBase *rhs) noexcept
    {
      return lhs.get_remote_base_ptr () != rhs;
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator!= (const RemoteBase *lhs,
                const reporter_base_common<Derived, RemoteBase>& rhs) noexcept
    {
      return lhs != rhs.get_remote_base_ptr ();
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator< (const reporter_base_common<Derived, RemoteBase>& lhs, const RemoteBase *rhs)
      noexcept (noexcept (std::less<const RemoteBase *> { } (lhs.get_remote_base_ptr (), rhs)))
    {
      return std::less<const RemoteBase *> { } (lhs.get_remote_base_ptr (), rhs);
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator< (const RemoteBase *lhs, const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (std::less<const RemoteBase *> { } (lhs, rhs.get_remote_base_ptr ())))
    {
      return std::less<const RemoteBase *> { } (lhs, rhs.get_remote_base_ptr ());
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator>= (const reporter_base_common<Derived, RemoteBase>& lhs, const RemoteBase *rhs)
      noexcept (noexcept (! (lhs < rhs)))
    {
      return ! (lhs < rhs);
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator>= (const RemoteBase *lhs, const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (! (lhs < rhs)))
    {
      return ! (lhs < rhs);
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator> (const reporter_base_common<Derived, RemoteBase>& lhs, const RemoteBase *rhs)
      noexcept (noexcept (rhs < lhs))
    {
      return rhs < lhs;
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator> (const RemoteBase *lhs, const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (rhs < lhs))
    {
      return rhs < lhs;
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator<= (const reporter_base_common<Derived, RemoteBase>& lhs, const RemoteBase *rhs)
      noexcept (noexcept (rhs >= lhs))
    {
      return rhs >= lhs;
    }

    template <typename Derived, typename RemoteBase>
    constexpr
    bool
    operator<= (const RemoteBase *lhs, const reporter_base_common<Derived, RemoteBase>& rhs)
      noexcept (noexcept (rhs >= lhs))
    {
      return rhs >= lhs;
    }

    template <typename Derived, typename RemoteBase>
    constexpr
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
      using traits = tracker_traits<reporter_base<LocalBaseTag, tag::reporter_base>>;
    public:

      using local_base_tag  = typename traits::local_base_tag;
      using remote_base_tag = typename traits::remote_base_tag;

      using local_reporter_type  = typename traits::local_reporter_type;
      using remote_reporter_type = typename traits::remote_reporter_type;

      using local_access_type  = typename traits::local_access_type;
      using remote_access_type = typename traits::remote_access_type;

      using local_base_type  = typename traits::local_base_type;
      using remote_base_type = typename traits::remote_base_type;

    private:
      using base = reporter_base_common<reporter_base, remote_base_type>;

    public:
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
      using traits = tracker_traits<reporter_base<LocalBaseTag, tag::tracker_base>>;
    public:

      using local_base_tag  = typename traits::local_base_tag;
      using remote_base_tag = typename traits::remote_base_tag;

      using local_reporter_type  = typename traits::local_reporter_type;
      using remote_reporter_type = typename traits::remote_reporter_type;

      using local_access_type  = typename traits::local_access_type;
      using remote_access_type = typename traits::remote_access_type;

      using local_const_access_type  = typename traits::local_const_access_type;
      using remote_const_access_type = typename traits::remote_const_access_type;

      using local_base_type  = typename traits::local_base_type;
      using remote_base_type = typename traits::remote_base_type;

    private:
      using base = reporter_base_common<reporter_base, remote_base_type>;

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

      reporter_base (gch::tag::bind_t, remote_base_type& remote, remote_const_access_type pos)
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
        return static_cast<std::size_t> (base::get_remote_base ().get_reporter_offset (m_self));
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

    template <typename Interface>
    class reporter_common;

    /////////////////////
    // reporter_common //
    /////////////////////

    template <typename Parent, typename RemoteTag, typename IntrusiveTag>
    class reporter_common<reporter<Parent, RemoteTag, IntrusiveTag>>
      : private reporter_base<tag::reporter_base, typename RemoteTag::base_tag>,
        public tracker_traits<reporter<Parent, RemoteTag, IntrusiveTag>>
    {
      using base = reporter_base<tag::reporter_base, typename RemoteTag::base_tag>;
      using traits = tracker_traits<reporter<Parent, RemoteTag, IntrusiveTag>>;

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
      has_remote (remote_parent_type& remote) const noexcept
      {
        return has_remote () && (&remote == get_remote_ptr ());
      }

      GCH_NODISCARD constexpr
      remote_parent_type&
      get_remote (void) const noexcept
      {
        return get_remote_interface ().get_parent ();
      }

      GCH_NODISCARD constexpr
      remote_parent_type *
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

  } // gch::detail

  //////////////////////////
  // reporter (intrusive) //
  //////////////////////////

  template <typename Derived, typename RemoteTag>
  class reporter<Derived, RemoteTag, tag::intrusive>
    : public detail::reporter_common<reporter<Derived, RemoteTag, tag::intrusive>>
  {
    using base = detail::reporter_common<reporter<Derived, RemoteTag, tag::intrusive>>;

  public:
    using derived_type  = Derived;
    using remote_tag    = RemoteTag;
    using intrusive_tag = tag::intrusive;

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
    using parent_type   = Parent;
    using remote_tag    = RemoteTag;
    using intrusive_tag = tag::nonintrusive;

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

    reporter (parent_type& parent, typename base::remote_interface_type& remote)
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

  template <typename RemoteTag>
  struct standalone_reporter
    : public reporter<standalone_reporter<RemoteTag>, RemoteTag, tag::intrusive>
  {
    using base = reporter<standalone_reporter, RemoteTag, tag::intrusive>;
  public:
    using base::base;
  };

  template <typename Derived, typename RemoteTag>
  using intrusive_reporter = reporter<Derived, RemoteTag, tag::intrusive>;

  namespace intrusive
  {

    template <typename Derived, typename RemoteTag>
    using reporter = gch::reporter<Derived, RemoteTag, tag::intrusive>;

  }

}

#endif // OCTAVE_IR_REPORTER_HPP
