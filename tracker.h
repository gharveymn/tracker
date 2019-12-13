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
#include <iterator>

namespace octave
{
  
  // Idea:
  //   Label the local object of type `reporter` or `tracker` as `local`
  //   and the object which tracks that `remote` (which may also be either
  //   type `reporter` or `tracker`).
  //
  //   Trackers essentially lists of reporters and are themselves agnostic 
  //   to the type of `remote`. This is not the case for the constituent 
  //   reporters.
  //
  //   `reporter`s may point to two types: other `reporter`s, and `tracker`s
  //   if pointing to a `tracker` then a self-iterator is held
  //      the self-iterator iterates over elements of `reporter`->`reporter`
  //   otherwise, no self-iterator is held
  //
  //   reporters may or may not hold a pointer to their parent
  //
  //   reporter layer 1 members:
  //     pointer to base of tracker (which may be tracker or reporter)
  //     self-iterator if tracker is of type tracker
  //   reporter layer 2 members:
  //     none
  //
  //   reporter layer 1 methods:
  //     basic reporting mechanisms
  //   reporter layer 2 methods:
  //     downcast tracker from layer 1 to resolved type
  //     pointer to parent
  //
  
  struct intrusive_tag
  { };

  struct nonintrusive_tag
  { };

  // LocalSuperior may be either intrusive_reporter or tracker_base
  template <typename LocalSuperior, typename RemoteSuperior, 
            typename Enable = void>
  class reporter_base;

  template <typename Derived, typename RemoteParent, typename Remote>
  class intrusive_reporter;

  template <typename LocalParent, typename RemoteParent, typename Remote>
  class reporter;

  template <typename Remote>
  class tracker_base;

  template <typename Derived, typename RemoteParent, typename Remote>
  class intrusive_tracker;

  template <typename LocalParent, typename RemoteParent, typename Remote>
  class tracker;

  // RemoteTag is either `intrusive_tag` or `nonintrusive_tag`
  template <typename LocalParent, typename RemoteParent, 
            typename RemoteTag = nonintrusive_tag>
  class tracker_1;
  
  template <typename LocalParent, typename RemoteParent>
  class multireporter;
  
  // support types
  
  template <template <typename...> class Type, class Alias>
  struct is_type : std::false_type
  { };

  template <template <typename...> class Type, class ...TemplateParams>
  struct is_type<Type, Type<TemplateParams...>> : std::true_type
  { };
  
  template <typename...>
  using map_void = void;
  
  template <typename Reporter>
  struct reporter_traits
  { };

  template <typename T>
  using base_t = typename reporter_traits<T>::base_type;

  template <typename T>
  using intrusive_t 
    = typename reporter_traits<T>::intrusive_type;

  template <typename T>
  using abs_base_t = typename reporter_traits<T>::abs_base_type;

  template <typename LocalSuperior, typename RemoteSuperior, typename Enable>
  struct reporter_traits<reporter_base<LocalSuperior, RemoteSuperior, Enable>>
  {
    using type = reporter_base<LocalSuperior, RemoteSuperior, Enable>;
    using base_type = type;
    using abs_base_type = type;
  };

  template <typename Derived, typename RemoteParent, typename Remote>
  struct reporter_traits<intrusive_reporter<Derived, RemoteParent, Remote>>
  {
    using type = intrusive_reporter<Derived, RemoteParent, Remote>;
    using base_type = reporter_base<type, Remote>;
    using intrusive_type = type;
    using abs_base_type = base_type;
  };

  template <typename LocalParent, typename RemoteParent, typename Remote>
  struct reporter_traits<reporter<LocalParent, RemoteParent, Remote>>
  {
    using type = reporter<LocalParent, RemoteParent, Remote>;
    using base_type = intrusive_reporter<type, RemoteParent, Remote>;
    using intrusive_type = base_type;
    using abs_base_type  = typename reporter_traits<intrusive_type>::base_type;
  };

