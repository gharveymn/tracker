/** tracker.h
 * Track variable lifetimes automatically. 
 * 
 * Copyright © 2019 Gene Harvey
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

//! standalone
#if ! defined (tracker_h)
#define tracker_h 1

#include "octave-config.h"
#include <plf_list.h>
#include <iterator>
#include <optional>

namespace track
{
  
  namespace detail
  {

    template <typename LocalDerived, typename RemoteDerived,
      typename LocalTag, typename RemoteTag>
    class reporter_base;
    
    template <typename LocalDerived, typename RemoteDerived, typename RemoteTag>
    class tracker_base;

    template <typename LocalDerived, typename RemoteDerived,
      typename RemoteTag, typename LocalParent, typename LocalTag>
    class intrusive_reporter;
  
    template <typename LocalDerived, typename RemoteDerived,
              typename RemoteTag, typename LocalParent, typename LocalTag>
    class intrusive_tracker;
  
    template <typename LocalParent, typename RemoteDerived,
      typename RemoteTag>
    class reporter;
  
    template <typename LocalParent, typename RemoteDerived,
      typename RemoteTag, typename Enable = void>
    class tracker;
    
    struct reporter_base_tag
    {
      using base = reporter_base_tag;
    };
  
    struct tracker_base_tag
    {
      using base = tracker_base_tag;
    };
  
    struct intrusive_reporter_tag
      : reporter_base_tag
    {
      template <typename ...Ts>
      using type = intrusive_reporter<Ts...>;
    };
  
    struct intrusive_tracker_tag
      : tracker_base_tag
    {
      template <typename ...Ts>
      using type = intrusive_tracker<Ts...>;
    };
  
    struct reporter_tag
      : reporter_base_tag
    {
      template <typename ...Ts>
      using type = reporter<Ts...>;
    };
  
    struct tracker_tag
      : tracker_base_tag
    {
      template <typename ...Ts>
      using type = tracker<Ts...>;
    };
  
    template <typename Tag>
    using tag_base_t = typename Tag::base;

    template <typename LocalDerived, typename RemoteDerived,
      typename RemoteTag, typename LocalParent = LocalDerived, 
      typename LocalTag = intrusive_reporter_tag>
    class intrusive_reporter;

    template <typename LocalDerived, typename RemoteDerived,
      typename RemoteTag, typename LocalParent = LocalDerived, 
      typename LocalTag = intrusive_tracker_tag>
    class intrusive_tracker;
    
    // support types
    
    template <template <typename...> class Type, class Alias>
    struct is_type : std::false_type
    { };
  
    template <template <typename...> class Type, class ...TemplateParams>
    struct is_type<Type, Type<TemplateParams...>> : std::true_type
    { };
    
    template <typename...>
    using map_void = void;
  
    // pretend like reporter_base doesn't exist
    template <typename ReporterCIter, typename RemoteParent>
    struct reporter_iterator
    {
    private:
      using reporter_citer     = ReporterCIter;
      using reporter_type      = typename ReporterCIter::value_type;
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
      reporter_iterator (Iter it)
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

//      constexpr auto& operator* (void) noexcept
//      {
//        return get_remote_parent();
//      }
  
      constexpr remote_parent_type& operator* (void) const noexcept
      {
        return get_remote_parent ();
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

//      constexpr auto& get_remote_parent (void) noexcept
//      {
//        return m_citer->get_remote_parent ();
//      }
      
      constexpr auto& get_remote_parent (void) const noexcept 
      {
        return m_citer->get_remote_parent ();
      }
      
      reporter_citer m_citer;
      
    };
  
    template <typename LocalDerived, typename RemoteDerived, typename LocalBaseTag>
    class reporter_base<LocalDerived, RemoteDerived,
                         LocalBaseTag, reporter_base_tag>
    {
    public:
  
      using local_type      = LocalDerived;
      using remote_type     = RemoteDerived;
      using local_tag_type  = LocalBaseTag;
      using remote_tag_type = reporter_base_tag;
  
      using remote_base_type = reporter_base<remote_type, local_type,
                                             remote_tag_type, local_tag_type>;
      
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
      
      constexpr remote_type& get_remote_reporter (void) const noexcept
      {
        return get_remote ();
      }
  
      [[nodiscard]]
      constexpr bool has_remote_parent (void) const noexcept
      {
        return has_remote () && get_remote ().has_parent ();
      }
  
      [[nodiscard]] // auto is actually necessary here
      constexpr auto& get_remote_parent (void) noexcept
      {
        return get_remote ().get_parent ();
      }
  
      [[nodiscard]] // auto is actually necessary here
      constexpr auto& get_remote_parent (void) const noexcept
      {
        return get_remote ().get_parent ();
      }
  
      void set_remote (remote_base_type& remote) noexcept
      {
        m_remote = &static_cast<remote_type&> (remote);
      }
  
    private:
  
      std::optional<remote_type *> m_remote;
  
    };
  
    template <typename LocalDerived, typename RemoteDerived, 
              typename LocalBaseTag>
    class reporter_base<LocalDerived, RemoteDerived,
                        LocalBaseTag, tracker_base_tag>
    {
    public:
      
    using local_type      = LocalDerived;  
    using remote_type     = RemoteDerived;
    using local_tag_type  = LocalBaseTag;
    using remote_tag_type = tracker_base_tag;
      
      using remote_base_type = reporter_base<remote_type, local_type,
                                             remote_tag_type, local_tag_type>;

    private:
      
      using self_iter = typename plf::list<remote_base_type>::iterator;
  
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
  
      constexpr remote_base_type& get_remote_reporter (void) const noexcept
      {
        return **get_self_iter ();
      }
      
      void orphan (void) noexcept
      {
        if (has_remote ())
          reset_self_iter ();
      }
  
      void set_remote (remote_base_type& remote) noexcept
      {
        m_data = { &remote, has_remote () ? get_self_iter () : std::nullopt };
      }
  
      [[nodiscard]]
      constexpr bool has_remote_parent (void) const noexcept
      {
        return has_remote () && get_remote ().has_parent ();
      }
  
      [[nodiscard]] // auto is actually necessary here
      constexpr auto& get_remote_parent (void) noexcept 
      {
        return get_remote ().get_parent ();
      }
  
      [[nodiscard]] // auto is actually necessary here
      constexpr auto& get_remote_parent (void) const noexcept
      {
        return get_remote ().get_parent ();
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
  
    //! Remote is one of the non-base classes defined here
    template <typename LocalDerived, typename RemoteInterface, 
              typename RemoteBaseTag>
    class tracker_base
    {
    public:
  
      using local_type      = LocalDerived;
      using remote_type     = typename RemoteInterface::derived_type;
      using local_tag_type  = tracker_base_tag;
      using remote_tag_type = RemoteBaseTag;
  
    protected:
  
      // Store pointers to the base types. Downcast when needed.
      using reporter_type   = reporter_base<local_type, remote_type,
                                            local_tag_type, remote_tag_type>;
      using reporter_list   = plf::list<reporter_type>;
      using internal_iter   = typename reporter_list::iterator;
      using internal_citer  = typename reporter_list::const_iterator;
      using internal_riter  = typename reporter_list::reverse_iterator;
      using internal_criter = typename reporter_list::const_reverse_iterator;
      using internal_ref    = typename reporter_list::reference;
      using internal_cref   = typename reporter_list::const_reference;
      
      using remote_reporter_type = typename reporter_type::remote_base_type;
      using remote_reporter_list = plf::list<remote_reporter_type>;
      using remote_internal_iter = typename remote_reporter_list::iterator;
  
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
      internal_iter track (remote_type& remote)
      {
        return m_reporters.emplace (m_reporters.end (), remote);
      }
  
      // safe, may throw
      internal_iter track (remote_type& remote, remote_internal_iter it)
      {
        return m_reporters.emplace (m_reporters.end (), remote, it);
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
  
    template <typename LocalDerived, typename RemoteParent, 
              typename RemoteTag, typename LocalParent, typename LocalTag>
    class intrusive_reporter
      : public reporter_base<LocalDerived,
                             typename RemoteTag::template type<RemoteParent,
                                                               LocalParent,
                                                               LocalTag>,
                             reporter_base_tag, tag_base_t<RemoteTag>>
    {
    public:
  
      using local_type = LocalDerived;
      using remote_interface_type 
        = typename RemoteTag::template type<RemoteParent, LocalParent, 
                                            LocalTag>;
      using local_tag_type  = LocalTag;
      using remote_tag_type = RemoteTag;
      using derived_type    = LocalDerived;
  
      using base = reporter_base<LocalDerived,
        typename RemoteTag::template type<RemoteParent, LocalParent, LocalTag>, 
        reporter_base_tag, tag_base_t<RemoteTag>>;
  
      intrusive_reporter            (void)                          = default;
      intrusive_reporter            (const intrusive_reporter&)     = default;
  //  intrusive_reporter            (intrusive_reporter&&) noexcept = impl;
      intrusive_reporter& operator= (const intrusive_reporter&)     = default;
  //  intrusive_reporter& operator= (intrusive_reporter&&) noexcept = impl;
  //  ~intrusive_reporter           (void)                          = impl;
  
      intrusive_reporter (intrusive_reporter&& other) noexcept
        : base (std::move (other))
      {
        if (this->is_tracked ())
          this->get_remote_reporter ().set_remote (*this);
      }
  
      intrusive_reporter& operator= (intrusive_reporter&& other) noexcept
      {
        if (&other != this)
          {
            base::operator= (std::move (other));
            if (this->is_tracked ())
              this->get_remote_reporter ().set_remote (*this);
          }
        return *this;
      }
  
      ~intrusive_reporter (void) noexcept
      {
        if (base::is_tracked ())
          base::orphan_remote ();
      }
  
      explicit intrusive_reporter (remote_interface_type& remote)
        : base (remote)
      { }
  
      void swap (intrusive_reporter& other) noexcept
      {
        if (&other.get_remote () != &this->get_remote ())
          {
            using std::swap;
            base::swap (other);
            
            if (this->is_tracked ())
              this->get_remote_reporter ().set_remote (*this);
  
            if (other.is_tracked ())
              other.get_remote_reporter ().set_remote (other);
          }
      }
  
      [[nodiscard]]
      constexpr bool has_parent (void) const noexcept
      {
        return true;
      }
  
      constexpr local_type& get_parent (void) noexcept
      {
        return static_cast<local_type&> (*this);
      }
  
      constexpr const local_type& get_parent (void) const noexcept
      {
        return static_cast<const local_type&> (*this);
      }
  
    };
  
    template <typename LocalDerived, typename RemoteParent, 
              typename RemoteTag, typename LocalParent, typename LocalTag>
    class intrusive_tracker
      : public tracker_base<LocalDerived,
                            typename RemoteTag::template type<RemoteParent, 
                                                              LocalParent,
                                                              LocalTag>,
                            tag_base_t<RemoteTag>>
    {
    public:
  
      using local_type = LocalDerived;
      using remote_type = typename RemoteTag::template type<RemoteParent,
                                                            LocalParent, 
                                                            LocalTag>;
      using remote_tag_type = RemoteTag;
      using derived_type    = LocalDerived;
  
      using base = tracker_base<LocalDerived,
        typename RemoteTag::template type<RemoteParent, LocalParent, LocalTag>, 
        tag_base_t<RemoteTag>>;
    
      using internal_iter = typename base::internal_iter;
      using internal_citer = typename base::internal_citer;
    
      intrusive_tracker            (void)                         = default;
      intrusive_tracker            (const intrusive_tracker&)     = delete;
      intrusive_tracker            (intrusive_tracker&&) noexcept = default;
      intrusive_tracker& operator= (const intrusive_tracker&)     = delete;
      intrusive_tracker& operator= (intrusive_tracker&&) noexcept = default;
      ~intrusive_tracker           (void)                         = default;
    
      using base::swap;
    
      using iter   = reporter_iterator<internal_citer, RemoteParent>;
      using citer  = iter;
      using riter  = std::reverse_iterator<iter>;
      using criter = std::reverse_iterator<citer>;
  //    using ref    = typename iter::reference;
  //    using cref   = typename citer::const_reference;
    
      iter   begin   (void)       noexcept { return base::internal_begin (); }
      citer  begin   (void) const noexcept { return base::internal_begin (); }
      citer  cbegin  (void) const noexcept { return base::internal_cbegin (); }
    
      iter   end     (void)       noexcept { return base::internal_end (); }
      citer  end     (void) const noexcept { return base::internal_end (); }
      citer  cend    (void) const noexcept { return base::internal_cend (); }
    
      riter  rbegin  (void)       noexcept { return base::internal_rbegin (); }
      criter rbegin  (void) const noexcept { return base::internal_rbegin (); }
      criter crbegin (void) const noexcept { return base::internal_crbegin (); }
    
      riter  rend    (void)       noexcept { return base::internal_rend (); }
      criter rend    (void) const noexcept { return base::internal_rend (); }
      criter crend   (void) const noexcept { return base::internal_crend (); }
    
      auto   front   (void)                { return *begin (); }
      auto   front   (void) const          { return *begin (); }
  
      auto   back    (void)                { return *--end (); }
      auto   back    (void) const          { return *--end (); }
    
      bool has_reporters (void) const noexcept
      {
        return ! base::empty ();
      }
    
      void transfer_from (intrusive_tracker&& src, citer pos) noexcept
      {
        return base::internal_transfer_from (std::move (src), pos);
      }
      
      void transfer_from (intrusive_tracker& src, citer pos) noexcept
      {
        return base::internal_transfer_from (src, pos);
      }
      
      template <typename T>
      void transfer_from (T&& src) noexcept
      {
        return base::internal_transfer_from (std::forward<T> (src));
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
      
      constexpr local_type& get_parent (void) noexcept
      {
        return static_cast<local_type&> (*this);
      }
  
      constexpr const local_type& get_parent (void) const noexcept
      {
        return static_cast<const local_type&> (*this);
      }
      
      std::size_t get_offset (citer pos) const noexcept
      {
        return std::distance (base::internal_cbegin (),
                              internal_citer (pos));
      }
      
    private:
      // only available to tracker constructors
      
    };
  
    //! non-intrusive; for use as a class member
    template <typename LocalParent, typename RemoteParent,
      typename RemoteTag>
    class reporter
      : public intrusive_reporter<reporter<LocalParent, RemoteParent, 
                                           RemoteTag>, 
                                  RemoteParent, RemoteTag, LocalParent, 
                                  reporter_tag>
    {
    public:
  
      using parent_type = LocalParent;
      using remote_type = typename RemoteTag::template type<RemoteParent,
                                                            LocalParent,
                                                            reporter_tag>;
      using remote_tag_type = RemoteTag;
      using derived_type    = reporter;
  
      using base = intrusive_reporter<reporter, RemoteParent, RemoteTag, 
                                      LocalParent, reporter_tag>;
  
  //    static_assert (std::is_base_of<remote_base_type, remote_type>::value,
  //                   "tracker base has unexpected type");
  
      reporter            (void)                = default;
      reporter            (const reporter&)     = delete;
      reporter            (reporter&&) noexcept = delete;
  //  reporter& operator= (const reporter&)     = impl;
  //  reporter& operator= (reporter&&) noexcept = impl;
      ~reporter           (void)                = default;
  
      explicit reporter (remote_type& remote)
        : base (remote)
      { }
  
      explicit reporter (remote_type& remote, parent_type& parent)
        : base (remote),
          m_parent (&parent)
      { }
  
      reporter (const reporter& other, parent_type& new_parent)
        : base (other),
          m_parent (&new_parent)
      { }
  
      reporter (reporter&& other, parent_type& new_parent) noexcept
        : base (std::move (other)),
          m_parent (&new_parent)
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
      
      [[nodiscard]]
      constexpr bool has_parent (void) const noexcept 
      {
        return m_parent.has_value ();
      }
      
      [[nodiscard]]
      constexpr parent_type& get_parent (void) const noexcept
      {
        return **m_parent;
      }
  
    private:
      
      std::optional<parent_type *> m_parent;
      
    };
  
    template <typename LocalParent, typename RemoteParent, typename RemoteTag>
    class tracker<LocalParent, RemoteParent, RemoteTag,
                  std::enable_if_t<std::is_base_of<reporter_base_tag, 
                                                   RemoteTag>::value>>
      : public intrusive_tracker<tracker<LocalParent, RemoteParent, RemoteTag>, 
                                 RemoteParent, RemoteTag, LocalParent, 
                                 tracker_tag>
    {
    public:
  
      using parent_type     = LocalParent;
      using remote_type     = RemoteParent;
      using remote_tag_type = reporter_tag;
      using derived_type    = tracker;
  
      using base = intrusive_tracker<tracker, RemoteParent, RemoteTag, 
                                     LocalParent, tracker_tag>;
  
      tracker            (void)               = default;
      tracker            (const tracker&)     = delete;
      tracker            (tracker&&) noexcept = default;
      tracker& operator= (const tracker&)     = delete;
  //  tracker& operator= (tracker&&) noexcept = impl;
      ~tracker           (void)               = default;
      
      explicit tracker (parent_type& parent)
        : m_parent (&parent)
      { }
  
      tracker (const tracker& other, parent_type& new_parent) = delete;
  
      tracker (tracker&& other, parent_type& new_parent) noexcept
        : base (std::move (other)),
          m_parent (&new_parent)
      { }
      
      // explicitly defined so that m_parent doesn't get implicitly reset
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
  
      [[nodiscard]]
      constexpr bool has_parent (void) const noexcept
      {
        return m_parent.has_value ();
      }
  
      [[nodiscard]]
      constexpr parent_type& get_parent (void) const noexcept
      {
        return **m_parent;
      }
      
    protected:
      
  //    explicit tracker (local_parent_type *parent)
  //      : m_parent (parent)
  //    { }
  
    private:
      
      std::optional<parent_type *> m_parent;
  
    };
  
    template <typename LocalParent, typename RemoteParent, typename RemoteTag>
    class tracker<LocalParent, RemoteParent, RemoteTag,
                  std::enable_if_t<std::is_base_of<tracker_base_tag,
                                                   RemoteTag>::value>>
      : public intrusive_tracker<tracker<LocalParent, RemoteParent, RemoteTag>, 
                                 RemoteParent, RemoteTag, LocalParent, 
                                 tracker_tag>
    {
    public:
  
      using parent_type     = LocalParent;
      using remote_tag_type = tracker_tag;
      using derived_type    = tracker;
  
      using base = intrusive_tracker<tracker, RemoteParent, RemoteTag, 
                                     LocalParent, tracker_tag>;

      using remote_type     = typename base::remote_type;
      
    private:
      
      using internal_iter = typename base::internal_iter;
      using internal_citer = typename base::internal_citer;
      
      using citer = typename base::citer;
      
    public:
  
      tracker            (void)               = default;
  //  tracker            (const tracker&)     = impl;
      tracker            (tracker&&) noexcept = default;
  //  tracker& operator= (const tracker&)     = impl;
      tracker& operator= (tracker&&) noexcept = default;
      ~tracker           (void)               = default;
  
      explicit tracker (parent_type& parent)
        : base (parent)
      { }
  
      explicit tracker (parent_type& parent, remote_type& remote)
        : base (parent)
      {
        internal_bind (remote);
      }
  
      tracker (const tracker& other)
        : base ()
      {
        copy_reporters (other);
      }
  
      tracker (const tracker& other, parent_type& parent)
        : base (parent)
      {
        copy_reporters (other);
      }
  
      tracker (tracker&& other, parent_type& parent) noexcept
        : base (std::move (other), parent)
      { }
  
      tracker& operator= (const tracker& other)
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
  
      void swap (tracker& other) noexcept
      {
        base::swap (other);
      }
  
      citer copy_reporters (const tracker& other)
      {
        return internal_copy_reporters (other);
      }
  
      citer overwrite_reporters (const tracker& other)
      {
        return this->untrack (this->internal_begin (),
                              internal_copy_reporters (other));
      }
      
    private:
  
      internal_iter internal_copy_reporters (const tracker& other)
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
    void bind (tracker<Types...>& l, 
               typename tracker<Types...>::remote_type& r)
    {
      l.bind (r);
    }
  
//    template <typename Derived, typename RemoteParent>
//    using intusive_reporter_nt = intrusive_reporter<
//      Derived, tracker<RemoteParent, Derived>>;
//  
//    template <typename LocalParent, typename RemoteParent>
//    struct reporter_nt;
//  
//    template <typename LocalParent, typename RemoteParent>
//    struct reporter_nt;
//  
//    template <typename LocalParent, typename RemoteParent>
//    struct reporter_nt 
//      : reporter<LocalParent, 
//                 tracker<RemoteParent, reporter_nt<LocalParent, RemoteParent>>>
//    { };
  }
  
  using detail::intrusive_reporter_tag;
  using detail::intrusive_tracker_tag;
  using detail::reporter_tag;
  using detail::tracker_tag;

  template <typename LocalDerived, typename RemoteParent, 
            typename RemoteTag = tracker_tag>
  using intrusive_reporter = detail::intrusive_reporter<LocalDerived,
                                                        RemoteParent, 
                                                        RemoteTag>;

  template <typename LocalDerived, typename RemoteParent, 
            typename RemoteTag = reporter_tag>
  using intrusive_tracker = detail::intrusive_tracker<LocalDerived, 
                                                      RemoteParent, 
                                                      RemoteTag>;

  template <typename LocalParent, typename RemoteParent,
            typename RemoteTag = tracker_tag>
  using reporter = detail::reporter<LocalParent, RemoteParent, RemoteTag>;

  template <typename LocalParent, typename RemoteParent, 
            typename RemoteTag = reporter_tag>
  using tracker = detail::tracker<LocalParent, RemoteParent, RemoteTag>;
  
  template <typename LocalParent, typename RemoteParent = LocalParent>
  using multireporter = tracker<LocalParent, RemoteParent, tracker_tag>;
  
  using detail::bind;
  
}

#endif
