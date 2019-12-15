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
#include <optional>

namespace octave
{

  // LocalSuperior may be either intrusive_reporter or tracker_base
  // Likewise for RemoteSuperior
  template <typename LocalSuperior, typename RemoteSuperior>
  class reporter_base;

  template <typename Derived, typename RemoteParent, typename Remote>
  class intrusive_reporter;

  template <typename LocalParent, typename RemoteParent, typename Remote>
  class reporter;

  template <typename Remote>
  class tracker_base;

  template <typename Derived, typename RemoteParent, template <typename...> class Remote, typename LocalParent>
  class intrusive_tracker;

  template <typename LocalParent, typename RemoteParent, template <typename...> class Remote>
  class tracker;
  
  template <typename LocalParent, typename RemoteParent, typename Remote>
  class multireporter;

  template <typename Derived, typename RemoteParent,
    typename Remote = tracker<RemoteParent, Derived, intrusive_reporter>>
  class intrusive_reporter;

  template <typename LocalParent, typename RemoteParent,
    typename Remote = tracker<RemoteParent, LocalParent, reporter>>
  class reporter;

  template <typename Derived, typename RemoteParent, 
            template <typename ...> class Remote, 
            typename LocalParent = Derived>
  class intrusive_tracker;

  template <typename LocalParent, typename RemoteParent,
    template <typename ...> class Remote = intrusive_reporter>
  class tracker;

  template <typename LocalParent, typename RemoteParent = LocalParent,
    typename Remote = tracker<RemoteParent, LocalParent, multireporter>>
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

  template <typename T>
  using superior_t = typename reporter_traits<T>::superior_type;

  template <typename Derived, typename RemoteParent, typename Remote>
  struct reporter_traits<intrusive_reporter<Derived, RemoteParent, Remote>>
  {
    using type = intrusive_reporter<Derived, RemoteParent, Remote>;
    using base_type = reporter_base<
      intrusive_reporter<Derived, RemoteParent, Remote>,
      tracker_base<intrusive_reporter<Derived, RemoteParent, Remote>>>;
    using intrusive_type = type;
    using abs_base_type = base_type;
    using superior_type = type;
  };

  template <typename LocalParent, typename RemoteParent, typename Remote>
  struct reporter_traits<reporter<LocalParent, RemoteParent, Remote>>
  {
    using type = reporter<LocalParent, RemoteParent, Remote>;
    using base_type = intrusive_reporter<type, RemoteParent, Remote>;
    using intrusive_type = base_type;
    using abs_base_type  = typename reporter_traits<intrusive_type>::base_type;
    using superior_type = intrusive_type;
  };

  template <typename Remote>
  struct reporter_traits<tracker_base<Remote>>
  {
    using type = tracker_base<Remote>;
    using base_type = tracker_base<Remote>;
    using abs_base_type = type;
    using superior_type = type;
  };
  
  template <typename LocalParent, typename RemoteParent, template <typename...> class Remote>
  struct reporter_traits<tracker<LocalParent, RemoteParent, Remote>>
  {
    using type = tracker<LocalParent, RemoteParent, Remote>;
    using base_type = intrusive_tracker<type, RemoteParent, Remote, LocalParent>;
    using intrusive_type = base_type;
    using abs_base_type = tracker_base<Remote<RemoteParent, LocalParent, type>>;
    using superior_type = abs_base_type;
  };
  
  template <typename LocalParent, typename RemoteParent, typename Dummy>
  struct reporter_traits<multireporter<LocalParent, RemoteParent, Dummy>>
  {
    using base_type = tracker<LocalParent, RemoteParent, multireporter>;
    using intrusive_type 
      = typename reporter_traits<base_type>::intrusive_type;
    using abs_base_type 
      = typename reporter_traits<base_type>::abs_base_type;
    using superior_type = abs_base_type;
  };

  template <typename Derived, typename RemoteParent, template <typename...> class Remote, typename LocalParent>
  struct reporter_traits<intrusive_tracker<Derived, RemoteParent, Remote, LocalParent>>
  {
    using type = intrusive_tracker<Derived, RemoteParent, Remote, LocalParent>;
    using base_type = tracker_base<
      typename std::conditional<
        is_type<intrusive_reporter,
          superior_t<Remote<RemoteParent, LocalParent, Derived>>>::value,
        superior_t<Remote<RemoteParent, LocalParent, Derived>>,
        Remote<RemoteParent, LocalParent, Derived>>::type>;
    using intrusive_type = type;
    using abs_base_type = base_type;
    using superior_type = base_type;
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