  template <typename Derived, typename RemoteParent, typename Remote>
  struct reporter_traits<intrusive_tracker<Derived, RemoteParent, Remote>>
  {
    using type = intrusive_tracker<Derived, RemoteParent, Remote>;
    using base_type = tracker_base<Remote>;
    using intrusive_type = type;
    using abs_base_type = base_type;
  };
  
  template <typename LocalParent, typename RemoteParent, typename Remote>
  struct reporter_traits<tracker<LocalParent, RemoteParent, Remote>>
  {
    using type = tracker<LocalParent, RemoteParent, Remote>;
    using base_type = intrusive_tracker<type, RemoteParent, Remote>;
    using intrusive_type = base_type;
    using abs_base_type = typename reporter_traits<intrusive_type>::base_type;
  };

  template <typename LocalParent, typename RemoteParent>
  struct reporter_traits<tracker_1<LocalParent, RemoteParent, intrusive_tag>>
  {
    using type = tracker_1<LocalParent, RemoteParent, intrusive_tag>;
    using base_type = tracker<LocalParent, RemoteParent, RemoteParent>;
    using intrusive_type
      = typename reporter_traits<base_type>::intrusive_type;
    using abs_base_type
      = typename reporter_traits<base_type>::abs_base_type;
  };

  template <typename LocalParent, typename RemoteParent>
  struct reporter_traits<tracker_1<LocalParent, RemoteParent, nonintrusive_tag>>
  {
    using type = tracker_1<LocalParent, RemoteParent, nonintrusive_tag>;
    using base_type = tracker<LocalParent, RemoteParent,
                              reporter<RemoteParent, LocalParent, type>>;
    using intrusive_type
      = typename reporter_traits<base_type>::intrusive_type;
    using abs_base_type
      = typename reporter_traits<base_type>::abs_base_type;
  };
  
  template <typename LocalParent, typename RemoteParent>
  struct reporter_traits<multireporter<LocalParent, RemoteParent>>
  {
    using base_type 
      = tracker<LocalParent, RemoteParent,
                multireporter<RemoteParent, LocalParent>>;
    using intrusive_type 
      = typename reporter_traits<base_type>::intrusive_type;
    using abs_base_type 
      = typename reporter_traits<base_type>::abs_base_type;
  };
  
  // Replace with std::optional in C++17. Don't use this for anything else.
  template <typename T>
  struct trivial_optional
  {
    trivial_optional            (void)                              = default;
    trivial_optional            (const trivial_optional&)           = default;
    trivial_optional            (trivial_optional&& other) noexcept = default;
    trivial_optional& operator= (const trivial_optional&)           = default;
    trivial_optional& operator= (trivial_optional&& other) noexcept = default;
    ~trivial_optional           (void)                              = default;
    
    template <typename U = T>
    explicit trivial_optional (U&& val)
      : m_val (std::forward<U> (val)),
        m_is_init (true)
    { }

    template <typename U = T>
    trivial_optional& operator= (U&& val)
    {
      m_val.operator= (std::forward<U> (val));
      m_is_init = true;
    }

    constexpr explicit operator bool() const noexcept
    {
      return m_is_init;
    }
    
    constexpr bool has_value (void) const noexcept 
    {
      return m_is_init;
    }

    constexpr const T * operator-> (void) const
    {
      return &m_val;
    }

    T * operator-> (void)
    {
      return &m_val;
    }
    
    constexpr const T& operator* (void) const&
    {
      return m_val;
    }

    T& operator* (void) &
    {
      return m_val;
    }

    constexpr const T&& operator* (void) const&&
    {
      return m_val;
    }

    T&& operator* (void) &&
    {
      return m_val;
    }
    
    void reset (void) noexcept
    {
      if (m_is_init)
        {
          m_is_init = false;
          m_val.~T ();
        }
    }
    
  private:
    T    m_val;
    bool m_is_init = false;
    
  };

  // pretend like reporter_base doesn't exist
  template <typename ReporterCIter, typename RemoteParent, typename Remote>
  struct reporter_iterator
  {
  private:
    using reporter_citer     = ReporterCIter;
    using remote_parent_type = RemoteParent;
    using remote_type        = Remote;
    
