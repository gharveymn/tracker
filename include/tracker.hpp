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
#  define TRK_NODISCARD [[nodiscard]]
#else
#  define TRK_NODISCARD
#endif

namespace trk
{

  struct bind_tag_t { };
  inline constexpr bind_tag_t bind_with { };

  namespace detail
  {

    template <typename LocalBaseTag, typename RemoteBaseTag>
    class reporter_base;

    template <typename RemoteBaseTag>
    class tracker_base;
    
    struct reporter_base_tag;
    struct tracker_base_tag;

    template <typename ...Ts>
    using list = plf::list<Ts...>;
    
    // support types
    
    template <typename...>
    using map_void = void;
    
    template <typename RemoteBaseCommon>
    class reporter_base_common
    {
    public:
      using remote_base_type = RemoteBaseCommon;

      reporter_base_common  (void)                                  = default;
      reporter_base_common  (const reporter_base_common&)           = default;
      reporter_base_common  (reporter_base_common&& other) noexcept = default;
      
      reporter_base_common& 
      operator=             (const reporter_base_common&)           = default;
      
      reporter_base_common& 
      operator=             (reporter_base_common&& other) noexcept = default;
      
      ~reporter_base_common (void)                                  = default;
      
      explicit constexpr reporter_base_common (bind_tag_t, 
                                               remote_base_type& remote) 
        noexcept
        : m_remote_base (remote)
      { }
      
      void swap (reporter_base_common& other) noexcept 
      {
        using std::swap;
        swap (this->m_remote_base, other.m_remote_base);
      }

      TRK_NODISCARD
      constexpr bool has_remote (void) const noexcept
      {
        return m_remote_base.has_value ();
      }

      TRK_NODISCARD
      constexpr remote_base_type& get_remote_base (void) const noexcept
      {
        return *m_remote_base;
      }
      
      void set_remote (remote_base_type& remote) noexcept 
      {
        m_remote_base.emplace (remote);
      }
      
      void reset_remote (void) noexcept 
      {
        m_remote_base.reset ();
      }
      
    private:
      
      gch::optional_ref<remote_base_type> m_remote_base;
      
    };
  
    template <typename LocalBaseTag>
    class reporter_base<LocalBaseTag, reporter_base_tag>
      : public reporter_base_common<reporter_base<reporter_base_tag, 
                                                  LocalBaseTag>>
    {
    public:
  
      using local_tag_type  = LocalBaseTag;
      using remote_tag_type = reporter_base_tag;
      using remote_base_type = reporter_base<remote_tag_type, local_tag_type>;
      using remote_reporter_type = remote_base_type;
      
      using base = reporter_base_common<remote_base_type>;
      
      using base::base;
      reporter_base            (void)                           = default;
      reporter_base            (const reporter_base&)           = delete;
      reporter_base            (reporter_base&& other) noexcept = delete;
      reporter_base& operator= (const reporter_base&)           = delete;
      reporter_base& operator= (reporter_base&& other) noexcept = delete;
      ~reporter_base           (void)                           = default;
      
      void orphan_remote (void) noexcept 
      {
        if (base::has_remote ())
          base::get_remote_base ().orphan ();
      }
      
      void orphan (void) noexcept
      {
        base::reset_remote ();
      }

      TRK_NODISCARD
      constexpr bool is_tracked (void) const noexcept 
      {
        return base::has_remote ();
      }

      TRK_NODISCARD
      constexpr remote_base_type& get_remote_reporter (void) const noexcept
      {
        return base::get_remote_base ();
      }
  
    };
  
