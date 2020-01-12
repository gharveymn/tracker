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
    struct reporter;
    struct tracker;
    GCH_INLINE_VARS constexpr struct bind_t { } bind;
  }
  
  namespace detail::tag
  {
    struct reporter_base { using base = reporter_base; };
    struct tracker_base  { using base = tracker_base;  };
  }
  
  template <typename Local, typename Remote,
            typename RemoteTag    = tag::tracker,
            typename IntrusiveTag = tag::nonintrusive>
  class reporter;
  
  template <typename Local, typename Remote,
            typename RemoteTag    = tag::reporter,
            typename IntrusiveTag = tag::nonintrusive>
  class tracker;
  
  struct tag::reporter : detail::tag::reporter_base
  {
    template <typename ...Ts>
    using type = gch::reporter<Ts..., tag::nonintrusive>;
    
    struct intrusive : detail::tag::reporter_base
    {
      template <typename ...Ts>
      using type = gch::reporter<Ts..., tag::intrusive>;
    };
    
    using nonintrusive = tag::reporter;
  };
  
  struct tag::tracker : detail::tag::tracker_base
  {
    template <typename ...Ts>
    using type = gch::tracker<Ts..., tag::nonintrusive>;
    
    struct intrusive : detail::tag::tracker_base
    {
      template <typename ...Ts>
      using type = gch::tracker<Ts..., tag::intrusive>;
    };
    
    using nonintrusive = tag::tracker;
  };
  
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
    
      explicit constexpr reporter_base_common (gch::tag::bind_t, remote_base_type& remote) noexcept
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
  
      explicit reporter_base (gch::tag::bind_t, remote_base_type& remote)
        : base (gch::tag::bind, remote),
          m_self_iter (remote.track (*this))
      { }
  
      explicit reporter_base (remote_base_type& remote, self_iter it)
        : base (gch::tag::bind, remote),
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
      using rptr_iter       = typename reporter_list::iterator;
      using rptr_citer      = typename reporter_list::const_iterator;
      using rptr_riter      = typename reporter_list::reverse_iterator;
      using rptr_criter     = typename reporter_list::const_reverse_iterator;
      using rptr_ref        = typename reporter_list::reference;
      using rptr_cref       = typename reporter_list::const_reference;
      
      using remote_base_type     = typename reporter_type::remote_base_type;
      using remote_reporter_type = typename reporter_type::remote_reporter_type;
      using remote_reporter_list = list<remote_reporter_type>;
      using remote_rptr_iter     = typename remote_reporter_list::iterator;
  
    public:
  
      tracker_base            (void)                    = default;
      tracker_base            (const tracker_base&);
//    tracker_base            (tracker_base&&) noexcept = impl;
      tracker_base& operator= (const tracker_base&);