    using reporter_type      = typename ReporterCIter::value_type;

  public:
    
    using difference_type   = typename reporter_citer::difference_type;
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type        = remote_parent_type;
    using pointer           = remote_parent_type *;
    using const_pointer     = const remote_parent_type *;
    using reference         = remote_parent_type&;
    using const_reference   = const remote_parent_type&;

    template <typename Iter>
    reporter_iterator (Iter it)
      : m_citer (reporter_citer (it))
    { }

    template <typename OtherRemoteParent, typename OtherRemote>
    reporter_iterator (reporter_iterator<reporter_citer, OtherRemoteParent, 
                                         OtherRemote> it)
      : m_citer (it.get_iterator ())
    { }
    
    operator reporter_citer (void)
    {
      return m_citer;
    }

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

    constexpr pointer get (void) const noexcept
    {
      return static_cast<pointer> (static_cast<remote_type *> (m_citer->get_remote ())->get_parent ());
    }

    constexpr reference operator* (void) const noexcept
    {
      return *get ();
    }

    constexpr pointer operator-> (void) const noexcept
    {
      return get ();
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
    
    reporter_citer m_citer;
    
  };

  template <typename LocalSuperior, typename RemoteSuperior, typename Enable>
  class reporter_base
  {
  public:

    using remote_type          = RemoteSuperior;
    using remote_reporter_type = remote_type;
    
    reporter_base            (void)                           = default;
    reporter_base            (const reporter_base&)           = delete;
    reporter_base            (reporter_base&& other) noexcept = delete;
    reporter_base& operator= (const reporter_base&)           = delete;
    reporter_base& operator= (reporter_base&& other) noexcept = delete;
    ~reporter_base           (void)                           = default;

    explicit constexpr reporter_base (remote_type *ptr) noexcept
      : m_remote (ptr)
    { }
    
    void orphan_remote (void) noexcept 
    {
      if (has_remote ())
        m_remote->orphan ();
    }

    constexpr bool has_remote (void) const noexcept
    {
      return m_remote != nullptr;
    }
    
    constexpr bool is_tracked (void) const noexcept 
    {
      return has_remote ();
    }

    constexpr remote_type * get_remote (void) const noexcept
    {
      return m_remote;
    }
    
    constexpr remote_reporter_type * get_remote_reporter (void) const noexcept
    {
      return get_remote ();
    }

    void reset_remote (remote_type *ptr) noexcept
    {
      m_remote = ptr;
    }

  private:

    remote_type *m_remote = nullptr;

  };

