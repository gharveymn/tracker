/*

Copyright (C) 2019 Gene Harvey

This file is part of Octave.

Octave is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Octave is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Octave; see the file COPYING.  If not, see
<https://www.gnu.org/licenses/>.

*/

#if ! defined (octave_tracker_h)
#define octave_tracker_h 1

//! I am well aware this is needlessly complex and doesn't provide all 
//! the functionality desired. I'll refactor at some point.

//#include "octave-config.h"
#include <plf_list.h>

namespace octave
{

  template <typename LocalParent>
  struct reporter_orphan_hook
  {
    void operator() (const LocalParent*) const noexcept { }
  };

  template <typename Reporter, typename RemoteParent>
  class tracker_base;

  template <typename Reporter,
    typename Parent = tracker_base<Reporter, Reporter>,
    typename RemoteParent = Reporter>
  class tracker;

  template <typename Reporter, typename LocalParent>
  class reporter_base;

  template <typename Derived, typename RemoteParent = tracker<Derived>,
    typename LocalParent = Derived, typename Hook = reporter_orphan_hook<LocalParent>>
  class intrusive_reporter;

  template <typename LocalParent, typename RemoteParent = tracker<LocalParent>,
    typename Hook = reporter_orphan_hook<LocalParent>>
  class reporter;

  template <typename LocalParent, typename RemoteParent = LocalParent,
    typename LocalHook = reporter_orphan_hook<LocalParent>,
    typename RemoteHook = reporter_orphan_hook<RemoteParent>>
  class multireporter;

  template <typename Reporter>
  class reporter_ptr
  {
  public:

    using reporter_type  = Reporter;

    using value_type        = reporter_type;
    using pointer           = reporter_type *;
    using const_pointer     = const reporter_type *;
    using reference         = reporter_type&;
    using const_reference   = const reporter_type&;

    explicit constexpr reporter_ptr (pointer ptr) noexcept
      : m_ptr (ptr)
    { }

    reporter_ptr (const reporter_ptr&)                      = delete;
    reporter_ptr (reporter_ptr&& other) noexcept            = delete;
    reporter_ptr& operator= (const reporter_ptr&)           = delete;
    reporter_ptr& operator= (reporter_ptr&& other) noexcept = delete;

    ~reporter_ptr (void) noexcept = default;
    
    void orphan_remote (void) noexcept 
    {
      if (m_ptr != nullptr)
        m_ptr->orphan ();
    }

    constexpr pointer get (void) const noexcept
    {
      return static_cast<pointer> (m_ptr);
    }

    constexpr reference operator* (void) const noexcept
    {
      return *get ();
    }

    constexpr pointer operator-> (void) const noexcept
    {
      return get ();
    }

    friend bool operator== (reporter_ptr r, pointer p) noexcept
    {
      return r.get () == p;
    }

    friend bool operator== (pointer p, reporter_ptr r) noexcept
    {
      return p == r.get ();
    }

    friend bool operator!= (reporter_ptr r, pointer p) noexcept
    {
      return r.get () != p;
    }

    friend bool operator!= (pointer p, reporter_ptr r) noexcept
    {
      return p != r.get ();
    }
    
    template <typename T>
    void reset_remote_tracker (T *ptr) noexcept
    {
      m_ptr->reset_tracker (ptr);
    }

    void reset (reporter_type *ptr = nullptr) noexcept
    {
      m_ptr = ptr;
    }

  private:

    reporter_type *m_ptr = nullptr;

  };

  template <typename Reporter, typename RemoteParent>
  class tracker_base
  {
  public:

    using reporter_type      = Reporter;
    using remote_parent_type = RemoteParent;

    friend reporter_base<reporter_type, remote_parent_type>;

  protected:

    using element_type    = reporter_ptr<reporter_type>;
    using reporter_list   = plf::list<element_type>;
    using internal_iter   = typename reporter_list::iterator;
    using internal_citer  = typename reporter_list::const_iterator;
    using internal_riter  = typename reporter_list::reverse_iterator;
    using internal_criter = typename reporter_list::const_reverse_iterator;
    using internal_ref    = typename reporter_list::reference;
    using internal_cref   = typename reporter_list::const_reference;

  public:

    // pretend like reporter_ptr doesn't exist
    struct external_iterator
    {
      
      using internal_type     = typename internal_citer::value_type;

