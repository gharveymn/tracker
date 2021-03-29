/** common.hpp
 * Copyright © 2021 Gene Harvey
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef OCTAVE_IR_COMMON_HPP
#define OCTAVE_IR_COMMON_HPP

#ifdef __clang__
#  ifndef GCH_CLANG
#    define GCH_CLANG
#  endif
#  if defined (__cplusplus) && __cplusplus >= 201103L
#    ifndef GCH_CLANG_11
#      define GCH_CLANG_11
#    endif
#  endif
#  if defined (__cplusplus) && __cplusplus >= 201402L
#    ifndef GCH_CLANG_14
#      define GCH_CLANG_14
#    endif
#  endif
#  if defined (__cplusplus) && __cplusplus >= 201703L
#    ifndef GCH_CLANG_17
#      define GCH_CLANG_17
#    endif
#  endif
#  if defined (__cplusplus) && __cplusplus >= 202002L
#    ifndef GCH_CLANG_20
#      define GCH_CLANG_20
#    endif
#  endif
#endif

#ifndef GCH_CPP14_CONSTEXPR
#  if defined (__cpp_constexpr) && __cpp_constexpr >= 201304L
#    define GCH_CPP14_CONSTEXPR constexpr
#    ifndef GCH_HAS_CPP14_CONSTEXPR
#      define GCH_HAS_CPP14_CONSTEXPR
#    endif
#  else
#    define GCH_CPP14_CONSTEXPR
#  endif
#endif

#ifndef GCH_CPP17_CONSTEXPR
#  if defined (__cpp_constexpr) && __cpp_constexpr >= 201603L
#    define GCH_CPP17_CONSTEXPR constexpr
#    ifndef GCH_HAS_CPP17_CONSTEXPR
#      define GCH_HAS_CPP17_CONSTEXPR
#    endif
#  else
#    define GCH_CPP17_CONSTEXPR
#  endif
#endif

#ifndef GCH_CPP20_CONSTEXPR
#  if defined (__cpp_constexpr) && __cpp_constexpr >= 201907L
#    define GCH_CPP20_CONSTEXPR constexpr
#    ifndef GCH_HAS_CPP20_CONSTEXPR
#      define GCH_HAS_CPP20_CONSTEXPR
#    endif
#  else
#    define GCH_CPP20_CONSTEXPR
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

#ifndef GCH_INLINE_VARIABLE
#  if defined (__cpp_inline_variables) && __cpp_inline_variables >= 201606L
#    define GCH_INLINE_VARIABLE inline
#  else
#    define GCH_INLINE_VARIABLE
#  endif
#endif

#if defined (__cpp_impl_three_way_comparison) && __cpp_impl_three_way_comparison >= 201907L
#  ifndef GCH_IMPL_THREE_WAY_COMPARISON
#    define GCH_IMPL_THREE_WAY_COMPARISON
#  endif
#endif

#include <plf_list.h>

#include <algorithm>
#include <iterator>

#ifdef GCH_IMPL_THREE_WAY_COMPARISON
#  if defined (__has_include) && __has_include (<compare>)
#    include <compare>
#  endif
#endif

#if defined (__cpp_lib_three_way_comparison) && __cpp_lib_three_way_comparison >= 201907L
#  ifndef GCH_LIB_THREE_WAY_COMPARISON
#    define GCH_LIB_THREE_WAY_COMPARISON
#  endif
#endif

namespace gch
{
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
    GCH_INLINE_VARIABLE constexpr bind_t bind { bind_t::create };

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

    template <typename ...Ts>
    using tracker_container = plf::list<Ts...>;

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
        using reporter_type = detail::reporter_base<LocalBaseTag, RemoteBaseTag>;

        template <typename>
        using access_type = void;

        template <typename>
        using const_access_type = void;

        template <typename LocalBaseTag, typename RemoteBaseTag>
        using base_type = detail::reporter_base<LocalBaseTag, RemoteBaseTag>;
      };

      struct tracker_base
      {
        using base_tag = tracker_base;

        template <typename, typename RemoteBaseTag>
        using reporter_type = detail::reporter_base<detail::tag::tracker_base, RemoteBaseTag>;

        template <typename RemoteBaseTag>
        using access_type = typename tracker_container<
          reporter_type<detail::tag::tracker_base, RemoteBaseTag>>::iterator;

        template <typename RemoteBaseTag>
        using const_access_type = typename tracker_container<
          reporter_type<detail::tag::tracker_base, RemoteBaseTag>>::const_iterator;

        template <typename, typename RemoteBaseTag>
        using base_type = detail::tracker_base<RemoteBaseTag>;
      };

      template <typename Parent, typename IntrusiveTag>
      struct reporter
        : reporter_base
      {
        using reduced_tag = reporter;

        template <typename RemoteTag>
        using parent_type = Parent;

        template <typename RemoteTag>
        using interface_type = gch::reporter<Parent, RemoteTag, IntrusiveTag>;

        template <typename RemoteTag>
        using common_type = detail::reporter_common<interface_type<RemoteTag>>;
      };

      template <typename Parent, typename IntrusiveTag>
      struct tracker
        : tracker_base
      {
        using reduced_tag = tracker;

        template <typename RemoteTag>
        using parent_type = Parent;

        template <typename RemoteTag>
        using interface_type = gch::tracker<Parent, RemoteTag, IntrusiveTag>;

        template <typename RemoteTag>
        using common_type = detail::tracker_common<interface_type<RemoteTag>>;
      };

      struct standalone_reporter
        : reporter_base
      {
        using reduced_tag = standalone_reporter;

        template <typename RemoteTag>
        using parent_type = gch::standalone_reporter<RemoteTag>;

        template <typename RemoteTag>
        using interface_type = gch::reporter<gch::standalone_reporter<RemoteTag>, RemoteTag,
                                             gch::tag::intrusive>;

        template <typename RemoteTag>
        using common_type = detail::reporter_common<interface_type<RemoteTag>>;
      };

      struct standalone_tracker
        : tracker_base
      {
        using reduced_tag = standalone_tracker;

        template <typename RemoteTag>
        using parent_type = gch::standalone_tracker<RemoteTag>;

        template <typename RemoteTag>
        using interface_type = gch::tracker<gch::standalone_tracker<RemoteTag>, RemoteTag,
                                            gch::tag::intrusive>;

        template <typename RemoteTag>
        using common_type = detail::tracker_common<interface_type<RemoteTag>>;
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
      GCH_INLINE_VARIABLE constexpr struct track_t { track_t (void) = default; } track;

    } // namespace gch::detail::tag

  } // namespace gch::detail

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

    } // namespace gch::remote::intrusive

  } // namespace gch::remote

  namespace detail
  {

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
    };

  } // namespace gch::detail

  template <typename T>
  struct tracker_traits;

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
    friend class detail::reporter_common;

    template <typename Interface>
    friend class detail::tracker_common;

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
      return lhs.m_iter == rhs.m_iter;
    }

#ifndef GCH_IMPL_THREE_WAY_COMPARISON

    friend
    bool
    operator!= (const tracker_iterator& lhs, const tracker_iterator& rhs) noexcept
    {
      return lhs.m_iter != rhs.m_iter;
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

} // namespace gch

#endif // OCTAVE_IR_COMMON_HPP