  template <typename LocalSuperior, typename Tracker>
  class reporter_base<LocalSuperior, Tracker, 
           map_void<typename reporter_traits<Tracker>::tracker_base_type>>
  {
  public:

    // local superior can be either a type of reporter, or a tracker.
    
    using remote_type = Tracker;
    using remote_reporter_type = reporter_base<remote_type, LocalSuperior>;
    
  private:
    
    using self_iter = typename plf::list<remote_reporter_type>::iterator;

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

    explicit reporter_base (remote_type& rem)
      : m_remote (&rem),
        m_self_iter (m_remote->track (downcast (this))),
        m_is_tracked (true)
    { }

    reporter_base (const reporter_base& other)
      : m_remote (other.m_remote),
        m_self_iter (other.has_remote () 
                       ? other.m_remote->track (downcast (this))
                       : self_iter { }),
        m_is_tracked (true)
    { }

    reporter_base (reporter_base&& other) noexcept
      : m_remote (other.m_remote),
        m_self_iter (other.m_self_iter)
    {
      other.m_remote = nullptr;
      other.m_is_tracked = false;
    }

    reporter_base& operator= (const reporter_base& other)
    {
      if (&other != this)
        {
          if (other.has_remote ())
            rebind (*other.m_remote);
          else
            {
              if (this->is_tracked ())
                m_remote->orphan (m_self_iter);
              m_remote     = nullptr;
              m_is_tracked = false;
            }
        }
      return *this;
    }

    reporter_base& operator= (reporter_base&& other) noexcept
    {
      if (&other != this)
        {
          if (this->is_tracked ())
            m_remote->orphan (m_self_iter);

          m_remote     = other.m_remote;
          m_self_iter  = other.m_self_iter;
          m_is_tracked = other.m_is_tracked;
          
          other.m_remote     = nullptr;
          other.m_is_tracked = false;
        }
      return *this;
    }

    void orphan_remote (void) noexcept
    {
      if (m_is_tracked)
        {
          m_remote->orphan (m_self_iter);
          m_is_tracked = false; 
        }
    }
    
    std::size_t get_position (void) const noexcept
    {
      if (! has_remote ())
        return 0;
      return m_remote->get_offset (m_self_iter);
    }

    reporter_base& rebind (remote_type& new_remote)
    {
      // if the remotes are the same then this is already in the remote,
      // so we shouldn't do anything unless we aren't being tracked right now.
      if (&new_remote != m_remote)
        {
          // might fail
          self_iter new_iter = new_remote.track (this);

          // if we didn't fail the rest is noexcept
          if (is_tracked ())
            m_remote->orphan (m_self_iter);
          
          m_remote     = &new_remote;
          m_self_iter  = new_iter;
          m_is_tracked = true;
        }
      else if (! this->is_tracked ())
        {
          m_self_iter  = m_remote->track (this);
          m_is_tracked = true;
        }
      return *this;
    }

    // WARNING! This is a pure swap. Move to private when ready.
    void swap (reporter_base& other) noexcept
    {
        using std::swap;
        swap (m_remote, other.m_remote_ptr);
        swap (m_self_iter, other.m_self_iter);
    }

    constexpr bool has_remote (void) const noexcept
    {
      return m_remote != nullptr;
    }

    constexpr bool is_tracked (void) const noexcept
    {
      return m_is_tracked;
    }

    constexpr remote_type * get_remote (void) const noexcept
    {
      return m_remote;
    }

    constexpr remote_reporter_type * get_remote_reporter (void) const noexcept
    {
      return *m_self_iter;
    }
    
    void orphan (void) noexcept
    {
      m_is_tracked = false;
    }

    void reset_remote (remote_type *ptr) noexcept
    {
      m_remote = ptr;
    }

  protected:

    void reset (remote_type *ptr = nullptr,
                self_iter it = self_iter { }) noexcept
    {
      m_remote = ptr;
      m_self_iter   = it;
    }

  private:

    static constexpr const LocalSuperior *downcast (const reporter_base *ptr)
      noexcept
    {
      return static_cast<const LocalSuperior *> (ptr);
    }

    static constexpr LocalSuperior *downcast (reporter_base *ptr)
      noexcept
    {
      return static_cast<LocalSuperior *> (ptr);
    }

    remote_type *m_remote     = nullptr;
    self_iter    m_self_iter;
    bool         m_is_tracked = false;

  };