    template <typename LocalBaseTag>
    class reporter_base<LocalBaseTag, tracker_base_tag>
      : public reporter_base_common<tracker_base<LocalBaseTag>>
    {
    public:
      
    using local_tag_type  = LocalBaseTag;
    using remote_tag_type = tracker_base_tag;
    using remote_base_type = tracker_base<local_tag_type>;
    using remote_reporter_type = reporter_base<remote_tag_type, local_tag_type>;
    
    using base = reporter_base_common<remote_base_type>;

    private:
      
      using self_iter = typename list<remote_reporter_type>::iterator;
  
  //    friend void remote_type::transfer_from (remote_type&& src,
  //                                            ext_citer_type pos) noexcept;
    public:

      reporter_base            (void)                           = default;
  //  reporter_base            (const reporter_base&)           = impl;
  //  reporter_base            (reporter_base&& other) noexcept = impl;
  //  reporter_base& operator= (const reporter_base&)           = impl;
  //  reporter_base& operator= (reporter_base&& other) noexcept = impl;
      ~reporter_base           (void)                           = default;
      
      // This would be a lot less verbose if we defined a destructor for the
      // orphan functions, but we need to allow the derived types to quickly
      // destroy instances.
  
      explicit reporter_base (bind_tag_t, remote_base_type& remote)
        : base (bind_with, remote),
          m_self_iter (remote.track (*this)),
          m_is_tracked (true)
      { }
  
      explicit reporter_base (remote_base_type& remote, self_iter it)
        : base (bind_with, remote),
          m_self_iter (it),
          m_is_tracked (true)
      { }
  
      reporter_base (const reporter_base& other)
      {
        base::set_remote (other.get_remote_base ());
        if (other.has_remote ())
          set_self_iter (other.get_remote_base ().track (*this));
      }
  
      reporter_base (reporter_base&& other) noexcept
      {
        this->set (other);
        other.reset ();
      }
  
      reporter_base& operator= (const reporter_base& other)
      {
        if (&other != this)
          {
            if (other.has_remote ())
              rebind (other.get_remote_base ());
            else
              {
                if (is_tracked ())
                  base::get_remote_base ().orphan (get_self_iter ());
                reset ();
              }
          }
        return *this;
      }
  
      reporter_base& operator= (reporter_base&& other) noexcept
      {
        if (&other != this)
          {
            if (is_tracked ())
              base::get_remote_base ().orphan (get_self_iter ());
            this->set (other);
            other.reset ();
          }
        return *this;
      }
  
      void orphan_remote (void) noexcept
      {
        if (is_tracked ())
          {
            base::get_remote_base ().orphan (get_self_iter ());
            reset_self_iter ();
          }
      }
  
      TRK_NODISCARD
      std::size_t get_position (void) const noexcept
      {
        if (! is_tracked ())
          return 0;
        return base::get_remote_base ().base_get_offset (get_self_iter ());
      }
  
      reporter_base& rebind (remote_base_type& new_remote)
      {
        // if the remotes are the same then this is already in the remote,
        // so we shouldn't do anything unless we aren't being tracked right now.
        if (! base::has_remote () || &new_remote != &base::get_remote_base ())
          {
            // might fail
            self_iter new_iter = new_remote.track (*this);
  
            // if we didn't fail the rest is noexcept
            if (is_tracked ())
              base::get_remote_base ().orphan (get_self_iter ());
            
            set (new_remote, new_iter);
          }
        else if (! is_tracked ())
          {
            set_self_iter (new_remote.track (*this));
          }
        return *this;
      }
  
      // WARNING! This is a pure swap. Move to private when ready.
      void swap (reporter_base& other) noexcept
      {
        using std::swap;
        swap (this->m_remote_base, other.m_remote);
        swap (this->m_self_iter, other.m_self_iter);
        swap (this->m_is_tracked, other.m_is_tracked);
      }
  
      TRK_NODISCARD
      constexpr bool is_tracked (void) const noexcept
      {
        return m_is_tracked;
      }
  
      TRK_NODISCARD
      constexpr remote_reporter_type& 
      get_remote_reporter (void) const noexcept
      {
        return *get_self_iter ();
      }
      
      void orphan (void) noexcept
      {
        reset_self_iter ();
      }
  
  //  protected:
  
      reporter_base& set (const reporter_base& other) noexcept
      {
        base::operator= (other);
        if ((m_is_tracked = other.m_is_tracked))
          m_self_iter = other.m_self_iter;
        return *this;
      }
  
      reporter_base& set (remote_base_type& remote, self_iter it) noexcept
      {
        base::set_remote (remote);
        set_self_iter (it);
        return *this;
      }
      
      void reset (void) noexcept 
      {
        base::reset_remote ();
        m_is_tracked = false;
      }
  
    private:
      
      constexpr void set_self_iter (self_iter it) noexcept 
      {
        m_self_iter = it;
        m_is_tracked = true;
      }
  
      constexpr void reset_self_iter (void) noexcept
      {
        m_is_tracked = false;
      }

      TRK_NODISCARD
      constexpr self_iter get_self_iter (void) const noexcept
      {
        return m_self_iter;
      }
  
      self_iter m_self_iter;
      bool      m_is_tracked = false;
  
    };
  