//    tracker_base& operator= (tracker_base&&) noexcept = impl;
      ~tracker_base           (void)                    = default;
      
      tracker_base (tracker_base&& other) noexcept 
      {
        base_transfer_bindings (other, rptrs_cend ());
      }
  
      tracker_base& operator= (tracker_base&& other) noexcept
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
            for (auto& p : m_rptrs)
              p.reset_remote_tracking ();
            m_rptrs.clear ();
          }
      }
      
      void swap (tracker_base& other) noexcept
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
      
      rptr_iter base_transfer_bindings (tracker_base& src, rptr_citer pos) noexcept
      {
        rptr_iter pivot = src.rptrs_begin ();
        repoint_reporters (src.rptrs_begin(), src.rptrs_end ());
        m_rptrs.splice (pos, src.m_rptrs);
        return pivot;
      }
  
      GCH_NODISCARD rptr_iter   rptrs_begin   (void)       noexcept { return m_rptrs.begin (); }
      GCH_NODISCARD rptr_citer  rptrs_begin   (void) const noexcept { return m_rptrs.begin (); }
      GCH_NODISCARD rptr_citer  rptrs_cbegin  (void) const noexcept { return m_rptrs.cbegin (); }
      
      GCH_NODISCARD rptr_iter   rptrs_end     (void)       noexcept { return m_rptrs.end (); }
      GCH_NODISCARD rptr_citer  rptrs_end     (void) const noexcept { return m_rptrs.end (); }
      GCH_NODISCARD rptr_citer  rptrs_cend    (void) const noexcept { return m_rptrs.cend (); }
      
      GCH_NODISCARD rptr_riter  rptrs_rbegin  (void)       noexcept { return m_rptrs.rbegin (); }
      GCH_NODISCARD rptr_criter rptrs_rbegin  (void) const noexcept { return m_rptrs.rbegin (); }
      GCH_NODISCARD rptr_criter rptrs_crbegin (void) const noexcept { return m_rptrs.crbegin (); }
      
      GCH_NODISCARD rptr_riter  rptrs_rend    (void)       noexcept { return m_rptrs.rend (); }
      GCH_NODISCARD rptr_criter rptrs_rend    (void) const noexcept { return m_rptrs.rend (); }
      GCH_NODISCARD rptr_criter rptrs_crend   (void) const noexcept { return m_rptrs.crend (); }
      
      GCH_NODISCARD rptr_ref    rptrs_front   (void)                { return m_rptrs.front (); }
      GCH_NODISCARD rptr_cref   rptrs_front   (void) const          { return m_rptrs.front (); }
      
      GCH_NODISCARD rptr_ref    rptrs_back    (void)                { return m_rptrs.back (); }
      GCH_NODISCARD rptr_cref   rptrs_back    (void) const          { return m_rptrs.back (); }
      
      GCH_NODISCARD bool        rptrs_empty   (void) const noexcept { return m_rptrs.empty (); }
      
      void rptrs_reverse (void) noexcept
      {
        return m_rptrs.reverse (); 
      }
      
      template <typename ...Args>
      void rptrs_splice (Args&&... args) noexcept 
      {
        return m_rptrs.splice (std::forward<Args> (args)...); 
      }
  
      // safe, may throw
      rptr_iter track (void)
      {
        return m_rptrs.emplace (m_rptrs.end ());
      }
  
      // safe, may throw
      rptr_iter track (remote_base_type& remote)
      {
        return m_rptrs.emplace (m_rptrs.end (),
                                gch::tag::bind, remote);
      }
  
      // safe, may throw
      rptr_iter track (remote_base_type& remote, remote_rptr_iter it)
      {
        return m_rptrs.emplace (m_rptrs.end (), remote, it);
      }
      
      rptr_iter append_uninit (std::size_t num)
      {
        return m_rptrs.insert (m_rptrs.cend (), num,
                               remote_base_type {});
      }
  
      // unsafe!
      rptr_iter erase (rptr_citer cit) noexcept
      {
        return m_rptrs.erase (cit);
      }
  
      GCH_NODISCARD
      std::size_t base_get_offset (rptr_citer pos) const noexcept
      {
        return std::distance (rptrs_cbegin (), pos);
      }
  
      void repoint_reporters (rptr_iter first, rptr_iter last) noexcept
      {
        std::for_each (first, last, [this] (reporter_type& rptr)
                                    { rptr.get_remote_reporter ().track (*this); });
      }
  
      template <typename Tag = remote_base_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      rptr_iter base_bind (remote_base_type& remote) = delete;
  
      template <typename Tag = remote_base_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      rptr_iter base_bind (remote_base_type& remote)
      {
        rptr_iter it = this->track ();
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
      
      template <typename It, 
                typename Tag = remote_base_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      rptr_iter base_bind (It first, It last) = delete;
  
      template <typename It,
                typename Tag = remote_base_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      rptr_iter base_bind (It first, It last)
      {
        if (first == last)
          return this->rptrs_end ();
  
        const rptr_iter pivot = base_bind (first->get_remote_base ());
        try
        {
          std::for_each (++first, last, [this] (remote_base_type& remote)
          { base_bind (remote); });
        }
        catch (...)
        {
          this->base_debind (pivot, this->rptrs_end ());
          throw;
        }
        return pivot;
      }
  
      template <typename Tag = remote_base_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      rptr_iter base_bind (rptr_citer first, rptr_citer last) = delete;
  
      template <typename Tag = remote_base_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      rptr_iter base_bind (rptr_citer first, rptr_citer last)
      {
        if (first == last)
          return this->rptrs_end ();
  
        const rptr_iter pivot = base_bind (first->get_remote_base ());
        try
        {
          std::for_each (++first, last, [this] (const reporter_type& reporter)
          { base_bind (reporter.get_remote_base ()); });
        }
        catch (...)
        {
          this->base_debind (pivot, this->rptrs_end ());
          throw;
        }
        return pivot;
      }
  
      //! safe
      rptr_iter base_debind (rptr_iter pos)
      {
        pos->reset_remote_tracking ();
        return m_rptrs.erase (pos);
      }
  
      //! safe
      rptr_iter base_debind (rptr_iter first, const rptr_iter last)
      {
        while (first != last)
          first = base_debind (first);
        return last;
      }
  
    private:
  
      reporter_list m_rptrs;
  
    };

    // template <>
    // tracker_base<tag::reporter_base>::rptr_iter 
    // tracker_base<tag::reporter_base>::base_bind (remote_base_type& remote) = delete;
    //
    // template <>
    // tracker_base<tag::tracker_base>::rptr_iter
    // tracker_base<tag::tracker_base>::base_bind (remote_base_type& remote)
    // {
    //   rptr_iter it = this->track ();
    //   try
    //     {
    //       it->set (remote, remote.track (*this, it));
    //     }
    //   catch (...)
    //     {
    //       this->erase (it);
    //       throw;
    //     }
    //   return it;
    // }
  
    // template <>
    // template <typename It>
    // tracker_base<tag::tracker_base>::rptr_iter
    // tracker_base<tag::tracker_base>::base_bind (It first, const It last)
    // {
    //   if (first == last)
    //     return this->rptrs_end ();
    //
    //   const rptr_iter pivot = base_bind (first->get_remote_base ());
    //   try
    //   {
    //     std::for_each (++first, last, [this] (remote_base_type& remote)
    //                                   { base_bind (remote); });
    //   }
    //   catch (...)
    //   {
    //     this->base_debind (pivot, this->rptrs_end ());
    //     throw;
    //   }
    //   return pivot;
    // }
  
    // template <>
    // template <>
    // tracker_base<tag::reporter_base>::rptr_iter
    // tracker_base<tag::reporter_base>::base_bind (rptr_citer first, rptr_citer last) = delete;
    //
    // template <>
    // template <>
    // tracker_base<tag::tracker_base>::rptr_iter
    // tracker_base<tag::tracker_base>::base_bind (rptr_citer first, const rptr_citer last)
    // {
    //   if (first == last)
    //     return this->rptrs_end ();
    //  
    //   const rptr_iter pivot = base_bind (first->get_remote_base ());
    //   try
    //   {
    //     std::for_each (++first, last, [this] (const reporter_type& reporter)
    //                                   { base_bind (reporter.get_remote_base ()); });
    //   }
    //   catch (...)
    //   {
    //     this->base_debind (pivot, this->rptrs_end ());
    //     throw;
    //   }
    //   return pivot;
    // }
  
    template <>
    tracker_base<tag::reporter_base>::tracker_base (const tracker_base& other) = delete;
  
    template <>
    tracker_base<tag::tracker_base>::tracker_base (const tracker_base& other)
    {
      base_bind (other.rptrs_begin (), other.rptrs_end ());
    }
  
    template <>
    tracker_base<tag::reporter_base>&
    tracker_base<tag::reporter_base>::operator= (const tracker_base& other) = delete;
  
    template <>
    tracker_base<tag::tracker_base>&
    tracker_base<tag::tracker_base>::operator= (const tracker_base& other)
    {
      if (&other == this)
        return *this;
      
      rptr_iter pivot = base_bind (other.rptrs_begin (), other.rptrs_end ());
      base_debind (rptrs_begin (), pivot);
      return *this;
    }
  
    template <typename Local,    typename Remote,
              typename LocalTag, typename RemoteTag>
    class reporter_common;
  
    template <typename Local,    typename Remote,
              typename LocalTag, typename RemoteTag>
    class tracker_common;
  
  } // detail

  // pretend like reporter_base doesn't exist
  template <typename ReporterIt, typename RemoteInterface, typename Remote>
  class remote_iterator
  {
    using reporter_iter         = ReporterIt;
    using reporter_type         = typename ReporterIt::value_type;
    using remote_interface_type = RemoteInterface;
    using remote_type           = Remote;

  public:

    using difference_type   = typename reporter_iter::difference_type;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type        = remote_type;
    using pointer           = remote_type *;
    using reference         = remote_type&;
  
    template <typename, typename, typename, typename>
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

    constexpr remote_type& operator* (void) const noexcept
    {
      return get_remote_parent();
    }

    constexpr remote_type * operator-> (void) const noexcept
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

    constexpr remote_type& get_remote_parent (void) const noexcept
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
    
    template <typename Local,    typename Remote,
              typename LocalTag, typename RemoteTag>
    using interface_t = typename LocalTag::template type<Local, Remote, RemoteTag>;
    
    /////////////////////
    // reporter_common //
    /////////////////////
    
    template <typename Local,    typename Remote,
              typename LocalTag, typename RemoteTag>
    class reporter_common
      : protected reporter_base<tag::reporter_base, typename RemoteTag::base>
    {
    public:
      
      using base = reporter_base<tag::reporter_base, typename RemoteTag::base>;
      
      using local_type            = Local;
      using remote_type           = Remote;
  
      using local_base_tag       = typename LocalTag::base;
      using remote_base_tag      = typename RemoteTag::base;
      
      using local_interface_tag   = LocalTag;
      using remote_interface_tag  = RemoteTag;
      
      using local_interface_type  = interface_t<Local, Remote, LocalTag, RemoteTag>;
      using remote_interface_type = interface_t<Remote, Local, RemoteTag, LocalTag>;
      
      using remote_base_type      = typename base::remote_base_type;
      
      using base::base;
      using base::get_position;
      
      friend remote_interface_type;
      friend class reporter_common<Remote, Local, LocalTag, RemoteTag>;
      
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
  
      constexpr explicit reporter_common (gch::tag::bind_t, 
                                          remote_interface_type& remote) noexcept
        : base (gch::tag::bind, remote)
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
      constexpr remote_type& get_remote_parent (void) const noexcept
      {
        return get_remote_interface ().get_parent ();
      }
  
      GCH_NODISCARD
      constexpr remote_interface_type& get_remote_interface (void) const noexcept
      {
        return static_cast<remote_interface_type&> (base::get_remote_base ());
      }
      
    private:
      
      
    }; // reporter_common
  
    ////////////////////
    // tracker_common //
    ////////////////////

    template <typename Local,    typename Remote,
              typename LocalTag, typename RemoteTag>
    class tracker_common
      : protected tracker_base<typename RemoteTag::base>
    {
      using local_type           = Local;
      using remote_type          = Remote;
      
      using local_base_tag       = typename LocalTag::base;
      using remote_base_tag      = typename RemoteTag::base;
      
      using local_tag  = LocalTag;
      using remote_tag = RemoteTag;
      
      using base                 = tracker_base<typename RemoteTag::base>;
      using rptr_iter            = typename base::rptr_iter;
      using rptr_citer           = typename base::rptr_citer;
      
    public:
      
      using local_interface_type  = interface_t<Local, Remote, LocalTag, RemoteTag>;
      using remote_interface_type = interface_t<Remote, Local, RemoteTag, LocalTag>;
    
      using iter   = remote_iterator<rptr_iter, remote_interface_type, remote_type>;
      using citer  = remote_iterator<rptr_citer, remote_interface_type, const remote_type>;
      using riter  = std::reverse_iterator<iter>;
      using criter = std::reverse_iterator<citer>;
      using ref    = Remote&;
      using cref   = const Remote&;
      
    protected:
      
      using init_list = std::initializer_list<std::reference_wrapper<remote_interface_type>>;

    public:

      using base::base;
      
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
  
      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      tracker_common (gch::tag::bind_t, remote_interface_type& remote) = delete;
  
      //! enabled for trackers
      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      tracker_common (gch::tag::bind_t, remote_interface_type& remote)
      {
        base::base_bind (remote);
      }
    
    public:
  
      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      tracker_common (gch::tag::bind_t, init_list init) = delete;
      
      //! enabled for trackers
      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      tracker_common (gch::tag::bind_t, init_list init)
      {
        base::base_bind (init.begin (), init.end ());
      }
      
      // imported functions
      using base::clear;
      using base::wipe;
      using base::num_reporters;
      
      GCH_NODISCARD iter   begin         (void)       noexcept { return base::rptrs_begin ();   }
      GCH_NODISCARD citer  begin         (void) const noexcept { return base::rptrs_begin ();   }
      GCH_NODISCARD citer  cbegin        (void) const noexcept { return base::rptrs_cbegin ();  }
      
      GCH_NODISCARD iter   end           (void)       noexcept { return base::rptrs_end ();     }
      GCH_NODISCARD citer  end           (void) const noexcept { return base::rptrs_end ();     }
      GCH_NODISCARD citer  cend          (void) const noexcept { return base::rptrs_cend ();    }
      
      GCH_NODISCARD riter  rbegin        (void)       noexcept { return base::rptrs_rbegin ();  }
      GCH_NODISCARD criter rbegin        (void) const noexcept { return base::rptrs_rbegin ();  }
      GCH_NODISCARD criter crbegin       (void) const noexcept { return base::rptrs_crbegin (); }
      
      GCH_NODISCARD riter  rend          (void)       noexcept { return base::rptrs_rend ();    }
      GCH_NODISCARD criter rend          (void) const noexcept { return base::rptrs_rend ();    }
      GCH_NODISCARD criter crend         (void) const noexcept { return base::rptrs_crend ();   }
      
      GCH_NODISCARD ref    front         (void)                { return *begin ();             }
      GCH_NODISCARD cref   front         (void) const          { return *begin ();             }
      
      GCH_NODISCARD ref    back          (void)                { return *--end ();             }
      GCH_NODISCARD cref   back          (void) const          { return *--end ();             }
  
      GCH_NODISCARD bool   has_reporters (void) const noexcept { return ! base::rptrs_empty (); }
  
      GCH_CPP17_CONSTEXPR std::size_t get_offset (citer pos) const noexcept
      {
        return base::base_get_offset (pos);
      }
      
      //! transfers all bindings from src; no overwriting
      template <typename T>
      GCH_CPP14_CONSTEXPR iter transfer_bindings (T&& src, citer pos) noexcept
      {
        return base::base_transfer_bindings (src, rptr_citer (pos));
      }
      
      template <typename T>
      GCH_CPP14_CONSTEXPR iter transfer_bindings (T&& src) noexcept
      {
        return transfer_bindings (src, cend ());
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
      iter bind (citer first, citer last) = delete;
  
      //! enabled for trackers
      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      iter bind (citer first, citer last)
      {
        return base::template base_bind<rptr_iter> (first, last);
      }
  
      //! disabled for reporters
      template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
      iter bind (remote_interface_type& r) = delete;
  
      //! enabled for trackers
      template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
      iter bind (remote_interface_type& r)
      { 
        return base::base_bind (r);
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
      constexpr bool has_parent (void) const noexcept
      {
        return true;
      }
  
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
      nonintrusive_common& operator= (const nonintrusive_common&)     { return *this; }
      nonintrusive_common& operator= (nonintrusive_common&&) noexcept { return *this; }
      ~nonintrusive_common           (void)                           = default;
  
      void swap (nonintrusive_common& other) = delete;
      
      constexpr nonintrusive_common (parent_type& parent)
        : m_parent (parent)
      { }
      
      GCH_NODISCARD
      constexpr bool has_parent (void) const noexcept
      {
        return true;
      }
  
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
  
  template <typename Derived, typename Remote, typename RemoteTag>
  class reporter<Derived, Remote, RemoteTag, tag::intrusive>
    : private detail::reporter_common<Derived, Remote,
                                      tag::reporter::intrusive, RemoteTag>,
      public detail::intrusive_common<Derived>
  {
    
    using base = detail::reporter_common<Derived, Remote,
                                         tag::reporter::intrusive, RemoteTag>;
    using access_base = detail::intrusive_common<Derived>;
  public:
    using local_interface_tag   = tag::reporter::intrusive;
    using remote_interface_tag  = RemoteTag;
    
    using local_interface_type  = reporter;
    using remote_interface_type = typename base::remote_interface_type;
    
    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;
    
    using derived_type          = Derived;
    using remote_type           = Remote;
    
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
    
  }; // reporter
  
  /////////////////////////
  // tracker (intrusive) //
  /////////////////////////
  
  template <typename Derived, typename Remote, typename RemoteTag>
  class tracker<Derived, Remote, RemoteTag, tag::intrusive>
    : private detail::tracker_common<Derived, Remote,
                                     tag::tracker::intrusive, RemoteTag>,
      public detail::intrusive_common<Derived>
  {
    using base = detail::tracker_common<Derived, Remote,
                                        tag::tracker::intrusive, RemoteTag>;
    using access_base = detail::intrusive_common<Derived>;
  public:
    using local_interface_tag   = tag::tracker::intrusive;
    using remote_interface_tag  = RemoteTag;
  
    using local_interface_type  = tracker;
    using remote_interface_type = typename base::remote_interface_type;
  
    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;
  
    using derived_type          = Derived;
    using remote_type           = Remote;
    
    using iter                  = typename base::iter;
    using citer                 = typename base::citer;
    using riter                 = typename base::riter;
    using criter                = typename base::criter;
    using ref                   = typename base::ref;
    using cref                  = typename base::cref;
  
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

    template <typename ...Args>
    remote_interface_type create (Args... args)
    {
      return remote_interface_type (*this, std::forward<Args> (args)...);
    }
  }; // tracker
  
  /////////////////////////////
  // reporter (nonintrusive) //
  /////////////////////////////
  
  template <typename Parent, typename Remote, typename RemoteTag>
  class reporter<Parent, Remote, RemoteTag, tag::nonintrusive>
    : public detail::reporter_common<Parent, Remote,
                                     tag::reporter::nonintrusive, RemoteTag>,
      public detail::nonintrusive_common<Parent>
  {
    using base = detail::reporter_common<Parent, Remote,
                                         tag::reporter::nonintrusive, RemoteTag>;
    using access_base = detail::nonintrusive_common<Parent>;
  public:
    using local_interface_tag   = tag::reporter::nonintrusive;
    using remote_interface_tag  = RemoteTag;
  
    using local_interface_type  = reporter;
    using remote_interface_type = typename base::remote_interface_type;
    
    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;
  
    using parent_type           = Parent;
    using remote_type           = Remote;
    
    template <typename, typename, typename>
    friend class remote_iterator;
    
    friend remote_common_type;
    friend remote_interface_type;
    
    using base::base;
    reporter            (void)                = default;
    reporter            (const reporter&)     = default;
    reporter            (reporter&&) noexcept = default;
    reporter& operator= (const reporter&)     = default;
    reporter& operator= (reporter&&) noexcept = default;
    ~reporter           (void)                = default;

    reporter (parent_type& parent, remote_interface_type& remote)
      : base        (tag::bind, remote),
        access_base (parent)
    { }

    reporter (const reporter& other, parent_type& new_parent)
      : base        (other),
        access_base (new_parent)
    { }

    reporter (reporter&& other, parent_type& new_parent) noexcept
      : base        (std::move (other)),
        access_base (new_parent)
    { }

    void swap (reporter& other)
    {
      base::swap (other);
    }
  
    reporter& rebind (remote_interface_type& new_remote)
    {
      base::rebind (new_remote);
      return *this;
    }
  }; // reporter
  
  ////////////////////////////
  // tracker (nonintrusive) //
  ////////////////////////////
  
  template <typename Parent, typename Remote, typename RemoteTag>
  class tracker<Parent, Remote, RemoteTag, tag::nonintrusive>
    : public detail::tracker_common<Parent, Remote,
                                    tag::tracker::nonintrusive, RemoteTag>,
      public detail::nonintrusive_common<Parent>
  {
    using base = detail::tracker_common<Parent, Remote,
                                        tag::tracker::nonintrusive, RemoteTag>;
    using access_base = detail::nonintrusive_common<Parent>;
    
    using init_list = typename base::init_list;
    
  public:
    using local_tag   = tag::tracker::nonintrusive;
    using remote_tag  = RemoteTag;
  
    using local_interface_type  = tracker;
    using remote_interface_type = typename base::remote_interface_type;
  
    using local_common_type     = base;
    using remote_common_type    = typename remote_interface_type::local_common_type;
  
    using parent_type           = Parent;
    using remote_type           = Remote;
    
    using iter                  = typename base::iter;
    using citer                 = typename base::citer;
    using riter                 = typename base::riter;
    using criter                = typename base::criter;
    using ref                   = typename base::ref;
    using cref                  = typename base::cref;
    
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
  
    template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
    tracker (const tracker& other, parent_type& new_parent) = delete;
  
    template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
    tracker (const tracker& other, parent_type& new_parent)
      : base        (other),
        access_base (new_parent)
    { }
  
    tracker (tracker&& other, parent_type& new_parent) noexcept
      : base        (std::move (other)),
        access_base (new_parent)
    { }
  
    void swap (tracker& other) noexcept
    {
      base::swap (other);
    }
  
    explicit tracker (parent_type& parent)
      : access_base (parent)
    { }
  
    template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
    tracker (parent_type& parent, remote_interface_type& remote) = delete;
  
    template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
    tracker (parent_type& parent, remote_interface_type& remote)
      : base        (tag::bind, remote),
        access_base (parent)
    { }
  
    template <typename Tag = remote_tag, tag::enable_if_reporter_t<Tag, bool> = false>
    tracker (parent_type& parent, init_list init) = delete;
  
    template <typename Tag = remote_tag, tag::enable_if_tracker_t<Tag, bool> = true>
    tracker (parent_type& parent, init_list init)
      : base        (tag::bind, init),
        access_base (parent)
    { }
    
  }; // tracker
  
  //! standalone reporter (no parent)
  template <typename RemoteParent, typename RemoteTag>
  class reporter<tag::reporter, RemoteParent, RemoteTag, tag::nonintrusive>
    : public reporter<reporter<tag::reporter, RemoteParent, RemoteTag, tag::nonintrusive>,
                     RemoteParent, RemoteTag, tag::intrusive>
  { };
  
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