  template <typename Derived, typename RemoteParent, typename Remote>
  class intrusive_reporter 
    : public reporter_base<Derived, Remote>
  {
  public:
    
    // tracker should be derived from tracker_base
    using remote_parent_type  = RemoteParent;
    
    using remote_type         = Remote;
//    using remote_base_type    = typename Tracker::base_type;
    
    using intrusive_base_type = intrusive_reporter;
    using base_type           = reporter_base<Derived, remote_type>;

    intrusive_reporter            (void)                          = default;
    intrusive_reporter            (const intrusive_reporter&)     = default;
//  intrusive_reporter            (intrusive_reporter&&) noexcept = impl;
    intrusive_reporter& operator= (const intrusive_reporter&)     = default;
//  intrusive_reporter& operator= (intrusive_reporter&&) noexcept = impl;
//  ~intrusive_reporter           (void)                          = impl;

    intrusive_reporter (intrusive_reporter&& other) noexcept
      : base_type (std::move (other))
    {
      if (this->is_tracked ())
        this->get_remote_reporter ()->reset_remote (this);
    }

    intrusive_reporter& operator= (intrusive_reporter&& other) noexcept
    {
      if (&other != this)
        {
          base_type::operator= (other);
          if (this->is_tracked ())
            this->get_remote_reporter ()->reset_remote (this);
        }
      return *this;
    }

    ~intrusive_reporter (void) noexcept
    {
      if (base_type::has_remote ())
        base_type::orphan_remote ();
    }

    explicit intrusive_reporter (remote_type& tkr)
      : base_type (tkr)
    { }

    void swap (intrusive_reporter& other) noexcept
    {
      if (other.get_remote () != this->get_remote ())
        {
          using std::swap;
          base_type::swap (other);
          
          if (this->is_tracked ())
            this->get_remote_reporter ()->reset_remote (this);

          if (other.is_tracked ())
            other.get_remote_reporter ()->reset_remote (&other);
        }
    }

    constexpr remote_type *get_remote (void) const noexcept
    {
      return static_cast<remote_type *> (base_type::get_remote ());
    }

    constexpr bool has_parent (void) const noexcept
    {
      return true;
    }

    constexpr intrusive_reporter * get_parent (void) const noexcept
    {
      return const_cast<intrusive_reporter *> (this);
    }

    constexpr bool has_remote_parent (void) const noexcept
    {
      return base_type::has_remote () && get_remote ()->has_parent ();
    }

    constexpr remote_parent_type *get_remote_parent (void) const noexcept
    {
      return base_type::has_remote () 
             ? static_cast<remote_parent_type *> (get_remote ()->get_parent ()) 
             : nullptr;
    }

  };

  //! non-intrusive; for use as a class member
  template <typename LocalParent, typename RemoteParent, typename Remote>
  class reporter : public intrusive_reporter<
    reporter<LocalParent, RemoteParent, Remote>, RemoteParent, Remote>
  {
  public:
    
    using local_parent_type   = LocalParent;
    
    // tracker should be derived from tracker_base
    using remote_parent_type  = RemoteParent;
    using remote_type         = Remote;

    using intrusive_base_type = intrusive_reporter<reporter, RemoteParent, 
                                                   Remote>;

//    static_assert (std::is_base_of<remote_base_type, remote_type>::value,
//                   "tracker base has unexpected type");

    reporter            (void)                = default;
    reporter            (const reporter&)     = delete;
    reporter            (reporter&&) noexcept = delete;
//  reporter& operator= (const reporter&)     = impl;
//  reporter& operator= (reporter&&) noexcept = impl;
    ~reporter           (void)                = default;

    explicit reporter (remote_type& remote)
      : intrusive_base_type (remote)
    { }

    explicit reporter (remote_type& remote, local_parent_type& parent)
      : intrusive_base_type (remote),
        m_local_parent (&parent)
    { }

    reporter (const reporter& other, local_parent_type& new_parent)
      : intrusive_base_type (other),
        m_local_parent (&new_parent)
    { }

    reporter (reporter&& other, local_parent_type& new_parent) noexcept
      : intrusive_base_type (std::move (other)),
        m_local_parent (&new_parent)
    { }

    reporter& operator= (const reporter& other)
    {
      if (&other != this)
        intrusive_base_type::operator= (other);
      return *this;
    }

    reporter& operator= (reporter&& other) noexcept
    {
      intrusive_base_type::operator= (std::move (other));
      return *this;
    }

    void swap (reporter& other)
    {
      intrusive_base_type::swap (other);
      
      using std::swap;
      swap (m_local_parent, other.m_local_parent);
    }

    constexpr local_parent_type * get_parent (void) const noexcept
    {
      return m_local_parent;
    }

  private:

    local_parent_type *m_local_parent = nullptr;

  };
  
  //! Remote is one of the non-base classes defined here
  template <typename Remote>
  class tracker_base
  {
  public:

    // this is either intrusive_reporter or intrusive_tracker
    using remote_type = typename reporter_traits<Remote>::abs_base_type;
    
  protected:

    // Store pointers to the base types. Downcast when needed.
    using element_type    = reporter_base<tracker_base, remote_type>;
    using reporter_list   = plf::list<element_type>;
    using internal_iter   = typename reporter_list::iterator;
    using internal_citer  = typename reporter_list::const_iterator;
    using internal_riter  = typename reporter_list::reverse_iterator;
    using internal_criter = typename reporter_list::const_reverse_iterator;
    using internal_ref    = typename reporter_list::reference;
    using internal_cref   = typename reporter_list::const_reference;

  public:

    tracker_base            (void)                    = default;
    tracker_base            (const tracker_base&)     = delete;
//  tracker_base            (tracker_base&&) noexcept = impl;
    tracker_base& operator= (const tracker_base&)     = delete;
//  tracker_base& operator= (tracker_base&&) noexcept = impl;
//  ~tracker_base           (void)                    = impl;

    tracker_base (tracker_base&& other) noexcept
    { 
      internal_transfer_from (std::move (other));
    }

    tracker_base& operator= (tracker_base&& other) noexcept
    {
      internal_transfer_from (std::move (other));
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

    void orphan (internal_iter it)
    {
      this->erase (it);
    }
    
    void swap (tracker_base& other) noexcept 
    {
      // kinda expensive
      m_reporters.swap (other.m_reporters);
      for (internal_ref c_ptr : m_reporters)
        c_ptr.reset_remote (this);
      for (internal_ref c_ptr : other.m_reporters)
        c_ptr.reset_remote (&other);
    }

    std::size_t num_reporters (void) const noexcept
    {
      return m_reporters.size ();
    }

    void internal_transfer_from (tracker_base&& src, 
                                 internal_citer pos) noexcept
    {
      for (internal_ref rptr : src.m_reporters)
        rptr.reset_remote_reporter (this);
      return m_reporters.splice (pos, src.m_reporters);
    }

    void internal_transfer_from (tracker_base& src, 
                                 internal_citer pos) noexcept
    {
      return internal_transfer_from (std::move (src), pos);
    }

    template <typename T>
    void internal_transfer_from (T&& src) noexcept
    {
      return internal_transfer_from (std::forward<T> (src), 
                                     m_reporters.cend ());
    }
    
    // unsafe!
    void clear (void) noexcept
    {
      m_reporters.clear ();
    }

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

    
    bool   empty   (void) const noexcept { return m_reporters.empty (); }
    void   reverse (void)       noexcept { return m_reporters.reverse (); }

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

  private:

    internal_iter   begin   (void)       noexcept { return m_reporters.begin (); }
    
    reporter_list m_reporters;

  };  