    //! Remote is one of the non-base classes defined here
    template <typename RemoteBaseTag>
    class tracker_base
    {
    public:
  
      using local_tag_type  = tracker_base_tag;
      using remote_tag_type = RemoteBaseTag;
  
    protected:
  
      // Store pointers to the base types. Downcast when needed.
      using reporter_type   = reporter_base<local_tag_type, remote_tag_type>;
      using reporter_list   = list<reporter_type>;
      using base_iter   = typename reporter_list::iterator;
      using base_citer  = typename reporter_list::const_iterator;
      using base_riter  = typename reporter_list::reverse_iterator;
      using base_criter = typename reporter_list::const_reverse_iterator;
      using base_ref    = typename reporter_list::reference;
      using base_cref   = typename reporter_list::const_reference;
      
      using remote_base_type = typename reporter_type::remote_base_type;
      using remote_reporter_type = typename reporter_type::remote_reporter_type;
      using remote_reporter_list = list<remote_reporter_type>;
      using remote_base_iter = typename remote_reporter_list::iterator;
  
    public:
  
      tracker_base            (void)                    = default;
      tracker_base            (const tracker_base&)     = delete;
  //  tracker_base            (tracker_base&&) noexcept = impl;
      tracker_base& operator= (const tracker_base&)     = delete;
  //  tracker_base& operator= (tracker_base&&) noexcept = impl;
  //  ~tracker_base           (void)                    = impl;
  
      tracker_base (tracker_base&& other) noexcept
      {
        base_transfer_from (std::move (other));
      }
  
      tracker_base& operator= (tracker_base&& other) noexcept
      {
        reset ();
        base_transfer_from (std::move (other));
        return *this;
      }
  
      ~tracker_base (void) noexcept
      {
        reset ();
      }
  
      void reset (void) noexcept
      {
        if (! m_reporters.empty ())
          {
            for (base_ref p : m_reporters)
              p.orphan_remote ();
            m_reporters.clear ();
          }
      }
  
      void orphan (base_iter it)
      {
        this->erase (it);
      }
  
      void swap (tracker_base& other) noexcept
      {
        // kinda expensive   
        m_reporters.swap (other.m_reporters);
        this->repoint_reporters (*this);
        other.repoint_reporters (other);
      }
  
      TRK_NODISCARD
      std::size_t num_reporters (void) const noexcept
      {
        return m_reporters.size ();
      }
  
      void base_transfer_from (tracker_base&& src,
                                   base_citer pos) noexcept
      {
        src.repoint_reporters (*this);
        return m_reporters.splice (pos, src.m_reporters);
      }
  
      void base_transfer_from (tracker_base& src,
                                   base_citer pos) noexcept
      {
        return base_transfer_from (std::move (src), pos);
      }
  
      template <typename T>
      void base_transfer_from (T&& src) noexcept
      {
        return base_transfer_from (std::forward<T> (src),
                                       m_reporters.cend ());
      }
  
      // unsafe!
      void clear (void) noexcept
      {
        m_reporters.clear ();
      }
  
      base_iter   base_begin   (void)       noexcept { return m_reporters.begin (); }
      base_citer  base_begin   (void) const noexcept { return m_reporters.begin (); }
      base_citer  base_cbegin  (void) const noexcept { return m_reporters.cbegin (); }
  
      base_iter   base_end     (void)       noexcept { return m_reporters.end (); }
      base_citer  base_end     (void) const noexcept { return m_reporters.end (); }
      base_citer  base_cend    (void) const noexcept { return m_reporters.cend (); }
  
      base_riter  base_rbegin  (void)       noexcept { return m_reporters.rbegin (); }
      base_criter base_rbegin  (void) const noexcept { return m_reporters.rbegin (); }
      base_criter base_crbegin (void) const noexcept { return m_reporters.crbegin (); }
  