      static_assert (std::is_same<internal_type,
                                  reporter_ptr<reporter_type>>::value,
                     "internal_type was unexpected type");

      using difference_type   = typename internal_citer::difference_type;
      using iterator_category = std::bidirectional_iterator_tag;
      using value_type        = remote_parent_type;
      using pointer           = remote_parent_type*;
      using const_pointer     = const remote_parent_type*;
      using reference         = remote_parent_type&;
      using const_reference   = const remote_parent_type&;

      external_iterator (internal_citer cit)
        : m_citer (cit)
      { }

      external_iterator (internal_iter it)
        : external_iterator (internal_citer (it))
      { }
      
      operator internal_citer (void) const noexcept
      {
        return m_citer;
      }
      
      external_iterator (const external_iterator&)     = default;
      external_iterator (external_iterator&&) noexcept = default;

      // ref-qualified to prevent assignment to rvalues
      external_iterator& operator= (const external_iterator& other) &
      {
        if (&other != this)
          m_citer = other.m_citer;
        return *this;
      }

      // ref-qualified to prevent assignment to rvalues
      external_iterator& operator= (external_iterator&& other) & noexcept
      {
        if (&other != this)
          m_citer = std::move (other.m_citer);
        return *this;
      }

      constexpr pointer get (void) const noexcept
      {
        return (*m_citer)->get_parent ();
      }

      constexpr reference operator* (void) const noexcept
      {
        return *get ();
      }

      constexpr pointer operator-> (void) const noexcept
      {
        return get ();
      }

      external_iterator& operator++ (void) noexcept
      {
        ++m_citer;
        return *this;
      }

      external_iterator operator++ (int) noexcept
      {
        const external_iterator tmp = *this;
        ++m_citer;
        return tmp;
      }

      external_iterator& operator-- (void) noexcept
      {
        --m_citer;
        return *this;
      }

      external_iterator operator-- (int) noexcept
      {
        const external_iterator tmp = *this;
        --m_citer;
        return tmp;
      }

      friend bool operator== (const external_iterator& x, 
                              const external_iterator& y) noexcept
      {
        return x.m_citer == y.m_citer;
      }

      friend bool operator!= (const external_iterator& x, 
                              const external_iterator& y) noexcept
      {
        return x.m_citer != y.m_citer;
      }

      friend class tracker_base;

    private:
      internal_citer m_citer;
    };

    using iter   = external_iterator;
    using citer  = external_iterator;
    using riter  = std::reverse_iterator<iter>;
    using criter = std::reverse_iterator<citer>;
    using ref    = typename external_iterator::reference;
    using cref   = typename external_iterator::const_reference;

    tracker_base (void) = default;

    tracker_base (const tracker_base&) = delete;

    tracker_base (tracker_base&& other) noexcept
    { 
      transfer_from (std::move (other));
    }

    tracker_base& operator= (const tracker_base&) = delete;

    tracker_base& operator= (tracker_base&& other) noexcept
    {
      transfer_from (std::move (other));
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
          for (internal_ref p : m_reporters)
            p.orphan_remote ();
          m_reporters.clear ();
        }
    }
    
    void swap (tracker_base& other) noexcept 
    {
      // kinda expensive
      this->repoint_reporters (&other);
      other.repoint_reporters (this);
      m_reporters.swap (other.m_reporters);
    }

    std::size_t num_reporters (void) const noexcept
    {
      return m_reporters.size ();
    }

    void transfer_from (tracker_base&& src, citer pos) noexcept
    {
      src.repoint_reporters (this);
      return m_reporters.splice (pos.m_citer, src.m_reporters);
    }

    void transfer_from (tracker_base& src, citer pos)
    {
      return transfer_from (std::move (src), pos);
    }

    template <typename T>
    void transfer_from (T&& src)
    {
      return transfer_from (std::forward<T> (src), m_reporters.cend ());
    }
    
    // unsafe!
    void clear (void) noexcept
    {
      m_reporters.clear ();
    }
    
  protected:

    internal_iter   internal_begin   (void)       noexcept { return m_reporters.begin (); }
    internal_citer  internal_begin   (void) const noexcept { return m_reporters.begin (); }
    internal_citer  internal_cbegin  (void) const noexcept { return m_reporters.cbegin (); }

