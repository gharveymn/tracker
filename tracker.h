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

//! standalone
#if ! defined (octave_tracker_h)
#define octave_tracker_h 1

#include "octave-config.h"
#include <plf_list.h>

namespace octave
{

  template <typename Child>
  struct reporter_orphan_hook
  {
    constexpr void operator() (const Child*) const noexcept { }
  };

  template <typename Reporter, typename Child>
  class tracker_base;

  template <typename Reporter,
    typename Parent = tracker_base<Reporter, Reporter>,
    typename Child = Reporter>
  class tracker;

  template <typename Reporter, typename Child>
  class reporter_base;

  template <typename Derived, typename Parent = tracker<Derived>,
    typename Child = Derived, typename Hook = reporter_orphan_hook<Child>>
  class intrusive_reporter;

  template <typename Child, typename Parent = tracker<Child>,
    typename Hook = reporter_orphan_hook<Child>>
  class reporter;

  template <typename Child, typename Parent = Child,
    typename HookLocal = reporter_orphan_hook<Child>,
    typename HookRemote = reporter_orphan_hook<Parent>>
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

    reporter_type * m_ptr = nullptr;

  };

  template <typename Reporter, typename Child>
  class tracker_base
  {
  public:

    using reporter_type = Reporter;
    using child_type    = Child;

    friend reporter_base<reporter_type, child_type>;

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
      using value_type        = child_type;
      using pointer           = child_type *;
      using const_pointer     = const child_type *;
      using reference         = child_type&;
      using const_reference   = const child_type&;

      external_iterator (internal_citer cit)
        : m_citer (cit)
      { }

      external_iterator (internal_iter it)
        : external_iterator (internal_citer (it))
      { }
      
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
        return (*m_citer)->get_child_ptr ();
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
      m_reporters.swap (other.m_reporters);
      for (internal_ref c_ptr : m_reporters)
        c_ptr.reset_remote_tracker (this);
      for (internal_ref c_ptr : other.m_reporters)
        c_ptr.reset_remote_tracker (&other);
    }

    std::size_t num_reporters (void) const noexcept
    {
      return m_reporters.size ();
    }

    void transfer_from (tracker_base&& src, citer pos) noexcept
    {
      for (internal_ref c_ptr : src.m_reporters)
        c_ptr.reset_remote_tracker (this);
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

  template <typename Reporter, typename Parent, typename Child>
  class tracker : public tracker_base<Reporter, Child>
  {
  public:

    using reporter_type   = Reporter;
    using parent_type     = Parent;
    using child_type      = Child;

    using base_type       = tracker_base<reporter_type, child_type>;
    
    explicit tracker (void) = default;

    explicit tracker (parent_type& parent)
      : m_parent (&parent)
    { }

    tracker (const tracker&)                                = delete;
    tracker (parent_type& new_parent, const tracker& other) = delete;

    tracker (tracker&& other) noexcept
      : base_type (std::move (other))
    { }

    tracker (parent_type& new_parent, tracker&& other) noexcept
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
      std::swap (m_parent, other.m_parent); // caution!
    }

    constexpr parent_type& operator* (void) const noexcept
    {
      return *m_parent;
    }

    constexpr parent_type * operator-> (void) const noexcept
    {
      return m_parent;
    }

    constexpr bool has_parent (void) const noexcept
    {
      return m_parent != nullptr;
    }

    constexpr parent_type * get_parent (void) const noexcept
    {
      return m_parent;
    }

  private:

    parent_type *m_parent = nullptr;

  };

  template <typename Reporter>
  class tracker<Reporter, tracker_base<Reporter, Reporter>, Reporter>
    : public tracker_base<Reporter, Reporter>
  {
  public:

    using reporter_type   = Reporter;
    using parent_type     = tracker_base<Reporter, Reporter>;
    using child_type      = Reporter;

    using base_type       = tracker_base<reporter_type, child_type>;

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

    constexpr parent_type * get_parent (void) const noexcept
    {
      return static_cast<parent_type *> (this);
    }

  };