      base_riter  base_rend    (void)       noexcept { return m_reporters.rend (); }
      base_criter base_rend    (void) const noexcept { return m_reporters.rend (); }
      base_criter base_crend   (void) const noexcept { return m_reporters.crend (); }
  
      base_ref    base_front   (void)                { return m_reporters.front (); }
      base_cref   base_front   (void) const          { return m_reporters.front (); }
  
      base_ref    base_back    (void)                { return m_reporters.back (); }
      base_cref   base_back    (void) const          { return m_reporters.back (); }
  
      TRK_NODISCARD
      bool   empty   (void) const noexcept { return m_reporters.empty (); }
      void   reverse (void)       noexcept { return m_reporters.reverse (); }
  
      // safe, may throw
      base_iter track (void)
      {
        return m_reporters.emplace (m_reporters.end ());
      }
  
      // safe, may throw
      base_iter track (remote_base_type& remote)
      {
        return m_reporters.emplace (m_reporters.end (),
                                    bind_with, remote);
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
  
      // safe
      base_iter untrack (base_iter pos)
      {
        pos->orphan_remote ();
        return m_reporters.erase (pos);
      }
  
      // safe
      base_iter untrack (base_iter first, const base_iter last)
      {
        while (first != last)
          first = untrack (first);
        return last;
      }
  
      // unsafe!
      base_iter erase (base_citer cit) noexcept
      {
        return m_reporters.erase (cit);
      }
  
      TRK_NODISCARD
      std::size_t base_get_offset (base_citer pos) const noexcept
      {
        return std::distance (base_cbegin (), pos);
      }
  
      void repoint_reporters (tracker_base& tkr) noexcept
      {
        for (base_ref c_ptr : m_reporters)
          c_ptr.get_remote_reporter ().set_remote (tkr);
      }

      base_iter base_bind (remote_base_type& remote) = delete;
      
      template <typename It>
      base_iter base_bind (It first, It last);

      base_citer copy_reporters (const tracker_base& other) = delete;

      base_citer overwrite_reporters (const tracker_base& other) = delete;
  
    private:
  
      reporter_list m_reporters;
  
    };

    template <>
    auto 
    tracker_base<reporter_base_tag>::base_bind (remote_base_type& remote) 
      -> base_iter
    {
      base_iter it = this->track ();
      it->set_remote (remote.set (*this, it));
      return it;
    }

    template <>
    auto
    tracker_base<tracker_base_tag>::base_bind (remote_base_type& remote)
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
    tracker_base<tracker_base_tag>::copy_reporters (const tracker_base& other) 
      -> base_citer
    {
      if (! other.empty ())
        {
          auto binder = [this] (auto&& reporter)
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
              this->untrack(pivot, this->base_end ());
              throw;
            }
          return pivot;
        }
      return this->base_end ();
    }

    template <>
    auto 
    tracker_base<tracker_base_tag>::overwrite_reporters (
      const tracker_base& other)
      -> base_citer
    {
      base_citer pivot = copy_reporters (other);
      return this->untrack (this->base_begin (), pivot);
    }

    template <>
    tracker_base<tracker_base_tag>::tracker_base (const tracker_base& other)
    {
      copy_reporters (other);
    }

    template <>
    tracker_base<tracker_base_tag>& 
    tracker_base<tracker_base_tag>::operator= (const tracker_base& other)
    {
      if (&other != this)
        overwrite_reporters (other);
      return *this;
    }
    
    template <typename Tag>
    template <typename It>
    auto tracker_base<Tag>::base_bind (It first, 
                                           const It last) -> base_iter
    {
      if (first == last)
        return base_end ();
      const base_iter ret = base_bind (*first);
      std::for_each (++first, last, [this] (remote_base_type& remote) 
                                    { base_bind (remote); });
      return ret;        
    }
    
  }

  template <typename LocalParent, typename RemoteParent, typename RemoteTag>
  class intrusive_reporter;

  template <typename LocalParent, typename RemoteParent, typename RemoteTag>
  class reporter;

  template <typename LocalParent, typename RemoteParent, typename RemoteTag>
  class intrusive_tracker;

  template <typename LocalParent, typename RemoteParent, typename RemoteTag>
  class tracker;

  struct intrusive_reporter_tag
  {
    template <typename ...Ts>
    using type = intrusive_reporter<Ts...>;
    using base = detail::reporter_base_tag;
  };