    internal_iter   internal_end     (void)       noexcept { return m_reporters.end (); }
    internal_citer  internal_end     (void) const noexcept { return m_reporters.end (); }
    internal_citer  internal_cend    (void) const noexcept { return m_reporters.cend (); }

    internal_riter  internal_rbegin  (void)       noexcept { return m_reporters.rbegin (); }
    internal_criter internal_rbegin  (void) const noexcept { return m_reporters.rbegin (); }
    internal_criter internal_crbegin (void) const noexcept { return m_reporters.crbegin (); }

    internal_riter  internal_rend    (void)       noexcept { return m_reporters.rend (); }
    internal_criter internal_rend    (void) const noexcept { return m_reporters.rend (); }
    internal_criter internal_crend   (void) const noexcept { return m_reporters.crend (); }

    internal_ref    internal_front   (void)                { return m_reporters.front (); }
    internal_cref   internal_front   (void) const          { return m_reporters.front (); }

    internal_ref    internal_back    (void)                { return m_reporters.back (); }
    internal_cref   internal_back    (void) const          { return m_reporters.back (); }

  public:
    
    iter   begin   (void)       noexcept { return m_reporters.begin (); }
    citer  begin   (void) const noexcept { return m_reporters.begin (); }
    citer  cbegin  (void) const noexcept { return m_reporters.cbegin (); }

    iter   end     (void)       noexcept { return m_reporters.end (); }
    citer  end     (void) const noexcept { return m_reporters.end (); }
    citer  cend    (void) const noexcept { return m_reporters.cend (); }

    riter  rbegin  (void)       noexcept { return m_reporters.rbegin (); }
    criter rbegin  (void) const noexcept { return m_reporters.rbegin (); }
    criter crbegin (void) const noexcept { return m_reporters.crbegin (); }

    riter  rend    (void)       noexcept { return m_reporters.rend (); }
    criter rend    (void) const noexcept { return m_reporters.rend (); }
    criter crend   (void) const noexcept { return m_reporters.crend (); }

    ref    front   (void)                { return m_reporters.front (); }
    cref   front   (void) const          { return m_reporters.front (); }

    ref    back    (void)                { return m_reporters.back (); }
    cref   back    (void) const          { return m_reporters.back (); }

    void   reverse (void)       noexcept { m_reporters.reverse (); }

    bool has_reporters (void) const noexcept
    {
      return ! m_reporters.empty ();
    }

    template <typename ...Args>
    reporter_type create (Args... args)
    {
      return { this, std::forward<Args> (args)... };
    }

  protected:

    // safe, may throw
    template <typename ...Args>
    internal_iter track (Args&&... args)
    {
      return m_reporters.emplace (m_reporters.end (),
                                  std::forward<Args> (args)...);
    }

    // safe
    internal_iter untrack (internal_iter pos)
    {
      pos->orphan_remote ();
      return m_reporters.erase (pos);
    }

    // safe
    internal_iter untrack (internal_iter first, const internal_iter last)
    {
      while (first != last)
        first = untrack (first);
      return last;
    }

    // unsafe!
    internal_iter erase (internal_citer cit) noexcept
    {
      return m_reporters.erase (cit);
    }

    // unsafe!
    void repoint_reporters (tracker_base *ptr)
    {
      for (internal_ref rptr : m_reporters)
        rptr.reset_remote_tracker (ptr);
    }
    
//    reporter_list copy_reporters (void) const
//    {
//      return m_reporters;
//    }
//    
//    void replace_reporters (reporter_list&& rep)
//    {
//      for (internal_ref p : m_reporters)
//        p.orphan_remote ();
//      m_reporters = std::move (rep);
//    }

  private:
    