//  template <typename Child, typename Parent, typename Hook>
//  class tracker<reporter<Child, Parent, Hook>, Parent, reporter<Child, Parent, Hook>>
//    : public tracker<reporter<Child, Parent, Hook>, Parent, Child>
//  {
//  public:
//    using reporter_type = reporter<Child, Parent, Hook>;
//    using parent_type   = Parent;
//    using child_type    = Child;
//
//    using forward_type  = tracker<reporter<Child, Parent, Hook>, Parent, Child>;
//    using base_type     = typename forward_type::base_type;
//  };

  template <typename Reporter, typename Child>
  class reporter_base
  {
  public:

    using reporter_type  = Reporter;
    using child_type     = Child;

    using tracker_base_type   = tracker_base<reporter_type, child_type>;
    using self_ptr_type  = reporter_ptr<reporter_type>;
    using ext_citer_type = typename tracker_base_type::citer;
    using self_iter_type = typename tracker_base_type::internal_iter;

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
      // copy-and-swap idiom
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
          std::swap (m_tracker, other.m_tracker);
          std::swap (m_self_iter, other.m_self_iter);
          
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
          std::swap (m_tracker, other.m_tracker);
          std::swap (m_self_iter, other.m_self_iter);

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

    tracker_base_type * m_tracker = nullptr;
    self_iter_type m_self_iter    = self_iter_type { };

  };

  template <typename Derived, typename Parent, typename Child, typename Hook>
  class intrusive_reporter : public reporter_base<Derived, Child>
  {

  public:

    using derived_type      = Derived;
    using parent_type       = Parent;
    using child_type        = Child;
    using hook_type         = Hook;
    using base_type         = reporter_base<derived_type, child_type>;
    
    using tracker_type      = tracker<derived_type, parent_type, child_type>;
    using tracker_base_type = typename tracker_type::base_type;

    static_assert (std::is_same<tracker_base_type,
                                tracker_base<derived_type, child_type>>::value,
                   "tracker base has unexpected type");

    friend reporter_ptr<Derived>::~reporter_ptr<Derived> (void) noexcept;

    intrusive_reporter (void) noexcept = default;

    explicit intrusive_reporter (tracker_type& tkr, hook_type&& hook)
      : base_type (tkr),
        m_orphan_hook (std::move (hook))
    { }

    explicit intrusive_reporter (tracker_type& tkr)
      : intrusive_reporter (tkr, hook_type { })
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
      std::swap (m_orphan_hook, other.m_orphan_hook);
    }
    
    constexpr const child_type * get_child_ptr (void) const noexcept
    {
      return downcast (this);
    }

    child_type * get_child_ptr (void) noexcept
    {
      return downcast (this);
    }

    constexpr tracker_type *get_tracker (void) const noexcept
    {
      return static_cast<tracker_type *> (this->get_base_tracker ());
    }

    constexpr bool has_parent (void) const noexcept
    {
      return this->has_tracker () && this->get_tracker ()->has_parent ();
    }

    constexpr parent_type *get_parent (void) const noexcept
    {
      return this->has_tracker () ? this->get_tracker()->get_parent ()
                                  : nullptr;
    }

    void orphan (void)
    {
      base_type::reset_tracker (nullptr);
      m_orphan_hook (get_child_ptr ());
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
  template <typename Child, typename Parent, typename Hook>
  class reporter : public reporter_base<reporter<Child, Parent, Hook>, Child>
  {
  public:
    using child_type   = Child;
    using parent_type  = Parent;
    using hook_type    = Hook;
    using base_type    = reporter_base<reporter, child_type>;
    using tracker_type = tracker<reporter, parent_type, child_type>;

    static_assert (std::is_same<tracker_type,
                           tracker<reporter, parent_type, child_type>>::value,
                   "tracker has unexpected type");
    
    reporter (void) noexcept = default;

    explicit reporter (child_type *child, tracker_type& tkr, hook_type&& hook)
      : base_type (tkr),
        m_child (child),
        m_orphan_hook (std::move (hook))
    { }

    explicit reporter (child_type *child, tracker_type& tkr)
      : reporter (child, tkr, hook_type { })
    { }

    explicit reporter (tracker_type& tkr, hook_type&& hook)
      : base_type (tkr),
        m_orphan_hook (std::move (hook))
    { }

    explicit reporter (tracker_type& tkr)
      : reporter (tkr, hook_type { })
    { }

    reporter (const reporter&)            = delete;
    
    reporter (reporter&&) noexcept        = delete;

    reporter (child_type *new_child, const reporter& other)
      : base_type (other),
        m_child (new_child),
        m_orphan_hook (other.m_orphan_hook)
    { }

    reporter (child_type *new_child, reporter&& other) noexcept
      : base_type (std::move (other)),
        m_child (new_child),
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
      std::swap (m_child, other.m_child);
      std::swap (m_orphan_hook, other.m_orphan_hook);
    }

    constexpr child_type& get_child_ref (void) const noexcept
    {
      return *m_child;
    }

    constexpr const child_type * get_child_ptr (void) const noexcept
    {
      return m_child;
    }

    child_type * get_child_ptr (void) noexcept
    {
      return m_child;
    }

    void orphan (void)
    {
      base_type::reset_tracker (nullptr);
      m_orphan_hook (get_child_ptr ());
    }

  private:

    child_type *m_child = nullptr;
    hook_type m_orphan_hook = hook_type { };

  };

  template <typename Child, typename Parent, typename HookLocal, typename HookRemote>
  class reporter_ptr<multireporter<Parent, Child, HookRemote, HookLocal>>
  {
  public:
    
    using child_type = Child;
    
    using local_type    = multireporter<Child, Parent, HookLocal, HookRemote>;
    using reporter_type = multireporter<Parent, Child, HookRemote, HookLocal>;
    
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

  template <typename Child, typename Parent, typename HookLocal, typename HookRemote>
  class multireporter
    : public tracker<multireporter<Parent, Child, HookRemote, HookLocal>, Child, Parent>
  {
  public:
    using child_type    = Child;
    using parent_type   = Parent;
    using hook_type     = HookLocal;

    friend class multireporter<Parent, Child, HookRemote, HookLocal>;

    using remote_type   = multireporter<Parent, Child, HookRemote, HookLocal>;
    using local_tracker_type = tracker<remote_type, child_type, parent_type>;
    using local_tracker_base_type = typename local_tracker_type::base_type;
    
    using internal_iter  = typename local_tracker_type::internal_iter;
    using internal_citer = typename local_tracker_type::internal_citer;
    
    using external_iter  = typename local_tracker_type::iter;
    using external_citer = typename local_tracker_type::citer;
    
    using remote_tracker_type      = typename remote_type::local_tracker_type;
    using remote_tracker_base_type = typename remote_tracker_type::base_type;
    
    using self_iter_type = typename remote_type::internal_iter;
    
    friend void bind (multireporter& l, remote_type& r)
    {
      l.bind (r);
    }

    template <typename ...Args>
    enable_if_t<conjunction<std::is_same<Args, remote_type>...>::value> 
    bind (remote_type& r, Args&... args)
    {
      internal_bind (r);
      bind (args...);
    }
    
    external_citer bind (remote_type& r)
    {
      return internal_bind (r);
    }

    explicit multireporter (hook_type&& hook)
      : local_tracker_type (),
        m_orphan_hook (std::move (hook))
    { }

    explicit multireporter (void)
      : multireporter (hook_type { })
    { }

    explicit multireporter (child_type& cld, hook_type&& hook)
      : local_tracker_type (cld),
        m_orphan_hook (std::move (hook))
    { }

    explicit multireporter (child_type& cld)
      : multireporter (cld, hook_type { })
    { }

    explicit multireporter (child_type& cld, remote_type& r,
                            hook_type&& hook)
      : local_tracker_type (cld),
        m_orphan_hook (std::move (hook))
    {
      internal_bind (r);
    }

    explicit multireporter (child_type& cld, remote_type& rem)
      : multireporter (cld, rem, hook_type { })
    { }

    multireporter (const multireporter& other)
      : local_tracker_type (),
        m_orphan_hook (other.m_orphan_hook)
    {
      try
        {
          for (internal_citer cit = other.internal_begin ();
               cit != other.internal_end (); ++cit)
            {
              internal_bind (**cit);
            }
        }
      catch (...)
        {
          this->reset ();
          throw;
        }
    }

    multireporter (child_type& cld, const multireporter& other)
      : local_tracker_type (cld),
        m_orphan_hook (other.m_orphan_hook)
    {
      try
        {
          for (internal_citer cit = other.internal_begin (); 
               cit != other.internal_end (); ++cit)
            {
              internal_bind (**cit);
            }
        }
      catch (...)
        {
          this->reset ();
          throw;
        }
    }

    multireporter (multireporter&& other) noexcept
      : local_tracker_type (std::move (other)),
        m_orphan_hook (std::move (other.m_orphan_hook))
    { }

    multireporter (child_type& cld, multireporter&& other) noexcept
      : local_tracker_type (cld, std::move (other)),
        m_orphan_hook (std::move (other.m_orphan_hook))
    { }

    multireporter& operator= (const multireporter& other)
    {
      if (&other != this && other.has_reporters ())
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
          this->untrack (this->internal_begin (), pivot);  
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
      std::swap (m_orphan_hook, other.m_orphan_hook);
    }

    constexpr const child_type * get_child_ptr (void) const noexcept
    {
      return local_tracker_type::get_parent ();
    }

    child_type * get_child_ptr (void) noexcept
    {
      return local_tracker_type::get_parent ();
    }

    void orphan (internal_iter it)
    {
      this->erase (it);
      m_orphan_hook (get_child_ptr ());
    }
    
  private:
    
    internal_citer internal_bind (remote_type& r)
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

    hook_type m_orphan_hook;

  };

//  template <typename Reporter, typename Child>
//  class tracker_base;
//
//  template <typename Reporter,
//    typename Parent = tracker_base<Reporter, Reporter>,
//    typename Child = Reporter>
//  class tracker;
//
//  template <typename Reporter, typename Child>
//  class reporter_base;
//
//  template <typename Derived, typename Parent = tracker<Derived>,
//    typename Child = Derived, typename Hook = default_orphan_hook>
//  class intrusive_reporter;
//
//  template <typename Child, typename Parent = tracker<Child>,
//    typename Hook = default_orphan_hook>
//  class reporter;
//
//  template <typename Parent, typename Child = Parent,
//            typename HookLocal = default_orphan_hook, 
//            typename HookRemote = default_orphan_hook>
//  class multireporter;

}

#endif