  struct intrusive_tracker_tag
  {
    template <typename ...Ts>
    using type = intrusive_tracker<Ts...>;
    using base = detail::tracker_base_tag;
  };
  
  struct reporter_tag
  {
    template <typename ...Ts>
    using type = reporter<Ts...>;
    using base = detail::reporter_base_tag;
  };

  struct tracker_tag
  {
    template <typename ...Ts>
    using type = tracker<Ts...>;
    using base = detail::tracker_base_tag;
  };

  // pretend like reporter_base doesn't exist
  template <typename ReporterCIter, typename Remote, typename RemoteParent>
  struct reporter_iterator
  {
  private:
    using reporter_citer     = ReporterCIter;
    using reporter_type      = typename ReporterCIter::value_type;
    using remote_type        = Remote;
    using remote_parent_type = RemoteParent;

  public:

    using difference_type   = typename reporter_citer::difference_type;
    using iterator_category = std::bidirectional_iterator_tag;
    //    using value_type        = remote_type;
    //    using pointer           = remote_type *;
    //    using const_pointer     = const remote_type *;
    //    using reference         = remote_type&;
    //    using const_reference   = const remote_type&;

    template <typename Iter>
    constexpr /* implicit */ reporter_iterator (Iter it)
      : m_citer (reporter_citer (it))
    { }

    //    template <typename ...Ts>
    //    reporter_iterator (reporter_iterator<reporter_citer, Ts...> it)
    //      : m_citer (it.get_iterator ())
    //    { }

    reporter_iterator (const reporter_iterator&)     = default;
    reporter_iterator (reporter_iterator&&) noexcept = default;

    // ref-qualified to prevent assignment to rvalues
    reporter_iterator& operator= (const reporter_iterator& other) &
    {
      if (&other != this)
        m_citer = other.m_citer;
      return *this;
    }

    // ref-qualified to prevent assignment to rvalues
    reporter_iterator& operator= (reporter_iterator&& other) & noexcept
    {
      if (&other != this)
        m_citer = std::move (other.m_citer);
      return *this;
    }

    explicit operator reporter_citer (void)
    {
      return m_citer;
    }

//      constexpr auto& operator* (void) noexcept
//      {
//        return get_remote_parent();
//      }

    constexpr remote_parent_type& operator* (void) const noexcept
    {
      return get_remote_parent();
    }

    constexpr remote_parent_type * operator-> (void) const noexcept
    {
      return &get_remote_parent ();
    }

    reporter_iterator& operator++ (void) noexcept
    {
      ++m_citer;
      return *this;
    }

    reporter_iterator operator++ (int) noexcept
    {
      const reporter_iterator tmp = *this;
      ++m_citer;
      return tmp;
    }

    reporter_iterator& operator-- (void) noexcept
    {
      --m_citer;
      return *this;
    }

    reporter_iterator operator-- (int) noexcept
    {
      const reporter_iterator tmp = *this;
      --m_citer;
      return tmp;
    }

    friend bool operator== (const reporter_iterator& x,
                            const reporter_iterator& y) noexcept
    {
      return x.m_citer == y.m_citer;
    }

    friend bool operator!= (const reporter_iterator& x,
                            const reporter_iterator& y) noexcept
    {
      return x.m_citer != y.m_citer;
    }

    reporter_citer get_iterator (void) const noexcept
    {
      return m_citer;
    }

  private:

    constexpr remote_parent_type& get_remote_parent (void) const noexcept
    {
      return get_remote_interface ().get_parent ();
    }

    constexpr remote_type& get_remote_interface (void) const noexcept
    {
      return static_cast<remote_type&> (m_citer->get_remote_base ());
    }

    reporter_citer m_citer;

  };
  