    template <typename ...Ts>
    reporter_iterator (reporter_iterator<reporter_citer, Ts...> it)
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

    constexpr reference get (void) const noexcept
    {
      return static_cast<reference> (get_remote ().get_parent ());
    }

    constexpr reference operator* (void) const noexcept
    {
      return get ();
    }

    constexpr pointer operator-> (void) const noexcept
    {
      return &get ();
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
    
    constexpr remote_type& get_remote (void) const noexcept 
    {
      return static_cast<remote_type&> (m_citer->get_remote ());
    }
    
    reporter_citer m_citer;
    
  };
  
  template <typename LocalSuperior, typename ...Ts>
  class reporter_base<LocalSuperior, intrusive_reporter<Ts...>>
  {
  public:

    using remote_type          = intrusive_reporter<Ts...>;
    using remote_reporter_type = remote_type;
    
    reporter_base            (void)                           = default;
    reporter_base            (const reporter_base&)           = delete;
    reporter_base            (reporter_base&& other) noexcept = delete;
    reporter_base& operator= (const reporter_base&)           = delete;
    reporter_base& operator= (reporter_base&& other) noexcept = delete;
    ~reporter_base           (void)                           = default;

    explicit constexpr reporter_base (remote_type& ptr) noexcept
      : m_remote (&ptr)
    { }
    
    void orphan_remote (void) noexcept 
    {
      if (has_remote ())
        get_remote ().orphan ();
    }

    constexpr bool has_remote (void) const noexcept
    {
      return m_remote.has_value ();
    }
    
    constexpr bool is_tracked (void) const noexcept 
    {
      return has_remote ();
    }

    constexpr remote_type& get_remote (void) const noexcept
    {
      return **m_remote;
    }
    
    constexpr remote_reporter_type& get_remote_reporter (void) const noexcept
    {
      return get_remote ();
    }

    template <typename Remote>
    void set_remote (Remote& remote) noexcept
    {
      m_remote = &static_cast<remote_type&> (remote);
    }

  private:

    std::optional<remote_type *> m_remote;

  };

  template <typename LocalSuperior, typename Other>
  class reporter_base<LocalSuperior, tracker_base<Other>>
  {
  public:

    // local superior can be either a type of reporter, or a tracker.
//    using local_superior_type = superior_t<LocalSuperior>;
//    using remote_type = typename std::conditional<is_type<tracker_base, LocalSuperior>::value, LocalSuperior, tracker_base<LocalSuperior>>::type;
  using remote_type = tracker_base<Other>;  
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

    explicit reporter_base (remote_type& remote)
      : m_data ({ &remote, remote.track (*this)})
    { }

    explicit reporter_base (remote_type& remote, self_iter it)
      : m_data ({ &remote, it })
    { }

    reporter_base (const reporter_base& other)
      : m_data (other.has_remote ()
                ? decltype (m_data) ({ &other.get_remote (),
                                       other.get_remote ().track (*this) })
                : std::nullopt)
    { }

    reporter_base (reporter_base&& other) noexcept
      : m_data (std::move (other.m_data))
    {
      other.reset ();
    }