  template <typename Derived, typename RemoteParent, typename Remote>
  class intrusive_tracker 
    : public tracker_base<Derived, Remote>
  {
  public:
    
    using remote_parent_type = RemoteParent;
    using remote_type        = Remote;

    using base_type = tracker_base<Derived, remote_type>;
    
    using internal_iter = typename base_type::internal_iter;
    using internal_citer = typename base_type::internal_citer;
    
    intrusive_tracker            (void)                         = default;
    intrusive_tracker            (const intrusive_tracker&)     = delete;
    intrusive_tracker            (intrusive_tracker&&) noexcept = default;
    intrusive_tracker& operator= (const intrusive_tracker&)     = delete;
    intrusive_tracker& operator= (intrusive_tracker&&) noexcept = default;
    ~intrusive_tracker           (void)                         = default;

    using base_type::swap;

    using iter   = reporter_iterator<internal_citer, remote_parent_type, 
                                     remote_type>;
    using citer  = iter;
    using riter  = std::reverse_iterator<iter>;
    using criter = std::reverse_iterator<citer>;
    using ref    = typename iter::reference;
    using cref   = typename citer::const_reference;

    iter   begin   (void)       noexcept { return base_type::internal_begin (); }
    citer  begin   (void) const noexcept { return base_type::internal_begin (); }
    citer  cbegin  (void) const noexcept { return base_type::internal_cbegin (); }

    iter   end     (void)       noexcept { return base_type::internal_end (); }
    citer  end     (void) const noexcept { return base_type::internal_end (); }
    citer  cend    (void) const noexcept { return base_type::internal_cend (); }

    riter  rbegin  (void)       noexcept { return base_type::internal_rbegin (); }
    criter rbegin  (void) const noexcept { return base_type::internal_rbegin (); }
    criter crbegin (void) const noexcept { return base_type::internal_crbegin (); }

    riter  rend    (void)       noexcept { return base_type::internal_rend (); }
    criter rend    (void) const noexcept { return base_type::internal_rend (); }
    criter crend   (void) const noexcept { return base_type::internal_crend (); }

    ref    front   (void)                { return base_type::internal_front (); }
    cref   front   (void) const          { return base_type::internal_front (); }

    ref    back    (void)                { return base_type::internal_back (); }
    cref   back    (void) const          { return base_type::internal_back (); }

    bool has_reporters (void) const noexcept
    {
      return ! base_type::empty ();
    }

    void transfer_from (intrusive_tracker&& src, citer pos) noexcept
    {
      return base_type::internal_transfer_from (std::move (src), pos);
    }

    void transfer_from (intrusive_tracker& src, citer pos) noexcept
    {
      return base_type::internal_transfer_from (src, pos);
    }

    template <typename T>
    void transfer_from (T&& src) noexcept
    {
      return base_type::internal_transfer_from (std::forward<T> (src));
    }

    template <typename ...Args>
    remote_type create (Args... args)
    {
      return { this, std::forward<Args> (args)... };
    }

    constexpr bool has_parent (void) const noexcept
    {
      return true;
    }

    constexpr intrusive_tracker * get_parent (void) const noexcept
    {
      return const_cast<intrusive_tracker *> (this);
    }
    
    std::size_t get_offset (citer pos) const noexcept 
    {
      return std::distance (base_type::internal_cbegin (), 
                            internal_citer (pos));
    }
  };

  template <typename LocalParent, typename RemoteParent, typename Remote>
  class tracker 
    : public intrusive_tracker<tracker<LocalParent, RemoteParent, Remote>, 
                               RemoteParent, Remote>
  {
  public:

    using local_parent_type  = LocalParent;
    using remote_parent_type = RemoteParent;
    using remote_type        = Remote;

    using base_type = intrusive_tracker<tracker, RemoteParent, Remote>;
    using intrusive_tracker_type = base_type;
    using tracker_base_type      = tracker_base<intrusive_tracker_type, Remote>;

    tracker            (void)               = default;
    tracker            (const tracker&)     = delete;
    tracker            (tracker&&) noexcept = default;
    tracker& operator= (const tracker&)     = delete;
    tracker& operator= (tracker&&) noexcept = default;
    ~tracker           (void)               = default;
    
    explicit tracker (local_parent_type& parent)
      : m_parent (&parent)
    { }

    tracker (const tracker& other, local_parent_type& new_parent) = delete;

    tracker (tracker&& other, local_parent_type& new_parent) noexcept
      : base_type (std::move (other)),
        m_parent (&new_parent)
    { }
    
    void swap (tracker& other) noexcept 
    {
      intrusive_tracker_type::swap (other);
      using std::swap;
      swap (m_parent, other.m_parent); // caution!
    }

    constexpr bool has_parent (void) const noexcept
    {
      return m_parent != nullptr;
    }

    constexpr local_parent_type * get_parent (void) const noexcept
    {
      return m_parent;
    }
    
  protected:
    
    explicit tracker (local_parent_type *parent)
      : m_parent (parent)
    { }

  private:

    local_parent_type *m_parent = nullptr;

  };
  