  namespace detail
  {
    template <typename RemoteTag>
    class reporter_common
      : public reporter_base<reporter_base_tag, typename RemoteTag::base>
    {
    public:

      using base = reporter_base<reporter_base_tag, typename RemoteTag::base>;
      using remote_base_type = typename base::remote_base_type;
      
      using base::base;

      reporter_common            (void)                       = default;
      reporter_common            (const reporter_common&)     = default;
//    reporter_common            (reporter_common&&) noexcept = impl;
      reporter_common& operator= (const reporter_common&)     = default;
//    reporter_common& operator= (reporter_common&&) noexcept = impl;
//    ~reporter_common           (void)                       = impl;

      reporter_common (reporter_common&& other) noexcept
      : base (std::move (other))
      {
        if (this->is_tracked ())
          this->get_remote_reporter ().set_remote (*this);
      }

      reporter_common& operator= (reporter_common&& other) noexcept
      {
        if (&other != this)
          {
            base::operator= (std::move (other));
            if (this->is_tracked ())
              this->get_remote_reporter ().set_remote (*this);
          }
        return *this;
      }

      ~reporter_common (void) noexcept
      {
        if (base::is_tracked ())
          base::orphan_remote ();
      }

      void swap (reporter_common& other) noexcept
      {
        if (&other.get_remote_base () != &this->get_remote_base ())
          {
            using std::swap;
            base::swap (other);

            if (this->is_tracked ())
              this->get_remote_reporter ().set_remote (*this);

            if (other.is_tracked ())
              other.get_remote_reporter ().set_remote (other);
          }
      }
    };

    template <typename LocalParent, typename RemoteParent, 
              typename RemoteTag, typename LocalTag>
    class tracker_common 
      : public tracker_base<typename RemoteTag::base>
    {
      
      using base           = tracker_base<typename RemoteTag::base>;
      using base_citer = typename base::base_citer;

      template <typename Compare, typename Head, typename ...Tail>
      struct all_same
        : std::integral_constant<bool, std::is_same<Compare, Head>::value
                                       && all_same<Compare, Tail...>::value>
      { };

      template <typename Compare, typename T>
      struct all_same<Compare, T> : std::is_same<Compare, T>
      { };
      
    public:
      using remote_type = typename RemoteTag::template type<RemoteParent,
        LocalParent, LocalTag>;
      
      using iter   = reporter_iterator<base_citer, remote_type, RemoteParent>;
      using citer  = iter;
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
      
      explicit tracker_common (bind_tag_t, remote_type& remote)
      { 
        base::base_bind (remote);
      }

      explicit tracker_common (bind_tag_t,
        std::initializer_list<std::reference_wrapper<remote_type>> init)
      {
        base::base_bind (init.begin (), init.end ());
      }

      iter   begin   (void)       noexcept { return base::base_begin ();   }
      citer  begin   (void) const noexcept { return base::base_begin ();   }
      citer  cbegin  (void) const noexcept { return base::base_cbegin ();  }

      iter   end     (void)       noexcept { return base::base_end ();     }
      citer  end     (void) const noexcept { return base::base_end ();     }
      citer  cend    (void) const noexcept { return base::base_cend ();    }

      riter  rbegin  (void)       noexcept { return base::base_rbegin ();  }
      criter rbegin  (void) const noexcept { return base::base_rbegin ();  }
      criter crbegin (void) const noexcept { return base::base_crbegin (); }

      riter  rend    (void)       noexcept { return base::base_rend ();    }
      criter rend    (void) const noexcept { return base::base_rend ();    }
      criter crend   (void) const noexcept { return base::base_crend ();   }

      ref    front   (void)                { return *begin ();             }
      cref   front   (void) const          { return *begin ();             }

      ref    back    (void)                { return *--end ();             }
      cref   back    (void) const          { return *--end ();             }

      TRK_NODISCARD
      bool has_reporters (void) const noexcept
      {
        return ! base::empty ();
      }

      template <typename T>
      void transfer_from (T&& src, citer pos) noexcept
      {
        return base::base_transfer_from (std::forward<T> (src), pos);
      }

      template <typename T>
      void transfer_from (T&& src) noexcept
      {
        return base::base_transfer_from (std::forward<T> (src));
      }

      std::size_t get_offset (citer pos) const noexcept
      {
        return base::base_get_offset (base_citer (pos));
      }

      template <typename ...Args>
      typename std::enable_if<all_same<remote_type, Args...>::value>::type
      bind (remote_type& r, Args&... args)
      {
        base::base_bind (r);
        bind (args...);
      }

      citer bind (remote_type& r)
      {
        return base::base_bind (r);
      }
      
    };
  }
  
//  template <typename Tag, typename ...Ts>
//  using tag_type = typename Tag::template type<Ts...>;