    reporter_base& operator= (const reporter_base& other)
    {
      if (&other != this)
        {
          if (other.has_remote ())
            rebind (other.get_remote ());
          else
            {
              if (is_tracked ())
                get_remote ().orphan (*get_self_iter ());
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
            get_remote ().orphan (*get_self_iter ());

          m_data = std::move (other.m_data);
          other.reset ();
        }
      return *this;
    }

    void orphan_remote (void) noexcept
    {
      if (is_tracked ())
        {
          get_remote ().orphan (*get_self_iter ());
          reset_self_iter ();
        }
    }

    [[nodiscard]]
    std::size_t get_position (void) const noexcept
    {
      if (! is_tracked ())
        return 0;
      return get_remote ().get_offset (*get_self_iter ());
    }

    reporter_base& rebind (remote_type& new_remote)
    {
      // if the remotes are the same then this is already in the remote,
      // so we shouldn't do anything unless we aren't being tracked right now.
      if (! has_remote () || &new_remote != &get_remote ())
        {
          // might fail
          self_iter new_iter = new_remote.track (*this);

          // if we didn't fail the rest is noexcept
          if (is_tracked ())
            get_remote ().orphan (*get_self_iter ());
          
          m_data = { &new_remote, new_iter };
        }
      else if (! get_self_iter ().has_value ())
        {
          set_self_iter (new_remote.track (*this));
        }
      return *this;
    }

    // WARNING! This is a pure swap. Move to private when ready.
    void swap (reporter_base& other) noexcept
    {
      using std::swap;
      swap (m_data, other.m_data);
    }

    [[nodiscard]]
    constexpr bool has_remote (void) const noexcept
    {
      return m_data.has_value ();
    }

    [[nodiscard]]
    constexpr bool is_tracked (void) const noexcept
    {
      return has_remote () && get_self_iter ().has_value ();
    }

    constexpr remote_type& get_remote (void) const noexcept
    {
      return *std::get<remote_type *> (*m_data);
    }

    constexpr remote_reporter_type& get_remote_reporter (void) const noexcept
    {
      return **get_self_iter ();
    }
    
    void orphan (void) noexcept
    {
      if (has_remote ())
        reset_self_iter ();
    }

    void set_remote (remote_type& remote) noexcept
    {
      m_data = { &remote, has_remote () ? get_self_iter () : std::nullopt };
    }

//  protected:

    void set (remote_type& remote, self_iter it) noexcept
    {
      m_data = { &remote, it };
    }
    
    void reset (void) noexcept 
    {
      m_data.reset ();
    }

  private:
    
    template <typename ...Args>
    constexpr void set_self_iter (Args&&... args) noexcept 
    {
      m_data->second = std::optional<self_iter> (std::forward<Args> (args)...);
    }

    constexpr void reset_self_iter (void) noexcept
    {
      m_data->second.reset ();
    }

    constexpr const std::optional<self_iter>& 
    get_self_iter (void) const noexcept
    {
      return std::get<std::optional<self_iter>> (*m_data);
    }

    std::optional<std::pair<remote_type *, std::optional<self_iter>>> m_data;

  };

  template <typename Derived, typename RemoteParent, typename Remote>
  class intrusive_reporter 
    : public reporter_base<
        intrusive_reporter<Derived, RemoteParent, Remote>, 
        tracker_base<intrusive_reporter<Derived, RemoteParent, Remote>>>
  {
  public:
    
    // tracker should be derived from tracker_base
    using remote_parent_type  = RemoteParent;
    
    using remote_type         = Remote;
//    using remote_base_type    = typename Tracker::base_type;
    
    using intrusive_base_type = intrusive_reporter;
    using base_type           = reporter_base<
      intrusive_reporter<Derived, RemoteParent, Remote>, 
       tracker_base<intrusive_reporter<Derived, RemoteParent, Remote>>>;

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
        this->get_remote_reporter ().set_remote (*this);
    }

    intrusive_reporter& operator= (intrusive_reporter&& other) noexcept
    {
      if (&other != this)
        {
          base_type::operator= (std::move (other));
          if (this->is_tracked ())
            this->get_remote_reporter ().set_remote (*this);
        }
      return *this;
    }

    ~intrusive_reporter (void) noexcept
    {
      if (base_type::is_tracked ())
        base_type::orphan_remote ();
    }

    explicit intrusive_reporter (remote_type& remote)
      : base_type (remote)
    { }

    void swap (intrusive_reporter& other) noexcept
    {
      if (&other.get_remote () != &this->get_remote ())
        {
          using std::swap;
          base_type::swap (other);
          
          if (this->is_tracked ())
            this->get_remote_reporter ().set_remote (*this);

          if (other.is_tracked ())
            other.get_remote_reporter ().set_remote (other);
        }
    }

    [[nodiscard]]
    constexpr remote_type& get_remote (void) const noexcept
    {
      return static_cast<remote_type&> (base_type::get_remote ());
    }

    [[nodiscard]]
    constexpr bool has_parent (void) const noexcept
    {
      return true;
    }

    constexpr intrusive_reporter& get_parent (void) const noexcept
    {
      return const_cast<intrusive_reporter&> (*this);
    }

    [[nodiscard]]
    constexpr bool has_remote_parent (void) const noexcept
    {
      return base_type::has_remote () && get_remote ().has_parent ();
    }

    constexpr remote_parent_type& get_remote_parent (void) const noexcept
    {
      return static_cast<remote_parent_type&> (get_remote ().get_parent ());
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
    
    constexpr bool has_parent (void) const noexcept 
    {
      return m_local_parent.has_value ();
    }

    constexpr local_parent_type& get_parent (void) const noexcept
    {
      return **m_local_parent;
    }

  private:
    
    std::optional<local_parent_type *> m_local_parent;
    
  };
  
  //! Remote is one of the non-base classes defined here
  template <typename Remote>
  class tracker_base
  {
  public:

    // this is either intrusive_reporter or intrusive_tracker
    using remote_type = typename reporter_traits<Remote>::superior_type;
    using remote_base_type = typename reporter_traits<Remote>::abs_base_type;
    
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
    
    using remote_internal_iterator = typename plf::list<reporter_base<remote_type, tracker_base>>::iterator;

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
      reset ();
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
      this->repoint_reporters (*this);
      other.repoint_reporters (other);
    }

    std::size_t num_reporters (void) const noexcept
    {
      return m_reporters.size ();
    }

    void internal_transfer_from (tracker_base&& src, 
                                 internal_citer pos) noexcept
    {
      src.repoint_reporters (*this);
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

    
    bool   empty   (void) const noexcept { return m_reporters.empty (); }
    void   reverse (void)       noexcept { return m_reporters.reverse (); }

//    // safe, may throw
//    template <typename ...Args>
//    internal_iter track (Args&&... args)
//    {
//      return m_reporters.emplace (m_reporters.end (),
//                                  std::forward<Args> (args)...);
//    }

    // safe, may throw
    internal_iter track (void)
    {
      return m_reporters.emplace (m_reporters.end ());
    }

    // safe, may throw
    internal_iter track (remote_base_type& reporter)
    {
      return m_reporters.emplace (m_reporters.end (), static_cast<remote_type&> (reporter));
    }

    // safe, may throw
    internal_iter track (remote_base_type& reporter, remote_internal_iterator it)
    {
      return m_reporters.emplace (m_reporters.end (), static_cast<remote_type&> (reporter), it);
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

    std::size_t get_offset (internal_citer pos) const noexcept
    {
      return std::distance (internal_cbegin (), pos);
    }
    
    void repoint_reporters (tracker_base& tkr) noexcept
    {
      for (internal_ref c_ptr : m_reporters)
        c_ptr.get_remote_reporter ().set_remote (tkr);
    }

  private:
    
    reporter_list m_reporters;

  };

  template <typename Derived, typename RemoteParent, template <typename...> class Remote, typename LocalParent>
  class intrusive_tracker 
: public tracker_base<
    typename std::conditional<
      is_type<intrusive_reporter, 
              superior_t<Remote<RemoteParent, LocalParent, Derived>>>::value,
      superior_t<Remote<RemoteParent, LocalParent, Derived>>,
      Remote<RemoteParent, LocalParent, Derived>>::type>
  {
  public:
    
    using derived_type         = Derived;
    using remote_parent_type   = RemoteParent;
    using remote_type          = Remote<RemoteParent, LocalParent, Derived>;
    
    using remote_superior_type = typename std::conditional<
      is_type<intrusive_reporter,
        superior_t<remote_type>>::value,
      superior_t<remote_type>,
        remote_type>::type;
    
    using base_type = tracker_base<remote_superior_type>;
    
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

    constexpr derived_type& get_parent (void) const noexcept
    {
      return static_cast<derived_type&> (*this);
    }
    
    std::size_t get_offset (citer pos) const noexcept 
    {
      return std::distance (base_type::internal_cbegin (), 
                            internal_citer (pos));
    }
  };
  
  // intrusive_tracker<tracker<LocalParent, RemoteParent, Remote>, 
  //                               RemoteParent, Remote, LocalParent>
  
  // intrusive_tracker<tracker<nonintruded_parent_temp<nonintruded_child_s>, nonintruded_child_s, reporter>, 
  //                               nonintruded_child_s, reporter, nonintruded_parent_temp<nonintruded_child_s>>
  
  // tracker_base<Remote<RemoteParent, LocalParent, Derived>>
  
  // tracker_base<reporter<nonintruded_child_s, nonintruded_parent_temp<nonintruded_child_s>, 
  //              tracker<nonintruded_parent_temp<nonintruded_child_s>, nonintruded_child_s, reporter>>>

  template <typename LocalParent, typename RemoteParent, template <typename ...> class Remote>
  class tracker 
    : public intrusive_tracker<tracker<LocalParent, RemoteParent, Remote>, 
                               RemoteParent, Remote, LocalParent>
  {
  public:

    using local_parent_type  = LocalParent;
    using remote_parent_type = RemoteParent;

    using base_type = intrusive_tracker<tracker, RemoteParent, Remote, LocalParent>;
    using intrusive_tracker_type = base_type;
    using tracker_base_type      = typename base_type::base_type;

    tracker            (void)               = default;
    tracker            (const tracker&)     = delete;
    tracker            (tracker&&) noexcept = default;
    tracker& operator= (const tracker&)     = delete;
//  tracker& operator= (tracker&&) noexcept = impl;
    ~tracker           (void)               = default;
    
    explicit tracker (local_parent_type& parent)
      : m_parent (&parent)
    { }

    tracker (const tracker& other, local_parent_type& new_parent) = delete;

    tracker (tracker&& other, local_parent_type& new_parent) noexcept
      : base_type (std::move (other)),
        m_parent (&new_parent)
    { }
    
    // explicitly defined so that m_parent doesn't get implicitly reset
    tracker& operator= (tracker&& other) noexcept
    {
      base_type::operator= (std::move (other));
      return *this;
    }
    
    void swap (tracker& other) noexcept 
    {
      intrusive_tracker_type::swap (other);
      using std::swap;
      swap (m_parent, other.m_parent); // caution!
    }

    constexpr bool has_parent (void) const noexcept
    {
      return m_parent.has_value ();
    }

    constexpr local_parent_type& get_parent (void) const noexcept
    {
      return **m_parent;
    }
    
  protected:
    
//    explicit tracker (local_parent_type *parent)
//      : m_parent (parent)
//    { }

  private:

    
    std::optional<local_parent_type *> m_parent;

  };
  
  template <typename LocalParent, typename RemoteParent, typename Dummy>
  class multireporter
    : public tracker<LocalParent, RemoteParent, multireporter>
  {
  public:
    
    using local_parent_type  = LocalParent;
    using remote_parent_type = RemoteParent;
    using remote_type        = multireporter<RemoteParent, LocalParent, 
                                 tracker<LocalParent, RemoteParent, multireporter>>;
    
    using base_type = tracker<LocalParent, RemoteParent, multireporter>;
    
  private:
    
    using internal_iter = typename base_type::internal_iter;
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
          internal_iter pivot = internal_bind (static_cast<remote_type&>(other.internal_front ().get_remote ()));
          try
            {
              for (internal_citer cit = ++other.internal_begin ();
                   cit != other.internal_end (); ++cit)
                {
                  internal_bind (static_cast<remote_type&>(cit->get_remote ()));
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
    
    static constexpr remote_type& get_remote (internal_citer cit) noexcept 
    {
      return static_cast<remote_type&> (cit->get_remote ());
    }

    static constexpr const remote_type *downcast (
      const typename reporter_traits<remote_type>::intrusive_type *ptr)
    noexcept
    {
      return static_cast<const remote_type *> (ptr);
    }
    
    static constexpr remote_type *downcast (
      typename reporter_traits<remote_type>::intrusive_type *ptr)
        noexcept
    {
      return static_cast<remote_type *> (ptr);
    }
    
    internal_iter internal_bind (remote_type& remote)
    {
      internal_iter it = this->track ();
      try
        {
          it->set (remote, remote.track (*this, it));
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

//  template <typename LocalParent, typename RemoteParent>
//  class tracker_1<LocalParent, RemoteParent, intrusive_tag>
//    : public tracker<LocalParent, RemoteParent, RemoteParent>
//  {
//  public:
//    using base_type = tracker<LocalParent, RemoteParent, 
//                              RemoteParent>;
//    using tracker<LocalParent, RemoteParent, RemoteParent>::tracker;
//  };
//
//  template <typename LocalParent, typename RemoteParent>
//  class tracker_1<LocalParent, RemoteParent, nonintrusive_tag>
//    : public tracker<LocalParent, RemoteParent, 
//                     reporter<RemoteParent, LocalParent,
//                              tracker_1<LocalParent, RemoteParent,
//                                        nonintrusive_tag>>>
//  {
//  public:
//    using remote_type = reporter<RemoteParent, LocalParent, 
//                                 tracker_1>;
//    using base_type = tracker<LocalParent, RemoteParent, 
//                              remote_type>;
//    using tracker<LocalParent, RemoteParent, remote_type>::tracker;
//  };

  // defaults

//  template <typename Derived, typename RemoteParent, typename Remote>
//  class intrusive_reporter;
//
//  template <typename LocalParent, typename RemoteParent,
//    typename Remote = tracker<RemoteParent, LocalParent, reporter>>
//  class reporter;
//
//  template <typename Derived, typename RemoteParent, template <typename ...> class Remote, typename LocalParent = Derived>
//  class intrusive_tracker;
//
//  template <typename LocalParent, typename RemoteParent,
//            template <typename ...> class Remote = intrusive_reporter>
//  class tracker;
//
//  template <typename LocalParent, typename RemoteParent = LocalParent, 
//            typename Dummy = tracker<RemoteParent, LocalParent, multireporter>>
//  class multireporter;
  
}

#endif