    reporter_list m_reporters;

  };

  template <typename Reporter, typename LocalParent, typename RemoteParent>
  class tracker : public tracker_base<Reporter, RemoteParent>
  {
  public:

    using reporter_type      = Reporter;
    using local_parent_type  = LocalParent;
    using remote_parent_type = RemoteParent;

    using base_type       = tracker_base<reporter_type, remote_parent_type>;
    
    explicit tracker (void) = default;

    explicit tracker (local_parent_type& parent)
      : m_parent (&parent)
    { }

    tracker (const tracker&)                                = delete;
    tracker (local_parent_type& new_parent, const tracker& other) = delete;

    tracker (tracker&& other) noexcept
      : base_type (std::move (other))
    { }

    tracker (local_parent_type& new_parent, tracker&& other) noexcept
      : base_type (std::move (other)),
        m_parent (&new_parent)
    { }

    tracker& operator= (const tracker& other) = delete;

    tracker& operator= (tracker&& other) noexcept
    {
      base_type::operator= (std::move (other));
      return *this;
    }
    
    void swap (tracker& other) noexcept 
    {
      base_type::swap (other);
      using std::swap;
      swap (m_parent, other.m_parent); // caution!
    }

    constexpr local_parent_type& operator* (void) const noexcept
    {
      return *m_parent;
    }

    constexpr local_parent_type* operator-> (void) const noexcept
    {
      return m_parent;
    }

    constexpr bool has_parent (void) const noexcept
    {
      return m_parent != nullptr;
    }

    constexpr local_parent_type *get_parent (void) const noexcept
    {
      return m_parent;
    }

  private:
    local_parent_type *m_parent = nullptr;

  };

  template <typename Reporter>
  class tracker<Reporter, tracker_base<Reporter, Reporter>, Reporter>
    : public tracker_base<Reporter, Reporter>
  {
  public:

    using reporter_type      = Reporter;
    using local_parent_type  = tracker_base<Reporter, Reporter>;
    using remote_parent_type = Reporter;

    using base_type       = tracker_base<reporter_type, remote_parent_type>;

    tracker (void) = default;

    tracker (const tracker& other)
      : base_type (other)
    { }

    tracker (tracker&& other) noexcept
      : base_type (std::move (other))
    { }

    tracker& operator= (const tracker& other)
    {
      if(&other != this)
        base_type::operator= (other);
      return *this;
    }

    tracker& operator= (tracker&& other) noexcept
    {
      base_type::operator= (std::move (other));
      return *this;
    }
    
    void swap (tracker& other) noexcept 
    {
      base_type::swap (other);
    }

    constexpr bool has_parent (void) const noexcept
    {
      return true;
    }

    constexpr local_parent_type *get_parent (void) const noexcept
    {
      return static_cast<local_parent_type*> (this);
    }

  };

  template <typename Reporter, typename LocalParent>
  class reporter_base
  {
  public:

    using reporter_type  = Reporter;
    using local_parent_type = LocalParent;

    using tracker_base_type  = tracker_base<reporter_type, local_parent_type>;
    using self_ptr_type      = reporter_ptr<reporter_type>;
    using ext_citer_type     = typename tracker_base_type::citer;
    using self_iter_type     = typename tracker_base_type::internal_iter;

    friend void tracker_base_type::transfer_from (tracker_base_type&& src, 
                                             ext_citer_type pos) noexcept;

    reporter_base (void) noexcept = default;

    explicit reporter_base (tracker_base_type& tkr)
      : m_tracker (&tkr),
        m_self_iter (m_tracker->track (downcast (this)))
    { }

    reporter_base (const reporter_base& other)
      : m_tracker (other.m_tracker),
        m_self_iter (has_tracker () ? m_tracker->track (downcast (this))
                                    : self_iter_type { })
    { }

    reporter_base (reporter_base&& other) noexcept
      : m_tracker (other.m_tracker),
        m_self_iter (other.m_self_iter)
    {
      if (has_tracker ())
        {
          m_self_iter->reset (downcast (this));
          other.m_tracker = nullptr; // other no longer owns an iterator
        }
    }

    reporter_base& operator= (const reporter_base& other)
    {
      // copy-and-swap
      if (other.m_tracker != m_tracker)
        reporter_base (other).swap (*this);
      return *this;
    }

    reporter_base& operator= (reporter_base&& other) noexcept
    {
      if (&other != this)
        {
          if (has_tracker ())
            m_tracker->erase (m_self_iter);

          m_tracker   = other.m_tracker;
          m_self_iter = other.m_self_iter;

          if (has_tracker ())
            {
              m_self_iter->reset (downcast (this));
              other.m_tracker = nullptr; // other no longer owns an iterator
            }
        }
      return *this;
    }

    ~reporter_base (void) noexcept
    {
      if (has_tracker ())
        m_tracker->erase (m_self_iter);
    }
    
    void swap (reporter_base& other) noexcept 
    {
      if (other.m_tracker != m_tracker)
        {
          using std::swap;
          swap (m_tracker, other.m_tracker);
          swap (m_self_iter, other.m_self_iter);
          
          if (has_tracker ())
            m_self_iter->reset (downcast (this));
          
          if (other.has_tracker ())
            other.m_self_iter->reset (downcast (&other));
        }
    }

    void fast_swap (reporter_base& other) noexcept
    {
      // where m_tracker is known to be non-null
      if (other.m_tracker != m_tracker)
        {
          using std::swap;
          swap (m_tracker, other.m_tracker);
          swap (m_self_iter, other.m_self_iter);

          if (has_tracker ())
            m_self_iter->reset (downcast (this));

          other.m_self_iter->reset (downcast (&other));
        }
    }

    constexpr bool has_tracker (void) const noexcept
    {
      return m_tracker != nullptr;
    }

    //! only use after using has_parent
    constexpr tracker_base_type *get_base_tracker (void) const noexcept
    {
      return m_tracker;
    }

    std::size_t get_position (void) const noexcept
    {
      if (! has_tracker ())
        return 0;
      return std::distance (m_tracker->m_reporters.begin (), m_self_iter);
    }
    
    reporter_base& rebind (tracker_base_type& tkr)
    {
      // copy-and-swap
      if (&tkr != m_tracker)
        reporter_base (tkr).fast_swap (*this);
      return *this;
    }

    void reset_tracker (tracker_base_type *ptr) noexcept
    {
      m_tracker = ptr;
    }
    
  protected:

    void reset_iterator (self_iter_type it) noexcept
    {
      m_self_iter = it;
    }

    void reset (tracker_base_type *ptr = nullptr,
                self_iter_type it = self_iter_type { }) noexcept
    {
      m_tracker   = ptr;
      m_self_iter = it;
    }

  private:

    static constexpr const reporter_type * downcast (const reporter_base *r)
    {
      return static_cast<const reporter_type *> (r);
    }

    static constexpr reporter_type * downcast (reporter_base *r)
    {
      return static_cast<reporter_type *> (r);
    }

    tracker_base_type *m_tracker = nullptr;
    self_iter_type m_self_iter   = self_iter_type { };

  };

  template <typename Derived, typename RemoteParent, typename LocalParent, typename Hook>
  class intrusive_reporter : public reporter_base<Derived, LocalParent>
  {

  public:

    using derived_type       = Derived;
    using remote_parent_type = RemoteParent;
    using local_parent_type  = LocalParent;
    using hook_type          = Hook;
    using base_type          = reporter_base<derived_type, local_parent_type>;
    
    using tracker_type      = tracker<derived_type, remote_parent_type, 
                                      local_parent_type>;
    using tracker_base_type = typename tracker_type::base_type;

    static_assert (std::is_same<tracker_base_type,
                                tracker_base<derived_type, local_parent_type>>::value,
                   "tracker base has unexpected type");

    friend reporter_ptr<Derived>::~reporter_ptr<Derived> (void) noexcept;

    intrusive_reporter (void) noexcept = default;

    explicit intrusive_reporter (tracker_type& tkr)
      : base_type (tkr)
    { }

    explicit intrusive_reporter (tracker_type& tkr, const hook_type& hook)
      : base_type (tkr),
        m_orphan_hook (hook)
    { }

    explicit intrusive_reporter (tracker_type& tkr, hook_type&& hook)
      : base_type (tkr),
        m_orphan_hook (std::move (hook))
    { }

    intrusive_reporter (const intrusive_reporter& other)
      : base_type (other),
        m_orphan_hook (other.m_orphan_hook)
    { }

    intrusive_reporter (intrusive_reporter&& other) noexcept
      : base_type (std::move (other)),
        m_orphan_hook (std::move (other.m_orphan_hook))
    { }

    intrusive_reporter& operator= (const intrusive_reporter& other)
    {
      if (&other != this)
        {
          base_type::operator= (other);
          m_orphan_hook = other.m_orphan_hook;
        }
      return *this;
    }

    intrusive_reporter& operator= (intrusive_reporter&& other) noexcept
    {
      base_type::operator= (std::move (other));
      m_orphan_hook = std::move (other.m_orphan_hook);
      return *this;
    }

    void swap (intrusive_reporter& other)
    {
      base_type::swap (other);
      using std::swap;
      std::swap (m_orphan_hook, other.m_orphan_hook);
    }
    
    constexpr const local_parent_type *get_parent (void) const noexcept
    {
      return downcast (this);
    }

    local_parent_type *get_parent (void) noexcept
    {
      return downcast (this);
    }

    constexpr tracker_type *get_tracker (void) const noexcept
    {
      return static_cast<tracker_type *> (this->get_base_tracker ());
    }

    constexpr bool has_remote_parent (void) const noexcept
    {
      return this->has_tracker () && this->get_tracker ()->has_parent ();
    }

    constexpr remote_parent_type *get_remote_parent (void) const noexcept
    {
      return this->has_tracker () ? this->get_tracker ()->get_parent ()
                                  : nullptr;
    }

    void orphan (void)
    {
      base_type::reset_tracker (nullptr);
      m_orphan_hook (get_parent ());
    }

  private:
    
    static constexpr const derived_type * downcast (const intrusive_reporter *r)
    {
      return static_cast<const derived_type *> (r);
    }

    static constexpr derived_type * downcast (intrusive_reporter *r)
    {
      return static_cast<derived_type *> (r);
    }

    hook_type m_orphan_hook = hook_type { };
    
  };

  //! non-intrusive; for use as a class member
  template <typename LocalParent, typename RemoteParent, typename Hook>
  class reporter 
    : public reporter_base<reporter<LocalParent, RemoteParent, Hook>, 
                           LocalParent>
  {
  public:
    using local_parent_type  = LocalParent;
    using remote_parent_type = RemoteParent;
    using hook_type          = Hook;
    using base_type          = reporter_base<reporter, local_parent_type>;
    using tracker_type       = tracker<reporter, remote_parent_type, 
                                       local_parent_type>;

    static_assert (std::is_same<tracker_type,
                           tracker<reporter, remote_parent_type, 
                                   local_parent_type>>::value,
                   "tracker has unexpected type");
    
    reporter (void) noexcept = default;

    explicit reporter (tracker_type& tkr)
      : base_type (tkr)
    { }

    explicit reporter (local_parent_type& parent, tracker_type& tkr)
      : base_type (tkr),
        m_parent (&parent)
    { }

    explicit reporter (local_parent_type& parent, tracker_type& tkr, 
                       const hook_type& hook)
      : base_type (tkr),
        m_parent (&parent),
        m_orphan_hook (hook)
    { }

    explicit reporter (local_parent_type& parent, tracker_type& tkr, hook_type&& hook)
      : base_type (tkr),
        m_parent (&parent),
        m_orphan_hook (std::move (hook))
    { }

    explicit reporter (tracker_type& tkr, const hook_type& hook)
      : base_type (tkr),
        m_orphan_hook (hook)
    { }

    explicit reporter (tracker_type& tkr, hook_type&& hook)
      : base_type (tkr),
        m_orphan_hook (std::move (hook))
    { }

    reporter (const reporter&)            = delete;
    reporter (reporter&&) noexcept        = delete;

    reporter (local_parent_type& new_parent, const reporter& other)
      : base_type (other),
        m_parent (&new_parent),
        m_orphan_hook (other.m_orphan_hook)
    { }

    reporter (local_parent_type& new_parent, reporter&& other) noexcept
      : base_type (std::move (other)),
        m_parent (&new_parent),
        m_orphan_hook (std::move (other.m_orphan_hook))
    { }

    reporter& operator= (const reporter& other)
    {
      if (&other != this)
        {
          base_type::operator= (other);
          m_orphan_hook = other.m_orphan_hook;
        }
      return *this;
    }

    reporter& operator= (reporter&& other) noexcept
    {
      base_type::operator= (std::move (other));
      m_orphan_hook = std::move (other.m_orphan_hook);
      return *this;
    }

    void swap (reporter& other)
    {
      base_type::swap (other);
      using std::swap;
      swap (m_parent, other.m_parent);
      swap (m_orphan_hook, other.m_orphan_hook);
    }

    constexpr local_parent_type& get_child_ref (void) const noexcept
    {
      return *m_parent;
    }

    constexpr const local_parent_type *get_parent (void) const noexcept
    {
      return m_parent;
    }

    local_parent_type *get_parent (void) noexcept
    {
      return m_parent;
    }

    void orphan (void)
    {
      base_type::reset_tracker (nullptr);
      m_orphan_hook (get_parent ());
    }

  private:
    local_parent_type *m_parent = nullptr;
    hook_type m_orphan_hook     = hook_type { };

  };

  template <typename LocalParent, typename RemoteParent, typename LocalHook, typename RemoteHook>
  class reporter_ptr<multireporter<RemoteParent, LocalParent, RemoteHook, LocalHook>>
  {
  public:
    
    using local_parent_type = LocalParent;
    
    using local_type    = multireporter<LocalParent, RemoteParent, LocalHook, RemoteHook>;
    using reporter_type = multireporter<RemoteParent, LocalParent, RemoteHook, LocalHook>;
    
    using tracker_base_type = typename local_type::local_tracker_base_type;
    using self_iter_type = typename reporter_type::internal_iter;

    using value_type        = reporter_type;
    using pointer           = reporter_type *;
    using const_pointer     = const reporter_type *;
    using reference         = reporter_type&;
    using const_reference   = const reporter_type&;

    friend reporter_ptr<local_type>;
    
    explicit reporter_ptr (void) = default;

    explicit reporter_ptr (pointer ptr) noexcept
      : m_ptr (ptr)
    { }

    explicit reporter_ptr (pointer ptr, self_iter_type iter) noexcept
      : m_ptr (ptr),
        m_iter (iter)
    { }

    reporter_ptr (const reporter_ptr&)                      = delete;
    reporter_ptr (reporter_ptr&& other) noexcept            = delete;
    reporter_ptr& operator= (const reporter_ptr&)           = delete;
    reporter_ptr& operator= (reporter_ptr&& other) noexcept = delete;

    ~reporter_ptr (void) noexcept = default;

    void orphan_remote (void) noexcept
    {
      if (m_ptr != nullptr)
        m_ptr->orphan (m_iter);
    }

    constexpr pointer get (void) const noexcept
    {
      return static_cast<pointer> (m_ptr);
    }

    constexpr reference operator* (void) const noexcept
    {
      return *get ();
    }

    constexpr pointer operator-> (void) const noexcept
    {
      return get ();
    }

    friend bool operator== (reporter_ptr r, pointer p) noexcept
    {
      return r.get () == p;
    }

    friend bool operator== (pointer p, reporter_ptr r) noexcept
    {
      return p == r.get ();
    }

    friend bool operator!= (reporter_ptr r, pointer p) noexcept
    {
      return r.get () != p;
    }

    friend bool operator!= (pointer p, reporter_ptr r) noexcept
    {
      return p != r.get ();
    }

    void reset_remote_tracker (tracker_base_type *ptr) noexcept
    {
      m_iter->m_ptr = static_cast<local_type *> (ptr);
    }

    void reset (reporter_type *ptr = nullptr) noexcept
    {
      m_ptr = ptr;
    }

    void reset (reporter_type *ptr, self_iter_type iter) noexcept
    {
      m_ptr  = ptr;
      m_iter = iter;
    }

  private:

    reporter_type *m_ptr  = nullptr;
    self_iter_type m_iter = self_iter_type { };

  };

  template <typename LocalParent, typename RemoteParent, typename LocalHook, typename RemoteHook>
  class multireporter
    : public tracker<multireporter<RemoteParent, LocalParent, RemoteHook, LocalHook>,
                     LocalParent, RemoteParent>
  {
  public:
    using local_parent_type = LocalParent;
    using remote_parent_type = RemoteParent;
    using hook_type     = LocalHook;

    friend class multireporter<RemoteParent, LocalParent, RemoteHook,
                               LocalHook>;

    using remote_type   = multireporter<RemoteParent, LocalParent, RemoteHook, LocalHook>;
    using local_tracker_type = tracker<remote_type, local_parent_type, remote_parent_type>;
    using local_tracker_base_type = typename local_tracker_type::base_type;
    
    using internal_iter  = typename local_tracker_type::internal_iter;
    using internal_citer = typename local_tracker_type::internal_citer;
    
    using iter  = typename local_tracker_type::iter;
    using citer = typename local_tracker_type::citer;
    
    using remote_tracker_type      = typename remote_type::local_tracker_type;
    using remote_tracker_base_type = typename remote_tracker_type::base_type;
    
    using self_iter_type = typename remote_type::internal_iter;

    template <typename Compare, typename Head, typename ...Tail>
    struct all_same
      : std::integral_constant<bool, std::is_same<Compare, Head>::value
                                     && all_same<Compare, Tail...>::value>
    { };

    template <typename Compare, typename T>
    struct all_same<Compare, T> : std::is_same<Compare, T>
    { };
    
    template <typename ...Args>
    typename std::enable_if<all_same<remote_type, Args...>::value>::type 
    bind (remote_type& r, Args&... args)
    {
      internal_bind (r);
      bind (args...);
    }

    citer
    bind (remote_type& r)
    {
      return internal_bind (r);
    }

    multireporter (void) = default;

    explicit multireporter (local_parent_type& cld)
      : local_tracker_type (cld)
    { }

    explicit multireporter (const hook_type& hook)
      : m_orphan_hook (hook)
    { }

    explicit multireporter (hook_type&& hook)
      : m_orphan_hook (std::move (hook))
    { }

    explicit multireporter (local_parent_type& parent, const hook_type& hook)
      : local_tracker_type (parent),
        m_orphan_hook (hook)
    { }
    
    explicit multireporter (local_parent_type& parent, hook_type&& hook)
      : local_tracker_type (parent),
        m_orphan_hook (std::move (hook))
    { }

    multireporter (const multireporter& other)
      : local_tracker_type (),
        m_orphan_hook (other.m_orphan_hook)
    {
      overwrite_reporters (other);
    }

    multireporter (local_parent_type& parent, const multireporter& other)
      : local_tracker_type (parent),
        m_orphan_hook (other.m_orphan_hook)
    {
      overwrite_reporters (other);
    }

    multireporter (multireporter&& other) noexcept
      : local_tracker_type (std::move (other)),
        m_orphan_hook (std::move (other.m_orphan_hook))
    { }

    multireporter (local_parent_type& parent, multireporter&& other) noexcept
      : local_tracker_type (parent, std::move (other)),
        m_orphan_hook (std::move (other.m_orphan_hook))
    { }
    
    multireporter& operator= (const multireporter& other)
    {
      if (&other != this)
        {
          m_orphan_hook = other.m_orphan_hook;
          overwrite_reporters (other);
        }
      return *this;
    }

    multireporter& operator= (multireporter&& other) noexcept
    {
      local_tracker_type::operator= (std::move (other));
      m_orphan_hook = std::move (other.m_orphan_hook);
      return *this;
    }
    
    void swap (multireporter& other) noexcept
    {
      local_tracker_type::swap (other);
      using std::swap;
      swap (m_orphan_hook, other.m_orphan_hook);
    }
    
    citer copy_reporters (const multireporter& other)
    {
      return internal_copy_reporters (other);
    }

    citer overwrite_reporters (const multireporter& other)
    {
      return this->untrack (this->internal_begin (), 
                            internal_copy_reporters (other));
    }

    constexpr const local_parent_type* get_parent (void) const noexcept
    {
      return local_tracker_type::get_parent ();
    }

    local_parent_type* get_parent (void) noexcept
    {
      return local_tracker_type::get_parent ();
    }

    void orphan (citer it)
    {
      this->erase (it);
      m_orphan_hook (get_parent ());
    }
    
  private:

    internal_iter internal_copy_reporters (const multireporter& other)
    {
      if (other.has_reporters ())
        {
          internal_iter pivot = internal_bind (*other.internal_front ());
          try
            {
              for (internal_citer cit = ++other.internal_begin ();
                   cit != other.internal_end (); ++cit)
                {
                  internal_bind (**cit);
                }
            }
          catch (...)
            {
              this->untrack(pivot, this->internal_end ());
              throw;
            }
          return pivot;
        }
      return this->internal_begin ();
    }
    
    internal_iter internal_bind (remote_type& r)
    {
      internal_iter it = this->track ();
      try
        {
          it->reset (&r, r.track (this, it));
        }
      catch (...)
        {
          // the node reporter_ptr holds no information, so it is safe to erase
          this->erase (it);
          throw;
        }
      return it;
    }

    hook_type m_orphan_hook = hook_type { };

  };

  template <typename ...Ts>
  void bind (multireporter<Ts...>& l, 
             typename multireporter<Ts...>::remote_type& r)
  {
    l.bind (r);
  }
  
}

#endif