  template <typename LocalParent, typename RemoteParent, 
            typename RemoteTag = tracker_tag>
  class intrusive_reporter 
    : public detail::reporter_common<RemoteTag>
  {
    using base = detail::reporter_common<RemoteTag>;
  public:
    using remote_type = typename RemoteTag::template type<RemoteParent,
                                                          LocalParent,
                                                       intrusive_reporter_tag>;
    
    using base::base;
    intrusive_reporter            (void)                          = default;
    intrusive_reporter            (const intrusive_reporter&)     = default;
    intrusive_reporter            (intrusive_reporter&&) noexcept = default;
    intrusive_reporter& operator= (const intrusive_reporter&)     = default;
    intrusive_reporter& operator= (intrusive_reporter&&) noexcept = default;
    ~intrusive_reporter           (void)                          = default;
    
    constexpr explicit intrusive_reporter (bind_tag_t, 
                                           remote_type& remote) noexcept 
      : base (bind_with, remote)
    { }
    
    TRK_NODISCARD
    constexpr bool has_parent (void) const noexcept
    {
      return true;
    }

    TRK_NODISCARD
    constexpr LocalParent& get_parent (void) noexcept
    {
      return static_cast<LocalParent&> (*this);
    }

    TRK_NODISCARD
    constexpr const LocalParent& get_parent (void) const noexcept
    {
      return static_cast<const LocalParent&> (*this);
    }

    TRK_NODISCARD
    constexpr bool has_remote_parent (void) const noexcept
    {
      return base::has_remote () && get_remote_interface ().has_parent ();
    }

    TRK_NODISCARD // auto is actually necessary here
    constexpr auto& get_remote_parent (void) const noexcept
    {
      return get_remote_interface ().get_parent ();
    }
    
  private:
    
    constexpr remote_type& get_remote_interface (void)
    {
      return static_cast<remote_type&> (base::get_remote_base ());
    }
    
  };

  template <typename LocalParent, typename RemoteParent, 
            typename RemoteTag = reporter_tag>
  class intrusive_tracker 
    : public detail::tracker_common<LocalParent, RemoteParent, RemoteTag, 
                                    intrusive_tracker_tag>
  {
    using base = detail::tracker_common<LocalParent, RemoteParent, RemoteTag,
                                        intrusive_tracker_tag>;
  public:
    using remote_type = typename base::remote_type;
    using iter        = typename base::iter;
    using citer       = typename base::citer;
    using riter       = typename base::riter;
    using criter      = typename base::criter;
    using ref         = typename base::ref;
    using cref        = typename base::cref;

    using base::base;
    intrusive_tracker            (void)                         = default;
    intrusive_tracker            (const intrusive_tracker&)     = default;
    intrusive_tracker            (intrusive_tracker&&) noexcept = default;
    intrusive_tracker& operator= (const intrusive_tracker&)     = default;
    intrusive_tracker& operator= (intrusive_tracker&&) noexcept = default;
    ~intrusive_tracker           (void)                         = default;

    TRK_NODISCARD
    constexpr bool has_parent (void) const noexcept
    {
      return true;
    }

    TRK_NODISCARD
    constexpr LocalParent& get_parent (void) noexcept
    {
      return static_cast<LocalParent&> (*this);
    }

    TRK_NODISCARD
    constexpr const LocalParent& get_parent (void) const noexcept
    {
      return static_cast<const LocalParent&> (*this);
    }

    template <typename ...Args>
    remote_type create (Args... args)
    {
      return { *this, std::forward<Args> (args)... };
    }
    
  };

