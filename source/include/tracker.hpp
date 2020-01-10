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
    GCH_INLINE_VARS constexpr struct { } bind;
  }
  
  template <typename LocalParent, typename RemoteParent, 
            typename RemoteTag, typename IntrusiveTag>
  class reporter;
  
  template <typename LocalParent, typename RemoteParent, 
            typename RemoteTag, typename IntrusiveTag>
  class tracker;

  namespace detail
  {
  
    template <typename ...Ts>
    using list = plf::list<Ts...>;
  
    namespace tag
    {
      struct reporter_base;
      struct tracker_base;
      using  gch::tag::bind;
    }
  
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
    
      explicit constexpr reporter_base_common (decltype (tag::bind), remote_base_type& remote)
      noexcept
        : m_remote_base (remote)
      { }
    
      void swap (reporter_base_common& other) noexcept
      {
        using std::swap;
        swap (this->m_remote_base, other.m_remote_base);
      }
    
      GCH_NODISCARD
      constexpr remote_base_type& get_remote_base (void) const noexcept
      {
        return *m_remote_base;
      }
  
      GCH_NODISCARD
      constexpr bool is_tracked (void) const noexcept
      {
        return m_remote_base.has_value ();
      }
      
      GCH_NODISCARD
      constexpr bool is_tracking (remote_base_type& remote_cmp) const noexcept 
      {
        return &remote_cmp == m_remote_base.get_pointer ();
      }
    
      void track (remote_base_type& remote) noexcept
      {
        m_remote_base.emplace (remote);
      }
    
      // local asymmetric debind
      void reset (void) noexcept
      {
        m_remote_base.reset ();
      }
  
      // symmetrically debind
      GCH_CPP14_CONSTEXPR void debind (void) noexcept
      {
        static_cast<derived_type *> (this)->reset_remote_tracking ();
        reset ();        
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
//    reporter_base            (reporter_base&& other) noexcept = impl;
      reporter_base& operator= (const reporter_base&)           = default;
//    reporter_base& operator= (reporter_base&& other) noexcept = impl;
      ~reporter_base           (void)                           = default;
  
      reporter_base (reporter_base&& other) noexcept
        : base (std::move (other))
      {
        if (base::is_tracked ())
          get_remote_reporter ().track (*this);
      }
      
      reporter_base& operator= (reporter_base&& other) noexcept
      {
        if (&other != this)
        {
          base::operator= (std::move (other));
          if (base::is_tracked ())
            get_remote_reporter ().track (*this);
        }
        return *this;
      }
  
      // remote asymmetric debinding
      void reset_remote_tracking (void) noexcept 
      {
        if (base::is_tracked ())
          base::get_remote_base ().reset ();
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
      
      using self_iter = typename list<remote_reporter_type>::iterator;
  
    public:

      reporter_base            (void)                           = default;
//    reporter_base            (const reporter_base&)           = impl;
//    reporter_base            (reporter_base&& other) noexcept = impl;
//    reporter_base& operator= (const reporter_base&)           = impl;
//    reporter_base& operator= (reporter_base&& other) noexcept = impl;
      ~reporter_base           (void)                           = default;
      
      // Note that the destructor here does NOT perform a debinding. This is 
      // for optimizations higher in the hierarchy.
  
      explicit reporter_base (decltype (tag::bind), remote_base_type& remote)
        : base (tag::bind, remote),
          m_self_iter (remote.track (*this))
      { }
  
      explicit reporter_base (remote_base_type& remote, self_iter it)
        : base (tag::bind, remote),
          m_self_iter (std::move (it))
      { }
  
      reporter_base (const reporter_base& other)
      {
        base::track (other.get_remote_base ());
        if (other.is_tracked ())
          set_self_iter (other.get_remote_base ().track (*this));
      }
  
      reporter_base (reporter_base&& other) noexcept
        : base (std::move (other)),
          m_self_iter (other.m_self_iter)
      {
        if (base::is_tracked ())
          m_self_iter->track (*this);
        other.reset ();
      }
  
      reporter_base& operator= (const reporter_base& other)
      {
        if (&other != this)
          {
            if (other.is_tracked ())
              rebind (other.get_remote_base ());
            else
              base::debind ();
          }
        return *this;
      }
  
      reporter_base& operator= (reporter_base&& other) noexcept
      {
        if (&other != this)
          {
            reset_remote_tracking ();
            if (other.is_tracked ())
            {
              base::operator= (std::move (other));
              m_self_iter = other.m_self_iter;
              m_self_iter->track (*this);
            }
            else
            {
              base::reset ();
            }
            other.reset ();
          }
        return *this;
      }
  
      // remote asymmetric debind
      GCH_CPP14_CONSTEXPR void reset_remote_tracking (void) noexcept
      {
        if (base::is_tracked ())
          base::get_remote_base ().erase (get_self_iter ());
      }
  
      GCH_NODISCARD
      std::size_t get_position (void) const noexcept
      {
        if (! base::is_tracked ())
          return 0;
        return base::get_remote_base ().base_get_offset (get_self_iter ());
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
            set_self_iter (new_remote.track (*this));
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
        return *get_self_iter ();
      }
  
  //  protected:
  
      reporter_base& set (remote_base_type& remote, self_iter it) noexcept
      {
        base::track (remote);
        set_self_iter (it);
        return *this;
      }
  
    private:
  
      GCH_CPP14_CONSTEXPR void set_self_iter (self_iter it) noexcept 
      {
        m_self_iter = it;
      }

      GCH_NODISCARD
      constexpr self_iter get_self_iter (void) const noexcept
      {
        return m_self_iter;
      }
  
      self_iter m_self_iter;
  
    };
  
    //! Remote is one of the non-base classes defined here
    template <typename RemoteBaseTag>
    class tracker_base
    {
    public:
  
      using local_base_tag  = tag::tracker_base;
      using remote_base_tag = RemoteBaseTag;
  
    protected:
  
      // Store pointers to the base types. Downcast when needed.
      using reporter_type   = reporter_base<local_base_tag, remote_base_tag>;
      using reporter_list   = list<reporter_type>;
      using base_iter       = typename reporter_list::iterator;
      using base_citer      = typename reporter_list::const_iterator;
      using base_riter      = typename reporter_list::reverse_iterator;
      using base_criter     = typename reporter_list::const_reverse_iterator;
      using base_ref        = typename reporter_list::reference;
      using base_cref       = typename reporter_list::const_reference;
      
      using remote_base_type     = typename reporter_type::remote_base_type;
      using remote_reporter_type = typename reporter_type::remote_reporter_type;
      using remote_reporter_list = list<remote_reporter_type>;
      using remote_base_iter     = typename remote_reporter_list::iterator;
  
    public:
  
      tracker_base            (void)                    = default;
      tracker_base            (const tracker_base&)     = delete;
  //  tracker_base            (tracker_base&&) noexcept = impl;
      tracker_base& operator= (const tracker_base&)     = delete;
  //  tracker_base& operator= (tracker_base&&) noexcept = impl;
  //  ~tracker_base           (void)                    = impl;
  
      tracker_base (tracker_base&& other) noexcept
      {
        base_transfer_bindings (other, base_cend ());
      }
  
      tracker_base& operator= (tracker_base&& other) noexcept
      {
        clear ();
        base_transfer_bindings (other, base_cend ());
        return *this;
      }
  
      //! safe by default
      ~tracker_base (void) noexcept
      {
        clear ();
      }
  
      //! unsafe if needed
      void wipe (void) noexcept
      {
        m_reporters.clear ();
      }
  
      void reset (void) noexcept 
      {
        clear ();
      }
  
      void clear (void) noexcept
      {
        if (! m_reporters.empty ())
          {
            for (base_ref p : m_reporters)
              p.reset_remote_tracking ();
            m_reporters.clear ();
          }
      }
      
      void swap (tracker_base& other) noexcept
      {
        // kinda expensive   
        m_reporters.swap (other.m_reporters);
        this->repoint_reporters (*this);
        other.repoint_reporters (other);
      }
  
      GCH_NODISCARD
      std::size_t num_reporters (void) const noexcept
      {
        return m_reporters.size ();
      }
  
      void base_transfer_bindings (tracker_base& src, base_citer pos) noexcept
      {
        src.repoint_reporters (*this);
        return m_reporters.splice (pos, src.m_reporters);
      }
  
      GCH_NODISCARD base_iter   base_begin   (void)       noexcept { return m_reporters.begin (); }
      GCH_NODISCARD base_citer  base_begin   (void) const noexcept { return m_reporters.begin (); }
      GCH_NODISCARD base_citer  base_cbegin  (void) const noexcept { return m_reporters.cbegin (); }
  
      GCH_NODISCARD base_iter   base_end     (void)       noexcept { return m_reporters.end (); }
      GCH_NODISCARD base_citer  base_end     (void) const noexcept { return m_reporters.end (); }
      GCH_NODISCARD base_citer  base_cend    (void) const noexcept { return m_reporters.cend (); }
  
      GCH_NODISCARD base_riter  base_rbegin  (void)       noexcept { return m_reporters.rbegin (); }
      GCH_NODISCARD base_criter base_rbegin  (void) const noexcept { return m_reporters.rbegin (); }
      GCH_NODISCARD base_criter base_crbegin (void) const noexcept { return m_reporters.crbegin (); }
  
      GCH_NODISCARD base_riter  base_rend    (void)       noexcept { return m_reporters.rend (); }
      GCH_NODISCARD base_criter base_rend    (void) const noexcept { return m_reporters.rend (); }
      GCH_NODISCARD base_criter base_crend   (void) const noexcept { return m_reporters.crend (); }
  
      GCH_NODISCARD base_ref    base_front   (void)                { return m_reporters.front (); }
      GCH_NODISCARD base_cref   base_front   (void) const          { return m_reporters.front (); }
  
      GCH_NODISCARD base_ref    base_back    (void)                { return m_reporters.back (); }
      GCH_NODISCARD base_cref   base_back    (void) const          { return m_reporters.back (); }
  
      GCH_NODISCARD bool        base_empty   (void) const noexcept { return m_reporters.empty (); }
      
      void   reverse (void) noexcept { return m_reporters.reverse (); }
  
      // safe, may throw
      base_iter track (void)
      {
        return m_reporters.emplace (m_reporters.end ());
      }
  
      // safe, may throw
      base_iter track (remote_base_type& remote)
      {
        return m_reporters.emplace (m_reporters.end (),
                                    tag::bind, remote);
      }
  
      // safe, may throw
      base_iter track (remote_base_type& remote, remote_base_iter it)
      {
        return m_reporters.emplace (m_reporters.end (), remote, it);
      }
      
      base_iter append_uninit (std::size_t num)
      {
        return m_reporters.insert (m_reporters.cend (), num, 
                                   remote_base_type {});
      }
  
      // unsafe!
      base_iter erase (base_citer cit) noexcept
      {
        return m_reporters.erase (cit);
      }
  
      GCH_NODISCARD
      std::size_t base_get_offset (base_citer pos) const noexcept
      {
        return std::distance (base_cbegin (), pos);
      }
  
      void repoint_reporters (tracker_base& tkr) noexcept
      {
        for (base_ref c_ptr : m_reporters)
          c_ptr.get_remote_reporter ().track (tkr);
      }

      base_iter base_bind (remote_base_type& remote) = delete;
      
      template <typename It>
      auto base_bind (It first, It last) -> typename std::enable_if<
        std::is_convertible<decltype (*first), remote_base_type&>::value, base_iter>::type;
  
      base_iter base_bind (base_iter first, base_iter last);
  
      //! safe
      base_iter base_debind (base_iter pos)
      {
        pos->reset_remote_tracking ();
        return m_reporters.erase (pos);
      }
  
      //! safe
      base_iter base_debind (base_iter first, const base_iter last)
      {
        while (first != last)
          first = base_debind (first);
        return last;
      }

      base_iter copy_reporters (const tracker_base& other) = delete;

      base_iter overwrite_reporters (const tracker_base& other) = delete;
  
    private:
  
      reporter_list m_reporters;
  
    };

    template <>
    auto 
    tracker_base<tag::reporter_base>::base_bind (remote_base_type& remote) 
      -> base_iter
    {
      base_iter it = this->track ();
      it->track (remote.set (*this, it));
      return it;
    }

    template <>
    auto
    tracker_base<tag::tracker_base>::base_bind (remote_base_type& remote)
      -> base_iter
    {
      base_iter it = this->track ();
      try
        {
          it->set (remote, remote.track (*this, it));
        }
      catch (...)
        {
          this->erase (it);
          throw;
        }
      return it;
    }

    template <>
    auto
    tracker_base<tag::tracker_base>::copy_reporters (const tracker_base& other) 
      -> base_iter
    {
      if (! other.base_empty ())
        {
          auto binder = [this] (const reporter_type& reporter)
          {
            return base_bind (reporter.get_remote_base ());
          };

          base_iter pivot = binder (other.base_front ());
          try
            {
              std::for_each (std::next (other.base_begin ()),
                             other.base_end (), binder);
            }
          catch (...)
            {
              this->base_debind (pivot, this->base_end ());
              throw;
            }
          return pivot;
        }
      return this->base_end ();
    }

    template <>
    auto 
    tracker_base<tag::tracker_base>::overwrite_reporters (const tracker_base& other)
      -> base_iter
    {
      base_iter pivot = copy_reporters (other);
      return this->base_debind (this->base_begin (), pivot);
    }

    template <>
    tracker_base<tag::tracker_base>::tracker_base (const tracker_base& other)
    {
      copy_reporters (other);
    }

    template <>
    tracker_base<tag::tracker_base>&
    tracker_base<tag::tracker_base>::operator= (const tracker_base& other)
    {
      if (&other != this)
        overwrite_reporters (other);
      return *this;
    }
    
    template <typename Tag>
    template <typename It>
    auto tracker_base<Tag>::base_bind (It first, It last) -> typename std::enable_if<
      std::is_convertible<decltype (*first), remote_base_type&>::value, base_iter>::type
    {
      if (first == last)
        return base_end ();
      const base_iter ret = base_bind (*first);
      std::for_each (++first, last, [this] (remote_base_type& remote) 
                                    { base_bind (remote); });
      return ret;        
    }
  
    template <typename Tag>
    auto tracker_base<Tag>::base_bind (base_iter first, const base_iter last)
      -> base_iter
    {
      if (first == last)
        return base_end ();
      const base_iter ret = base_bind (first->get_remote_base ());
      std::for_each (++first, last, [this] (reporter_type& reporter)
                                    { base_bind (reporter.get_remote_base ()); });
      return ret;
    }
  
    template <typename LocalParent, typename RemoteParent,
              typename LocalTag,    typename RemoteTag>
    class reporter_common;
  
    template <typename LocalParent, typename RemoteParent,
              typename LocalTag,    typename RemoteTag>
    class tracker_common;
  
  } // detail
  
  namespace tag
  {
    struct intrusive;
    struct nonintrusive;
    struct reporter;
    struct tracker;
  } // tag
  
  template <typename LocalParent, typename RemoteParent,
            typename RemoteTag    = tag::tracker,
            typename IntrusiveTag = tag::nonintrusive>
  class reporter;
  
  template <typename LocalParent, typename RemoteParent,
            typename RemoteTag    = tag::reporter,
            typename IntrusiveTag = tag::nonintrusive>
  class tracker;
  
  struct tag::reporter
  {
    template <typename ...Ts>
    using type = gch::reporter<Ts..., tag::nonintrusive>;
    using base = detail::tag::reporter_base;
    
    struct intrusive
    {
      template <typename ...Ts>
      using type = gch::reporter<Ts..., tag::intrusive>;
      using base = detail::tag::reporter_base;
    };
    using nonintrusive = tag::reporter;
  };
  
  struct tag::tracker
  {
    template <typename ...Ts>
    using type = gch::tracker<Ts..., tag::nonintrusive>;
    using base = detail::tag::tracker_base;
    
    struct intrusive
    {
      template <typename ...Ts>
      using type = gch::tracker<Ts..., tag::intrusive>;
      using base = detail::tag::tracker_base;
    };
    using nonintrusive = tag::tracker;
  };

  // pretend like reporter_base doesn't exist
  template <typename ReporterIt, typename RemoteInterface, typename RemoteParent>
  class remote_iterator
  {
    using reporter_iter         = ReporterIt;
    using reporter_type         = typename ReporterIt::value_type;
    using remote_interface_type = RemoteInterface;
    using remote_parent_type    = RemoteParent;

  public:

    using difference_type   = typename reporter_iter::difference_type;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type        = remote_parent_type;
    using pointer           = remote_parent_type *;
    using reference         = remote_parent_type&;
  
    template <typename LP, typename RP,
              typename RT, typename IT>
    friend class detail::tracker_common;

    template <typename Iter>
    constexpr /* implicit */ remote_iterator (Iter it)
      : m_iter (reporter_iter (it))
    { }

    remote_iterator (const remote_iterator&)     = default;
    remote_iterator (remote_iterator&&) noexcept = default;

    // ref-qualified to prevent assignment to rvalues
    remote_iterator& operator= (const remote_iterator& other) &
    {
      if (&other != this)
        m_iter = other.m_iter;
      return *this;
    }

    // ref-qualified to prevent assignment to rvalues
    remote_iterator& operator= (remote_iterator&& other) & noexcept
    {
      if (&other != this)
        m_iter = std::move (other.m_iter);
      return *this;
    }

    constexpr remote_parent_type& operator* (void) const noexcept
    {
      return get_remote_parent();
    }

    constexpr remote_parent_type * operator-> (void) const noexcept
    {
      return &get_remote_parent ();
    }

    remote_iterator& operator++ (void) noexcept
    {
      ++m_iter;
      return *this;
    }

    remote_iterator operator++ (int) noexcept
    {
      const remote_iterator tmp = *this;
      ++m_iter;
      return tmp;
    }

    remote_iterator& operator-- (void) noexcept
    {
      --m_iter;
      return *this;
    }

    remote_iterator operator-- (int) noexcept
    {
      const remote_iterator tmp = *this;
      --m_iter;
      return tmp;
    }

    friend bool operator== (const remote_iterator& l,
                            const remote_iterator& r) noexcept
    {
      return l.m_iter == r.m_iter;
    }

    friend bool operator!= (const remote_iterator& l,
                            const remote_iterator& r) noexcept
    {
      return l.m_iter != r.m_iter;
    }

    reporter_iter get_iterator (void) const noexcept
    {
      return m_iter;
    }

  private:
  
    /* implicit */ operator reporter_iter (void)
    {
      return m_iter;
    }

    constexpr remote_parent_type& get_remote_parent (void) const noexcept
    {
      return get_remote_interface ().get_parent ();
    }

    constexpr remote_interface_type& get_remote_interface (void) const noexcept
    {
      return static_cast<remote_interface_type&> (m_iter->get_remote_base ());
    }

    reporter_iter m_iter;

  };
  
  namespace detail
  {
  
    template <typename Compare, typename Head, typename ...Tail>
    struct all_same
      : std::integral_constant<bool, std::is_same<Compare, Head>::value
        && all_same<Compare, Tail...>::value>
    { };
  
    template <typename Compare, typename T>
    struct all_same<Compare, T> : std::is_same<Compare, T>
    { };
    
    template <typename Tag, typename ...Ts>
    using interface_t = typename Tag::template type<Ts...>;
    
    /////////////////////
    // reporter_common //
    /////////////////////
    
    template <typename LocalParent, typename RemoteParent,
              typename LocalTag,    typename RemoteTag>
    class reporter_common
      : protected reporter_base<tag::reporter_base, typename RemoteTag::base>
    {
    public:
  
      using base = reporter_base<tag::reporter_base, typename RemoteTag::base>;
      
      using local_parent_type     = LocalParent;
      using remote_parent_type    = RemoteParent;
      using local_interface_tag   = LocalTag;
      using remote_interface_tag  = RemoteTag;
      
      using local_interface_type  = interface_t<LocalTag, LocalParent, RemoteParent, RemoteTag>;
      using remote_interface_type = interface_t<RemoteTag, RemoteParent, LocalParent, LocalTag>;
      
      using remote_base_type      = typename base::remote_base_type;
      
      using base::base;
      using base::get_position;
      
      friend remote_interface_type;
      friend class reporter_common<RemoteParent, LocalParent, LocalTag, RemoteTag>;

      reporter_common            (void)                       = default;
      reporter_common            (const reporter_common&)     = default;
      reporter_common            (reporter_common&&) noexcept = default;
      reporter_common& operator= (const reporter_common&)     = default;
      reporter_common& operator= (reporter_common&&) noexcept = default;
//    ~reporter_common           (void)                       = impl   ;
  
      ~reporter_common (void)
      {
        if (base::is_tracked ())
          base::reset_remote_tracking ();
      }
  
      constexpr explicit reporter_common (decltype (tag::bind), 
                                          remote_interface_type& remote) noexcept
        : base (tag::bind, remote)
      { }

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
  
      local_interface_type& transfer_binding (local_interface_type&& other) noexcept
      {
        return static_cast<local_interface_type&> (base::operator= (std::move (other)));
      }
  
      local_interface_type& transfer_binding (local_interface_type& other) noexcept 
      {
        return transfer_binding (std::move (other));
      }
  
      local_interface_type& copy_binding (const local_interface_type& other)
      {
        return static_cast<local_interface_type&> (base::rebind (other.get_remote_interface ()));
      }
  
      GCH_NODISCARD 
      constexpr bool has_remote_parent (void) const noexcept
      {
        return base::has_remote () && get_remote_interface ().has_parent ();
      }
      
      GCH_NODISCARD 
      constexpr remote_parent_type& get_remote_parent (void) const noexcept
      {
        return get_remote_interface ().get_parent ();
      }
      
    private:
      
      GCH_NODISCARD 
      constexpr remote_interface_type& get_remote_interface (void) const noexcept
      {
        return static_cast<remote_interface_type&> (base::get_remote_base ());
      }
      
    }; // reporter_common
  
    ////////////////////
    // tracker_common //
    ////////////////////

    template <typename LocalParent, typename RemoteParent, 
              typename LocalTag,    typename RemoteTag>
    class tracker_common 
      : protected tracker_base<typename RemoteTag::base>
    {
      using local_parent_type    = LocalParent;
      using remote_parent_type   = RemoteParent;
      using local_interface_tag  = LocalTag;
      using remote_interface_tag = RemoteTag;
      
      using base                 = tracker_base<typename RemoteTag::base>;
      using base_iter            = typename base::base_iter;
      using base_citer           = typename base::base_citer;
      
    public:
      
      using local_interface_type  = interface_t<LocalTag, LocalParent, RemoteParent, RemoteTag>;
      using remote_interface_type = interface_t<RemoteTag, RemoteParent, LocalParent, LocalTag>;
    
      using iter   = remote_iterator<base_iter, remote_interface_type, remote_parent_type>;
      using citer  = remote_iterator<base_iter, remote_interface_type, const remote_parent_type>;
      using riter  = std::reverse_iterator<iter>;
      using criter = std::reverse_iterator<citer>;
      using ref    = RemoteParent&;
      using cref   = const RemoteParent&;

      using base::base;
      
      tracker_common            (void)                      = default;
      tracker_common            (const tracker_common&)     = default;
      tracker_common            (tracker_common&&) noexcept = default;
      tracker_common& operator= (const tracker_common&)     = default;
      tracker_common& operator= (tracker_common&&) noexcept = default;
      ~tracker_common           (void)                      = default;
  
      explicit tracker_common (decltype (tag::bind), remote_interface_type& remote)
      {
        base::base_bind (remote);
      }

    private:
      using init_list = std::initializer_list<std::reference_wrapper<remote_interface_type>>;
    public:
      explicit tracker_common (decltype (tag::bind), init_list init)
      {
        base::base_bind (init.begin (), init.end ());
      }
      
      // imported functions
      using base::clear;
      using base::wipe;
      using base::num_reporters;
  
      GCH_NODISCARD iter   begin         (void)       noexcept { return base::base_begin ();   }
      GCH_NODISCARD citer  begin         (void) const noexcept { return base::base_begin ();   }
      GCH_NODISCARD citer  cbegin        (void) const noexcept { return base::base_cbegin ();  }
  
      GCH_NODISCARD iter   end           (void)       noexcept { return base::base_end ();     }
      GCH_NODISCARD citer  end           (void) const noexcept { return base::base_end ();     }
      GCH_NODISCARD citer  cend          (void) const noexcept { return base::base_cend ();    }
  
      GCH_NODISCARD riter  rbegin        (void)       noexcept { return base::base_rbegin ();  }
      GCH_NODISCARD criter rbegin        (void) const noexcept { return base::base_rbegin ();  }
      GCH_NODISCARD criter crbegin       (void) const noexcept { return base::base_crbegin (); }
  
      GCH_NODISCARD riter  rend          (void)       noexcept { return base::base_rend ();    }
      GCH_NODISCARD criter rend          (void) const noexcept { return base::base_rend ();    }
      GCH_NODISCARD criter crend         (void) const noexcept { return base::base_crend ();   }
  
      GCH_NODISCARD ref    front         (void)                { return *begin ();             }
      GCH_NODISCARD cref   front         (void) const          { return *begin ();             }
  
      GCH_NODISCARD ref    back          (void)                { return *--end ();             }
      GCH_NODISCARD cref   back          (void) const          { return *--end ();             }
  
      GCH_NODISCARD bool   has_reporters (void) const noexcept { return ! base::base_empty (); }
      
      template <typename T>
      GCH_CPP14_CONSTEXPR void transfer_bindings (T&& src, citer pos) noexcept
      {
        return base::base_transfer_bindings (src, base_citer (pos));
      }
      
      template <typename T>
      GCH_CPP14_CONSTEXPR void transfer_bindings (T&& src) noexcept
      {
        return transfer_bindings (src, cend ());
      }
  
      iter copy_bindings (const local_interface_type& other)
      {
        return bind (other.begin (), other.end ());
      }
  
      GCH_CPP17_CONSTEXPR std::size_t get_offset (citer pos) const noexcept
      {
        return base::base_get_offset (base_citer (pos));
      }
      
      iter bind (citer first, citer last)
      {
        return base::base_bind (base_iter (first), base_iter (last));
      }
  
      iter bind (remote_interface_type& r) 
      { 
        return base::base_bind (r);
      }
  
      template <typename ...Args>
      typename std::enable_if<all_same<remote_interface_type, Args...>::value, iter>::type
      bind (remote_interface_type& r, Args&... args)
      {
        bind (args...);
        return bind (r);
      }
      
    }; // tracker_common
    
  } // detail

  template <typename LocalParent, typename RemoteParent, typename RemoteTag>
  class reporter<LocalParent, RemoteParent, RemoteTag, tag::intrusive>
    : private detail::reporter_common<LocalParent, RemoteParent, 
                                      tag::reporter::intrusive, RemoteTag>
  {
    
    using base = detail::reporter_common<LocalParent, RemoteParent, 
                                         tag::reporter::intrusive, RemoteTag>;
  public:
    using local_interface_tag   = tag::reporter::intrusive;
    using remote_interface_tag  = RemoteTag;
  
    using local_interface_type  = reporter;
    using remote_interface_type = typename base::remote_interface_type;
  
    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;
  
    using parent_type     = LocalParent;
    using remote_parent_type    = RemoteParent;
  
    template <typename, typename, typename>
    friend class remote_iterator;
    
    friend remote_common_type;
    friend remote_interface_type;
  
    using base::base;
    using base::swap;
    using base::get_position;
    using base::has_remote_parent;
    using base::get_remote_parent;
  
    reporter            (void)                = default;
    reporter            (const reporter&)     = default;
    reporter            (reporter&&) noexcept = default;
    reporter& operator= (const reporter&)     = default;
    reporter& operator= (reporter&&) noexcept = default;
    ~reporter           (void)                = default;
  
    reporter& rebind (remote_interface_type& new_remote)
    {
      base::rebind (new_remote);
      return *this;
    }
    
    GCH_NODISCARD
    constexpr bool has_parent (void) const noexcept
    {
      return true;
    }

    GCH_NODISCARD
    GCH_CPP14_CONSTEXPR parent_type& get_parent (void) noexcept
    {
      return static_cast<parent_type&> (*this);
    }

    GCH_NODISCARD
    constexpr const parent_type& get_parent (void) const noexcept
    {
      return static_cast<const parent_type&> (*this);
    }
    
  }; // intrusive_reporter
  
  template <typename LocalParent, typename RemoteParent, typename RemoteTag>
  class tracker<LocalParent, RemoteParent, RemoteTag, tag::intrusive>
    : private detail::tracker_common<LocalParent, RemoteParent, 
                                     tag::tracker::intrusive, RemoteTag>
  {
    using base = detail::tracker_common<LocalParent, RemoteParent, 
                                        tag::tracker::intrusive, RemoteTag>;
  public:
    using local_interface_tag   = tag::tracker::intrusive;
    using remote_interface_tag  = RemoteTag;
  
    using local_interface_type  = tracker;
    using remote_interface_type = typename base::remote_interface_type;
  
    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;
  
    using local_parent_type     = LocalParent;
    using remote_parent_type    = RemoteParent;
    
    using iter        = typename base::iter;
    using citer       = typename base::citer;
    using riter       = typename base::riter;
    using criter      = typename base::criter;
    using ref         = typename base::ref;
    using cref        = typename base::cref;
  
    template <typename, typename, typename>
    friend class remote_iterator;
  
    friend remote_common_type;
    friend remote_interface_type;

    using base::base;
    
    tracker            (void)               = default;
    tracker            (const tracker&)     = default;
    tracker            (tracker&&) noexcept = default;
    tracker& operator= (const tracker&)     = default;
    tracker& operator= (tracker&&) noexcept = default;
    ~tracker           (void)               = default;
  
    using base::begin;
    using base::cbegin;
    using base::end;
    using base::cend;
    using base::rbegin;
    using base::crbegin;
    using base::rend;
    using base::crend;
    using base::front;
    using base::back;
    using base::has_reporters;
    using base::transfer_bindings;
    using base::get_offset;
    using base::bind;
  
    // from tracker_base
    using base::wipe;
    using base::num_reporters;
    
    GCH_NODISCARD
    constexpr bool has_parent (void) const noexcept
    {
      return true;
    }

    GCH_NODISCARD
    GCH_CPP14_CONSTEXPR local_parent_type& get_parent (void) noexcept
    {
      return static_cast<local_parent_type&> (*this);
    }

    GCH_NODISCARD
    constexpr const local_parent_type& get_parent (void) const noexcept
    {
      return static_cast<const local_parent_type&> (*this);
    }

    template <typename ...Args>
    remote_interface_type create (Args... args)
    {
      return remote_interface_type (*this, std::forward<Args> (args)...);
    }
  }; // tracker
  
  template <typename LocalParent, typename RemoteParent, typename RemoteTag>
  class reporter<LocalParent, RemoteParent, RemoteTag, tag::nonintrusive>
    : public detail::reporter_common<LocalParent, RemoteParent, 
                                     tag::reporter::nonintrusive, RemoteTag>
  {
    using base = detail::reporter_common<LocalParent, RemoteParent, 
                                         tag::reporter::nonintrusive, RemoteTag>;
  public:
    using local_interface_tag   = tag::reporter::nonintrusive;
    using remote_interface_tag  = RemoteTag;
  
    using local_interface_type  = reporter;
    using remote_interface_type = typename base::remote_interface_type;
    
    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;
  
    using parent_type     = LocalParent;
    using remote_parent_type    = RemoteParent;
    
    template <typename, typename, typename>
    friend class remote_iterator;
    
    friend remote_common_type;
    friend remote_interface_type;

    using base::base;
    reporter            (void)                = default;
    reporter            (const reporter&)     = delete;
    reporter            (reporter&&) noexcept = delete;
    reporter& operator= (const reporter&)     = delete;
    reporter& operator= (reporter&&) noexcept = delete;
    ~reporter           (void)                = default;

    explicit reporter (decltype (tag::bind), remote_interface_type& remote)
      : base (tag::bind, remote)
    { }

    explicit reporter (parent_type& parent, remote_interface_type& remote)
      : base (tag::bind, remote),
        m_parent (parent)
    { }

    reporter (const reporter& other, parent_type& new_parent)
      : base (other),
        m_parent (new_parent)
    { }

    reporter (reporter&& other, parent_type& new_parent) noexcept
      : base (std::move (other)),
        m_parent (new_parent)
    { }

    void swap (reporter& other)
    {
      base::swap (other);

      using std::swap;
      swap (m_parent, other.m_parent);
    }
  
    reporter& rebind (remote_interface_type& new_remote)
    {
      base::rebind (new_remote);
      return *this;
    }

    GCH_NODISCARD
    constexpr bool has_parent (void) const noexcept
    {
      return m_parent.has_value ();
    }

    GCH_NODISCARD
    constexpr parent_type& get_parent (void) const noexcept
    {
      return *m_parent;
    }

  private:

    gch::optional_ref<parent_type> m_parent;
    
  };
  
  template <typename LocalParent, typename RemoteParent, typename RemoteTag>
  class tracker<LocalParent, RemoteParent, RemoteTag, tag::nonintrusive>
    : public detail::tracker_common<LocalParent, RemoteParent,
                                    tag::tracker::nonintrusive, RemoteTag>
  {
    using base = detail::tracker_common<LocalParent, RemoteParent,
                                        tag::tracker::nonintrusive, RemoteTag>;
  public:
    using local_interface_tag   = tag::tracker::nonintrusive;
    using remote_interface_tag  = RemoteTag;
  
    using local_interface_type  = tracker;
    using remote_interface_type = typename base::remote_interface_type;
  
    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;
  
    using local_parent_type     = LocalParent;
    using remote_parent_type    = RemoteParent;
    
    using iter        = typename base::iter;
    using citer       = typename base::citer;
    using riter       = typename base::riter;
    using criter      = typename base::criter;
    using ref         = typename base::ref;
    using cref        = typename base::cref;
  
    template <typename, typename, typename>
    friend class remote_iterator;
  
    friend remote_common_type;
    friend remote_interface_type;

    using base::base;
    tracker            (void)               = default;
    tracker            (const tracker&)     = default;
    tracker            (tracker&&) noexcept = default;
    tracker& operator= (const tracker&)     = delete;
    tracker& operator= (tracker&&) noexcept = delete;
    ~tracker           (void)               = default;

    explicit tracker (local_parent_type& parent)
      : m_parent (parent)
    { }

    explicit tracker (local_parent_type& parent, remote_interface_type& remote)
      : base (tag::bind, remote),
        m_parent (parent)
    { }

    explicit tracker (local_parent_type& parent, 
      std::initializer_list<std::reference_wrapper<remote_interface_type>> init)
      : base (tag::bind, init),
        m_parent (parent)
    { }
    
    tracker (const tracker& other, local_parent_type& new_parent)
      : base (other),
        m_parent (new_parent)
    { }

    tracker (tracker&& other, local_parent_type& new_parent) noexcept
      : base (std::move (other)),
        m_parent (new_parent)
    { }

    void swap (tracker& other) noexcept
    {
      base::swap (other);
      using std::swap;
      swap (m_parent, other.m_parent); // caution!
    }

    GCH_NODISCARD
    constexpr bool has_parent (void) const noexcept
    {
      return m_parent.has_value ();
    }
  
    GCH_NODISCARD
    GCH_CPP14_CONSTEXPR local_parent_type& get_parent (void) noexcept
    {
      return *m_parent;
    }

    GCH_NODISCARD
    constexpr local_parent_type& get_parent (void) const noexcept
    {
      return *m_parent;
    }
    
  private:

    gch::optional_ref<local_parent_type> m_parent;
    
  };
  
  //! standalone tracker (no parent)
  template <typename RemoteParent, typename RemoteTag>
  class tracker<tag::tracker, RemoteParent, RemoteTag, tag::nonintrusive>
    : public tracker<tracker<tag::tracker, RemoteParent, RemoteTag, tag::nonintrusive>, 
                     RemoteParent, RemoteTag, tag::intrusive>
  { };
  
  template <typename LocalParent, typename RemoteParent = LocalParent>
  using multireporter = tracker<LocalParent, RemoteParent, 
                                tag::tracker::nonintrusive, tag::nonintrusive>;

  template <typename ...Ts, typename ...RemoteTypes>
  void bind (tracker<Ts...>& l, RemoteTypes&... Remotes)
  {
    l.bind (Remotes...);
  }
  
}

#undef GCH_NODISCARD

#endif