  template <typename LocalParent, typename RemoteParent>
  class multireporter
    : public tracker<LocalParent, RemoteParent, 
                     multireporter<RemoteParent, LocalParent>>
  {
  public:
    
    using local_parent_type  = LocalParent;
    using remote_parent_type = RemoteParent;
    using remote_type        = multireporter<RemoteParent, LocalParent>;
    
    using base_type = tracker<local_parent_type, remote_parent_type, 
                              remote_type>;
    
  private:
    
    using internal_iter = typename base_type::internal_citer;
    using internal_citer = typename base_type::internal_citer;
    
    using citer = typename base_type::citer;
    
  public:

    multireporter            (void)                     = default;
//  multireporter            (const multireporter&)     = impl;
    multireporter            (multireporter&&) noexcept = default;
//  multireporter& operator= (const multireporter&)     = impl;
    multireporter& operator= (multireporter&&) noexcept = default;
    ~multireporter           (void)                     = default;

    explicit multireporter (local_parent_type& parent)
      : base_type (parent)
      { }

    explicit multireporter (local_parent_type& parent, remote_type& remote)
      : base_type (parent)
    {
      internal_bind (remote);
    }

    multireporter (const multireporter& other)
      : base_type ()
    {
      copy_reporters (other);
    }

    multireporter (const multireporter& other, local_parent_type& parent)
      : base_type (parent)
    {
      copy_reporters (other);
    }

    multireporter (multireporter&& other, local_parent_type& parent) noexcept
      : base_type (std::move (other), parent)
    { }

    multireporter& operator= (const multireporter& other)
    {
      if (&other != this)
        overwrite_reporters (other);
      return *this;
    }

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

    citer bind (remote_type& r)
    {
      return internal_bind (r);
    }

    void swap (multireporter& other) noexcept
    {
      base_type::swap (other);
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
    
    static constexpr remote_type *get_remote (internal_citer cit) noexcept 
    {
      return static_cast<remote_type *> (cit->get_remote ());
    }

    static constexpr const remote_type *downcast (
      const typename reporter_traits<remote_type>::intrusive_tracker_type *ptr)
    noexcept
    {
      return static_cast<const remote_type *> (ptr);
    }
    
    static constexpr remote_type *downcast (
      typename reporter_traits<remote_type>::intrusive_tracker_type *ptr)
        noexcept
    {
      return static_cast<remote_type *> (ptr);
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
    
  };

  template <typename ...Types>
  void bind (multireporter<Types...>& l, 
             typename multireporter<Types...>::remote_type& r)
  {
    l.bind (r);
  }

  template <typename LocalParent, typename RemoteParent>
  class tracker_1<LocalParent, RemoteParent, intrusive_tag>
    : public tracker<LocalParent, RemoteParent, RemoteParent>
  {
  public:
    using base_type = tracker<LocalParent, RemoteParent, 
                              RemoteParent>;
    using tracker<LocalParent, RemoteParent, RemoteParent>::tracker;
  };

  template <typename LocalParent, typename RemoteParent>
  class tracker_1<LocalParent, RemoteParent, nonintrusive_tag>
    : public tracker<LocalParent, RemoteParent, 
                     reporter<RemoteParent, LocalParent,
                              tracker_1<LocalParent, RemoteParent,
                                        nonintrusive_tag>>>
  {
  public:
    using remote_type = reporter<RemoteParent, LocalParent, 
                                 tracker_1>;
    using base_type = tracker<LocalParent, RemoteParent, 
                              remote_type>;
    using tracker<LocalParent, RemoteParent, remote_type>::tracker;
  };

  // defaults

  template <typename Derived, typename RemoteParent, typename Remote>
  class intrusive_reporter;

  template <typename LocalParent, typename RemoteParent,
    typename Remote = tracker_1<RemoteParent, LocalParent>>
  class reporter;

  template <typename Derived, typename RemoteParent, typename Remote>
  class intrusive_tracker;

  template <typename LocalParent, typename RemoteParent, typename Remote = RemoteParent>
  class tracker;

  template <typename LocalParent, typename RemoteParent = LocalParent>
  class multireporter;
  
}

#endif