  template <typename LocalParent, typename RemoteParent,
            typename RemoteTag = tracker_tag>
  class reporter 
    : public detail::reporter_common<RemoteTag>
  {
    using base = detail::reporter_common<RemoteTag>;
  public:
    using parent_type = LocalParent;
    using remote_type = typename RemoteTag::template type<RemoteParent,
                                                          LocalParent,
                                                          reporter_tag>;

    using base::base;
    reporter            (void)                = default;
    reporter            (const reporter&)     = delete;
    reporter            (reporter&&) noexcept = delete;
//  reporter& operator= (const reporter&)     = impl;
//  reporter& operator= (reporter&&) noexcept = impl;
    ~reporter           (void)                = default;

    explicit reporter (bind_tag_t, remote_type& remote)
      : base (bind_with, remote)
    { }

    explicit reporter (parent_type& parent, remote_type& remote)
      : base (bind_with, remote),
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

    reporter& operator= (const reporter& other)
    {
      if (&other != this)
        base::operator= (other);
      return *this;
    }

    reporter& operator= (reporter&& other) noexcept
    {
      base::operator= (std::move (other));
      return *this;
    }

    void swap (reporter& other)
    {
      base::swap (other);

      using std::swap;
      swap (m_parent, other.m_parent);
    }

    TRK_NODISCARD
    constexpr bool has_remote_parent (void) const noexcept
    {
      return base::has_remote () && get_remote_interface ().has_parent ();
    }

    TRK_NODISCARD // auto is actually necessary here
    constexpr auto& get_remote_parent (void) const noexcept
    {
      return get_remote_interface ().get_parent ();
    }

    TRK_NODISCARD
    constexpr bool has_parent (void) const noexcept
    {
      return m_parent.has_value ();
    }

    TRK_NODISCARD
    constexpr parent_type& get_parent (void) const noexcept
    {
      return *m_parent;
    }

  private:

    constexpr remote_type& get_remote_interface (void)
    {
      return static_cast<remote_type&> (base::get_remote_base ());
    }

    gch::optional_ref<parent_type> m_parent;
    
  };

  template <typename LocalParent, typename RemoteParent, 
            typename RemoteTag = reporter_tag>
  class tracker 
    : public detail::tracker_common<LocalParent, RemoteParent, 
                                    RemoteTag, tracker_tag>
  {

    using base = detail::tracker_common<LocalParent, RemoteParent, 
                                        RemoteTag, tracker_tag>;
    
  public:
    using parent_type = LocalParent;
    using remote_type = typename base::remote_type;
    using iter        = typename base::iter;
    using citer       = typename base::citer;
    using riter       = typename base::riter;
    using criter      = typename base::criter;
    using ref         = typename base::ref;
    using cref        = typename base::cref;

    using base::base;
    tracker            (void)               = default;
    tracker            (const tracker&)     = default;
    tracker            (tracker&&) noexcept = default;
//  tracker& operator= (const tracker&)     = impl;
//  tracker& operator= (tracker&&) noexcept = impl;
    ~tracker           (void)               = default;

    explicit tracker (parent_type& parent)
      : m_parent (parent)
    { }

    explicit tracker (parent_type& parent, remote_type& remote)
      : base (bind_with, remote),
        m_parent (parent)
    { }

    explicit tracker (parent_type& parent, 
      std::initializer_list<std::reference_wrapper<remote_type>> init)
      : base (bind_with, init),
        m_parent (parent)
    { }
    
    tracker (const tracker& other, parent_type& new_parent)
      : base (other),
        m_parent (new_parent)
    { }

    tracker (tracker&& other, parent_type& new_parent) noexcept
      : base (std::move (other)),
        m_parent (new_parent)
    { }

    // explicitly defined so that m_parent doesn't get implicitly reset
    tracker& operator= (const tracker& other)
    {
      if (&other != this)
        base::operator= (other);
      return *this;
    }
    
    tracker& operator= (tracker&& other) noexcept
    {
      base::operator= (std::move (other));
      return *this;
    }

    void swap (tracker& other) noexcept
    {
      base::swap (other);
      using std::swap;
      swap (m_parent, other.m_parent); // caution!
    }

    TRK_NODISCARD
    constexpr bool has_parent (void) const noexcept
    {
      return m_parent.has_value ();
    }

    TRK_NODISCARD
    constexpr parent_type& get_parent (void) const noexcept
    {
      return *m_parent;
    }
    
  private:

    gch::optional_ref<parent_type> m_parent;
    
  };
  
  template <typename LocalParent, typename RemoteParent = LocalParent>
  using multireporter = tracker<LocalParent, RemoteParent, tracker_tag>;

  template <typename ...Ts, typename ...RemoteTypes>
  void bind (tracker<Ts...>& l, RemoteTypes&... Remotes)
  {
    l.bind (Remotes...);
  }
  
}

#undef TRK_NODISCARD

#endif
