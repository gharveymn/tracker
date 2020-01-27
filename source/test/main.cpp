#include "tracker.hpp"

#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <random>
#include <list>
#include <sstream>
#include <plf_list.h>

using namespace gch;

#ifndef NDEBUG
constexpr std::size_t multiplier = 1;
#else
constexpr std::size_t multiplier = 100;
#endif

template <typename RemoteTag>
struct ichild_r : reporter<ichild_r<RemoteTag>, RemoteTag, tag::intrusive>
{
  using base = reporter<ichild_r, RemoteTag, tag::intrusive>;

  ichild_r (void) = default;

  explicit ichild_r (typename base::remote_interface_type& remote)
    : base (tag::bind, remote)
  { }
};

template <typename RemoteTag>
struct nchild_r
{
  using reporter_type = reporter<nchild_r, RemoteTag>;

  nchild_r (void) = default;

  explicit nchild_r (typename reporter_type::remote_interface_type& remote)
    : m_reporter (*this, remote)
  { }

  nchild_r (nchild_r&& other) noexcept
    : m_reporter (std::move (other.m_reporter), *this)
  { }

  reporter_type m_reporter;

};

template <typename RemoteTag>
struct ichild_t : tracker<ichild_t<RemoteTag>, RemoteTag, tag::intrusive>
{
  using base = tracker<ichild_t, RemoteTag, tag::intrusive>;

  ichild_t (void) = default;

  explicit ichild_t (typename base::remote_interface_type&)
  { }

};

template <typename RemoteTag>
struct nchild_t
{
  using tracker_type = tracker<nchild_t, RemoteTag>;

  nchild_t (void) = default;

  explicit nchild_t (typename tracker_type::remote_interface_type&)
    : m_tracker (*this)
  { }

  nchild_t (nchild_t&& other) noexcept
    : m_tracker (std::move (other.m_tracker), *this)
  { }

  tracker_type m_tracker;

};

template <typename LocalTag, template <typename ...> class Child,
  template <typename, template <typename ...> class = plf::list> class TRemoteTag>
using base_type = typename LocalTag::template interface_type<TRemoteTag<Child<LocalTag>>>;

template <template <typename ...> class Child,
  template <typename, template <typename ...> class> class TRemoteTag>
struct iparent_r
  : base_type<tag::intrusive::reporter<iparent_r<Child, TRemoteTag>>, Child, TRemoteTag>
{

  using child_type = Child<tag::intrusive::reporter<iparent_r>>;

  iparent_r (void) = default;

};

template <template <typename ...> class Child, template <typename, template <typename ...> class> class TRemoteTag>
struct nparent_r
{

  using child_type = Child<tag::nonintrusive::reporter<nparent_r>>;

  nparent_r (void) = default;

  base_type<tag::nonintrusive::reporter<nparent_r>, Child, TRemoteTag> m_reporter;

};

template <template <typename ...> class Child, template <typename, template <typename ...> class> class TRemoteTag>
struct iparent_t
  : base_type<tag::intrusive::tracker<iparent_t<Child, TRemoteTag>>, Child, TRemoteTag>
{

  using child_type = Child<tag::intrusive::tracker<iparent_t>>;

  iparent_t (void) = default;

  child_type create (void)
  {
    return child_type (*this);
  }

};

template <template <typename ...> class Child, template <typename, template <typename ...> class> class TRemoteTag>
struct nparent_t
{

  using child_type = Child<tag::nonintrusive::tracker<nparent_t>>;

  nparent_t (void) = default;

  child_type create (void)
  {
    return child_type (m_tracker);
  }

  base_type<tag::nonintrusive::tracker<nparent_t>, Child, TRemoteTag> m_tracker;

};

template struct iparent_r<ichild_r, tag::intrusive::reporter   >;
template struct iparent_r<nchild_r, tag::nonintrusive::reporter>;
template struct iparent_r<ichild_t, tag::intrusive::tracker    >;
template struct iparent_r<nchild_t, tag::nonintrusive::tracker >;

template struct nparent_r<ichild_r, tag::intrusive::reporter   >;
template struct nparent_r<nchild_r, tag::nonintrusive::reporter>;
template struct nparent_r<ichild_t, tag::intrusive::tracker    >;
template struct nparent_r<nchild_t, tag::nonintrusive::tracker >;

template struct iparent_t<ichild_r, tag::intrusive::reporter   >;
template struct iparent_t<nchild_r, tag::nonintrusive::reporter>;
template struct iparent_t<ichild_t, tag::intrusive::tracker    >;
template struct iparent_t<nchild_t, tag::nonintrusive::tracker >;

template struct nparent_t<ichild_r, tag::intrusive::reporter   >;
template struct nparent_t<nchild_r, tag::nonintrusive::reporter>;
template struct nparent_t<ichild_t, tag::intrusive::tracker    >;
template struct nparent_t<nchild_t, tag::nonintrusive::tracker >;

template struct ichild_r<tag::intrusive::reporter   <iparent_r<ichild_r, tag::intrusive::reporter>>>;
template struct ichild_r<tag::nonintrusive::reporter<nparent_r<ichild_r, tag::intrusive::reporter>>>;
template struct ichild_r<tag::intrusive::tracker    <iparent_t<ichild_r, tag::intrusive::reporter>>>;
template struct ichild_r<tag::nonintrusive::tracker <nparent_t<ichild_r, tag::intrusive::reporter>>>;

template struct nchild_r<tag::intrusive::reporter   <iparent_r<nchild_r, tag::nonintrusive::reporter>>>;
template struct nchild_r<tag::nonintrusive::reporter<nparent_r<nchild_r, tag::nonintrusive::reporter>>>;
template struct nchild_r<tag::intrusive::tracker    <iparent_t<nchild_r, tag::nonintrusive::reporter>>>;
template struct nchild_r<tag::nonintrusive::tracker <nparent_t<nchild_r, tag::nonintrusive::reporter>>>;

template struct ichild_t<tag::intrusive::reporter   <iparent_r<ichild_t, tag::intrusive::tracker>>>;
template struct ichild_t<tag::nonintrusive::reporter<nparent_r<ichild_t, tag::intrusive::tracker>>>;
template struct ichild_t<tag::intrusive::tracker    <iparent_t<ichild_t, tag::intrusive::tracker>>>;
template struct ichild_t<tag::nonintrusive::tracker <nparent_t<ichild_t, tag::intrusive::tracker>>>;

template struct nchild_t<tag::intrusive::reporter   <iparent_r<nchild_t, tag::nonintrusive::tracker>>>;
template struct nchild_t<tag::nonintrusive::reporter<nparent_r<nchild_t, tag::nonintrusive::tracker>>>;
template struct nchild_t<tag::intrusive::tracker    <iparent_t<nchild_t, tag::nonintrusive::tracker>>>;
template struct nchild_t<tag::nonintrusive::tracker <nparent_t<nchild_t, tag::nonintrusive::tracker>>>;

// template struct nchild_t<iparent_r<nchild_t, tracker>, tag::intrusive::reporter>;
// template struct nchild_t<nparent_r<nchild_t, tracker>, reporter>;
// template struct nchild_t<iparent_t<nchild_t, tracker>, tag::intrusive::tracker>;
// template struct nchild_t<nparent_t<nchild_t, tracker>, tracker>;

class parent;
class child;

class child : public intrusive_reporter<child, remote::tracker<parent>>
{
public:

  using base = intrusive_reporter<child, remote::tracker<parent>>;

  child (void) = default;

  child (std::nullptr_t, std::string s)
    : m_name (std::move (s))
  { }

  child (typename base::remote_interface_type& tkr)
    : base (tag::bind, tkr)
  { }

  child (typename base::remote_interface_type& tkr, std::string s)
    : base (tag::bind, tkr),
      m_name (std::move (s))
  { }

  child (const child& o)
    : base (o.clone ()),
      m_name (o.m_name)
  { }

  child (child&& o) noexcept
    : base (std::move (o)),
      m_name (std::move (o.m_name))
  { }

  child& operator= (const child& other)
  {
    if (&other != this)
      {
        reporter::operator= (other.clone ());
        m_name = other.m_name;
      }
    return *this;
  }

  child& operator= (child&& other) noexcept
  {
    m_name = std::move (other.m_name);
    reporter::operator= (std::move (other));
    return *this;
  }

  friend std::ostream& operator<< (std::ostream& o, const child& c)
  {
    return o << c.m_name;
  }

  std::size_t f (std::size_t x) const
  {
    return x % 17;
  }

  void rebind (parent& p);

private:
  std::string m_name;
};

class parent
{
public:

  using reporter_type = child;
  using tracker_type  = tracker<parent, remote::reporter<child, tag::intrusive>>;

  parent (void)
    : m_children (*this)
  { }

  explicit parent (std::string s)
    : m_children (*this),
      m_name (std::move (s))
  { }

//  child create (std::string s)
//  {
//    return { std::move (s, *this) };
//  }

  friend void child::rebind (parent&);

  void transfer_from (parent& other)
  {
    m_children.transfer_bindings (other.m_children);
  }

  template <typename ...Args>
  child create (Args&&... args)
  {
    return { m_children, std::forward<Args> (args)... };
  }

  friend std::ostream& operator<< (std::ostream& o, const parent& p)
  {
    o << p.m_name << ": { ";
    for (const child& c : p.m_children)
      o << c << " ";
    return o << "}";
  }

  std::size_t num_children (void) noexcept
  {
    return m_children.num_reporters ();
  }

  tracker_type::citer find (const child& c)
  {
    return std::find_if (m_children.begin (), m_children.end (),
                         [&c](const child& x) { return &x == &c; });
  }

  tracker_type::iter begin (void) noexcept { return m_children.begin (); }
  tracker_type::iter end (void)   noexcept { return m_children.end (); }

private:
  tracker_type m_children;
  std::string m_name;
};

void child::rebind (parent& p)
{
  base::rebind (p.m_children);
}

template class gch::tracker<int, tag::intrusive::reporter<int>, tag::intrusive>;
template class gch::tracker<int, tag::nonintrusive::reporter<int>, tag::intrusive>;
template class gch::tracker<int, tag::intrusive::tracker <int>, tag::intrusive>;
template class gch::tracker<int, tag::nonintrusive::tracker            <int>, tag::intrusive>;
template class gch::tracker<int, tag::intrusive::reporter<int>, tag::nonintrusive>;
template class gch::tracker<int, tag::nonintrusive::reporter           <int>, tag::nonintrusive>;
template class gch::tracker<int, tag::intrusive::tracker <int>, tag::nonintrusive>;
template class gch::tracker<int, tag::nonintrusive::tracker            <int>, tag::nonintrusive>;

template class gch::reporter<int, tag::intrusive::reporter<int>, tag::intrusive>;
template class gch::reporter<int, tag::nonintrusive::reporter           <int>, tag::intrusive>;
template class gch::reporter<int, tag::intrusive::tracker <int>, tag::intrusive>;
template class gch::reporter<int, tag::nonintrusive::tracker            <int>, tag::intrusive>;
template class gch::reporter<int, tag::intrusive::reporter<int>, tag::nonintrusive>;
template class gch::reporter<int, tag::nonintrusive::reporter           <int>, tag::nonintrusive>;
template class gch::reporter<int, tag::intrusive::tracker <int>,  tag::nonintrusive>;
template class gch::reporter<int, tag::nonintrusive::tracker            <int>, tag::nonintrusive>;

class nonintruded_child_s;
class nonintruded_child;

template <typename T>
class nonintruded_parent_temp;

using nonintruded_parent = nonintruded_parent_temp<nonintruded_child>;
using nonintruded_parent_s = nonintruded_parent_temp<nonintruded_child_s>;

class nonintruded_child_s
{
public:

  using reporter_type = reporter<nonintruded_child_s, remote::tracker<nonintruded_parent_s>>;

  nonintruded_child_s (reporter_type::remote_interface_type& remote, std::string name)
    : m_reporter (*this, remote),
      m_name (std::move (name))
  { }

  nonintruded_child_s (const nonintruded_child_s& other)
    : m_reporter (other.m_reporter.clone (), *this),
      m_name (other.m_name)
  { }

  nonintruded_child_s (nonintruded_child_s&& other) noexcept
    : m_reporter (std::move (other.m_reporter), *this),
      m_name (std::move (other.m_name))
  { }

  nonintruded_child_s& operator= (const nonintruded_child_s& other)
  {
    if (&other != this)
      {
        m_reporter.replace_binding (other.m_reporter);
        m_name = other.m_name;
      }
    return *this;
  }

  nonintruded_child_s& operator= (nonintruded_child_s&& other) noexcept
  {
    m_reporter.replace_binding (std::move (other.m_reporter));
    m_name = std::move (other.m_name);
    return *this;
  }

  friend std::ostream& operator<< (std::ostream& o, const nonintruded_child_s& c)
  {
    return o << c.m_name;
  }

  std::size_t get_position (void) const noexcept
  {
    return m_reporter.get_position ();
  }

  std::size_t f (std::size_t x) const
  {
    return x % 17;
  }

  void rebind (nonintruded_parent_s& p);

private:
  reporter_type m_reporter;
  std::string m_name;
};

class nonintruded_child
{
public:

  using reporter_type = reporter<nonintruded_child, remote::tracker<nonintruded_parent>>;

  nonintruded_child (reporter_type::remote_interface_type& p)
    : m_reporter (*this, p)
  { }

  nonintruded_child (const nonintruded_child& other)
    : m_reporter (other.m_reporter.clone (), *this)
  { }

  nonintruded_child (nonintruded_child&& other) noexcept
    : m_reporter (std::move (other.m_reporter), *this)
  { }

  nonintruded_child& operator= (const nonintruded_child& other)
  {
    if (&other != this)
      m_reporter.replace_binding (other.m_reporter);
    return *this;
  }

  nonintruded_child& operator= (nonintruded_child&& other) noexcept
  {
    m_reporter.replace_binding (std::move (other.m_reporter));
    return *this;
  }

  friend std::ostream& operator<< (std::ostream& o, const nonintruded_child&)
  {
    return o;
  }

  std::size_t get_position (void) const noexcept
  {
    return m_reporter.get_position ();
  }

  std::size_t f (std::size_t x) const
  {
    return x % 17;
  }

private:
  reporter_type m_reporter;
};

template <typename T>
class nonintruded_parent_temp
{
public:

  using reporter_type = typename T::reporter_type;
  using tracker_type  = typename reporter_type::remote_interface_type;

  nonintruded_parent_temp (void)
    : m_children (*this)
  { }

  explicit nonintruded_parent_temp (std::string s)
    : m_children (*this),
      m_name (std::move (s))
  { }

  friend void nonintruded_child_s::rebind (nonintruded_parent_s&);

//  child create (std::string s)
//  {
//    return { std::move (s, *this) };
//  }

  void transfer_from (nonintruded_parent_temp& other)
  {
    m_children.transfer_bindings (other.m_children);
  }

  template <typename ...Args>
  T create (Args... args)
  {
    return { m_children, std::forward<Args> (args)... };
  }

  friend std::ostream& operator<< (std::ostream& o, const nonintruded_parent_temp& p)
  {
    o << p.m_name << ": { ";
    for (const T& c : p.m_children)
      o << c << " ";
    return o << "}";
  }

  std::size_t num_children (void) noexcept
  {
    return m_children.num_reporters ();
  }

  typename tracker_type::iter begin (void) noexcept { return m_children.begin (); }
  typename tracker_type::iter end (void)   noexcept { return m_children.end (); }

private:
  tracker_type m_children;
  std::string m_name;
};

void nonintruded_child_s::rebind (nonintruded_parent_s& p)
{
  m_reporter.rebind (p.m_children);
}

class self_parent
{
public:

  self_parent (void) = delete;

  explicit self_parent (std::string name)
    : m_tracker (*this),
      m_name (std::move(name))
  { }

  self_parent (const self_parent& other) = delete;

//  self_parent (const self_parent& other)
//    : m_tracker (this, other.m_tracker)
//  { }

  self_parent (self_parent&& other) noexcept
    : m_tracker (std::move (other.m_tracker), *this),
      m_name (std::move (other.m_name))
  { }

  self_parent& operator= (const self_parent& other) = delete;


//  ~self_parent (void)
//  {
////    std::cout << "destroying " << m_name << std::endl;
//  }

//  self_parent& operator= (const self_parent& other)
//  {
//    if (&other != this)
//      m_tracker = other.m_tracker;
//    return *this;
//  }

  friend void bind (self_parent& l, self_parent& r)
  {
    bind (l.m_tracker, r.m_tracker);
  }

  std::size_t num_reporters (void) const noexcept
  {
    return m_tracker.num_reporters ();
  }

  self_parent& operator= (self_parent&& other) noexcept
  {
    m_tracker.transfer_replace_bindings (other.m_tracker);
    m_name = std::move (other.m_name);
    return *this;
  }

  friend std::ostream& operator<< (std::ostream& o, const self_parent& p)
  {
    o << p.m_name << ": { ";
    for (const self_parent& c : p.m_tracker)
      o << c.m_name << " ";
    return o << "}";
  }

private:

  multireporter<self_parent> m_tracker;
  std::string m_name;

};

class anon_self_parent
{
public:

  explicit anon_self_parent (void)
    : m_tracker (*this)
  { }

  anon_self_parent (const anon_self_parent& other) = delete;

  anon_self_parent (anon_self_parent&& other) noexcept
    : m_tracker (std::move (other.m_tracker), *this)
  { }

  anon_self_parent& operator= (const anon_self_parent& other) = delete;

  friend void bind (anon_self_parent& l, anon_self_parent& r)
  {
    bind (l.m_tracker, r.m_tracker);
  }

  anon_self_parent& operator= (anon_self_parent&& other) noexcept
  {
    m_tracker.transfer_replace_bindings (other.m_tracker);
    return *this;
  }

  void clear_tracker (void) noexcept
  {
    return m_tracker.wipe ();
  }

  friend std::ostream& operator<< (std::ostream& o, const anon_self_parent& p)
  {
    return o << p.m_tracker.num_reporters ();
  }

private:

  multireporter<anon_self_parent> m_tracker;

};

class anon1;
class anon2;

class anon1
{
public:

  explicit anon1 (void)
    : m_tracker (*this)
  { }

  anon1 (const anon_self_parent& other) = delete;

  anon1 (anon1&& other) noexcept
    : m_tracker (std::move (other.m_tracker), *this)
  { }

  anon1& operator= (const anon1& other) = delete;

  void bind (anon2& r);

  anon1& operator= (anon1&& other) noexcept
  {
    m_tracker.transfer_replace_bindings (other.m_tracker);
    return *this;
  }

  void clear_tracker (void) noexcept
  {
    return m_tracker.wipe ();
  }

  friend std::ostream& operator<< (std::ostream& o, const anon1& p)
  {
    return o << p.m_tracker.num_reporters ();
  }

private:

  multireporter<anon1, anon2> m_tracker;

};

class anon2
{
public:

  explicit anon2 (void)
    : m_tracker (*this)
  { }

  anon2 (const anon2& other) = delete;

  anon2 (anon2&& other) noexcept
    : m_tracker (std::move (other.m_tracker), *this)
  { }

  anon2& operator= (const anon2& other) = delete;

  void bind (anon1& r)
  {
    r.bind (*this);
  }

  friend void anon1::bind (anon2& r);

  anon2& operator= (anon2&& other) noexcept
  {
    m_tracker.transfer_replace_bindings (other.m_tracker);
    return *this;
  }

  void clear_tracker (void) noexcept
  {
    return m_tracker.wipe ();
  }

  friend std::ostream& operator<< (std::ostream& o, const anon2& p)
  {
    return o << p.m_tracker.num_reporters ();
  }

private:

  multireporter<anon2, anon1> m_tracker;

};

void anon1::bind (anon2& r)
{
  m_tracker.bind (r.m_tracker);
}

class named1;
class named2;

class named1
{
public:

  explicit named1 (std::string name)
    : m_tracker (*this),
      m_name (std::move (name))
  { }

  named1 (const named1& other)
    : m_tracker (other.m_tracker.clone (), *this),
      m_name (other.m_name)
  { }

  named1 (named1&& other) noexcept
    : m_tracker (std::move (other.m_tracker), *this),
      m_name (std::move (other.m_name))
  { }

  named1& operator= (const named1& other)
  {
    if (&other != this)
      {
        m_tracker.copy_replace_bindings (other.m_tracker);
        m_name = other.m_name;
      }
    return *this;
  }

  named1& operator= (named1&& other) noexcept
  {
    m_tracker.transfer_replace_bindings (other.m_tracker);
    m_name = std::move (other.m_name);
    return *this;
  }

  friend inline void bind (std::initializer_list<named1 *> n1s,
                           std::initializer_list<named2 *> n2s);

  friend inline void
  bind (std::initializer_list<std::reference_wrapper<named1>> n1s,
        std::initializer_list<std::reference_wrapper<named2>> n2s);

  void bind (named2& n2);

  template <typename ...Args>
  void bind (named2& n2, Args&... args)
  {
    bind (args...);
    bind (n2);
  }

  void clear_tracker (void) noexcept
  {
    return m_tracker.wipe ();
  }

  multireporter<named1, named2>& get_tracker (void)
  {
    return m_tracker;
  }

  std::size_t num_reporters (void) { return m_tracker.num_reporters (); }

  const std::string& get_name (void) const noexcept { return m_name; }

  friend std::ostream& operator<< (std::ostream& o, const named1& p);

  std::string print (void) const noexcept
  {
    std::ostringstream oss {};
    oss << *this;
    return oss.str ();
  }

private:

  multireporter<named1, named2> m_tracker;
  std::string m_name;

};

class named2
{
public:

  explicit named2 (std::string name)
    : m_tracker (*this),
      m_name (std::move (name))
  { }

  named2 (const named2& other)
    : m_tracker (other.m_tracker.clone (), *this),
      m_name (other.m_name)
  { }

  named2 (named2&& other) noexcept
    : m_tracker (std::move (other.m_tracker), *this),
      m_name (std::move (other.m_name))
  { }

  named2& operator= (const named2& other)
  {
    if (&other != this)
      {
        m_tracker.copy_replace_bindings (other.m_tracker);
        m_name = other.m_name;
      }
    return *this;
  }

  named2& operator= (named2&& other) noexcept
  {
    m_tracker.transfer_replace_bindings (other.m_tracker);
    m_name = std::move (other.m_name);
    return *this;
  }

  friend inline void bind (std::initializer_list<named1 *> n1s,
                           std::initializer_list<named2 *> n2s);

  friend inline void
  bind (std::initializer_list<std::reference_wrapper<named1>> n1s,
        std::initializer_list<std::reference_wrapper<named2>> n2s);

  void clear_tracker (void) noexcept
  {
    return m_tracker.wipe ();
  }

  multireporter<named2, named1>& get_tracker (void)
  {
    return m_tracker;
  }

  void bind (named1& n1)
  {
    m_tracker.bind (n1.get_tracker ());
  }

  template <typename ...Args>
  void bind (named1& n1, Args&... args)
  {
    bind (args...);
    bind (n1);
  }

  std::size_t num_reporters (void) { return m_tracker.num_reporters (); }

  const std::string& get_name (void) const noexcept { return m_name; }

  friend std::ostream& operator<< (std::ostream& o, const named2& p);

  std::string print (void) const noexcept
  {
    std::ostringstream oss {};
    oss << *this;
    return oss.str ();
  }

private:

  multireporter<named2, named1> m_tracker;
  std::string m_name;

};

void named1::bind (named2& n2)
{
  m_tracker.bind (n2.get_tracker ());
}

void bind (named2& n2, named1& n1)
{
  n2.bind (n1);
}

void bind (named1& n1, named2& n2)
{
  n1.bind (n2);
}

inline void
bind (std::initializer_list<named1 *> n1s, std::initializer_list<named2 *> n2s)
{
  for (named1 *n1 : n1s)
    for (named2 *n2 : n2s)
      bind (n1->m_tracker, n2->m_tracker);
}

inline void
bind (std::initializer_list<std::reference_wrapper<named1>> n1s,
      std::initializer_list<std::reference_wrapper<named2>> n2s)
{
  for (named1& n1 : n1s)
    for (named2& n2 : n2s)
      bind (n1.m_tracker, n2.m_tracker);
}

std::ostream& operator<< (std::ostream& o, const named1& p)
{
  o.width (4);
  o << p.m_name << ": { ";
  for (const named2& c : p.m_tracker)
    o << c.get_name () << " ";
  return o << "}";
}

std::ostream& operator<< (std::ostream& o, const named2& p)
{
  o.width (4);
  o << p.m_name << ": { ";
  for (const named1& c : p.m_tracker)
    o << c.get_name () << " ";
  return o << "}";
}

template <typename Child, typename Parent>
std::chrono::duration<double> perf_create (void)
{

  std::cout << "\nperf_create" << std::endl;
  std::cout <<   "___________" << std::endl;

  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();

  Parent p, q;
  std::list<Child> children;
  constexpr std::size_t iter_max = 1000 * multiplier;

  for (std::size_t i = 0; i < iter_max; ++i)
    {
      children.emplace_back (p.create ());
    }

  auto print_num_children = [&p, &q] (void)
    {
      std::cout << p.num_children () << "\n"
                << q.num_children () << "\n" << std::endl;
    };

  print_num_children ();

  q.transfer_from (p);

  print_num_children ();

  p.transfer_from (q);

  print_num_children ();

  std::random_device rd;
  std::mt19937 gen (rd ());
  std::uniform_real_distribution<double> dist (0.0, 1.0);
  for (std::size_t i = 0; i < multiplier; ++i)
    {
      for (auto it = children.begin (); it != children.end (); )
      {
        if (dist (gen) < 0.5 / multiplier)
          {
            children.erase (it++);
            continue;
          }
          ++it;
      }
    }

  print_num_children ();

  q.transfer_from (p);

  print_num_children ();

  p.transfer_from (q);

  print_num_children ();

  children.clear ();

  time t2 = clock::now ();
  return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1);
}

template <typename Child, typename Parent>
std::chrono::duration<double> perf_access (void)
{

  std::cout << "\nperf_access" << std::endl;
  std::cout <<   "___________" << std::endl;

  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();

  Parent p;
  std::vector<Child> children;
  constexpr std::size_t iter_max = 1000 * multiplier;

  for (std::size_t i = 0; i < iter_max; ++i)
    {
      children.emplace_back (p.create ());
    }

  std::random_device rd;
  std::mt19937 gen (rd ());
  std::uniform_int_distribution<std::size_t> dist (0, 99);
  for (std::size_t i = 0; i < 10 ; ++i)
    {
      for (const Child& r : p)
        {
          static_cast<void> (r.f (dist (gen)));
        }
    }

  children.clear ();

  time t2 = clock::now ();
  return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1);
}

std::chrono::duration<double> test_multireporter (void)
{

  std::cout << "\ntest_multireporter" << std::endl;
  std::cout <<   "__________________" << std::endl;

  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();

  std::unique_ptr<self_parent> p (new self_parent ("p"));
  std::unique_ptr<self_parent> q (new self_parent ("q"));
  std::unique_ptr<self_parent> r (new self_parent ("r"));
  bind (*p, *q);
  bind (*q, *r);
  bind (*r, *p);

  std::cout << "initial state" << std::endl;
  std::cout << *p << std::endl;
  std::cout << *q << std::endl;
  std::cout << *r << std::endl;

  std::cout << "remove p" << std::endl;
  p.reset ();
  std::cout << *q << std::endl;
  std::cout << *r << std::endl;
  std::cout << q->num_reporters () << std::endl;
  std::cout << r->num_reporters () << std::endl;

  time t2 = clock::now ();
  return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1);
}

std::chrono::duration<double> perf_multireporter (void)
{

  std::cout << "\nperf_multireporter" << std::endl;
  std::cout <<   "__________________" << std::endl;

  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();
  constexpr std::size_t num_iter = 100 * multiplier;

  std::vector<anon_self_parent> objs;
  objs.reserve (num_iter);

  for (std::size_t i = 0; i < num_iter; ++i)
    {
      objs.emplace_back ();
      auto& b = objs.back ();
      std::for_each (objs.begin (), --objs.end (),
                     [&b] (anon_self_parent& a)
                     {
                       bind (a, b);
                     });
    }

//  objs.emplace_back ();
//  auto it = objs.begin ();
//  for (std::size_t i = 0; i < num_iter-1; ++i)
//    {
//      objs.emplace_back ();
//      objs.back ().bind_all (*it++);
//    }

  std::cout << objs.back () << std::endl;

  std::for_each (objs.begin (), objs.end (),
                 [] (anon_self_parent& a) { a.clear_tracker (); });
  objs.clear ();

  time t2 = clock::now ();
  return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1);
}

std::chrono::duration<double> perf_disparate_multireporter (void)
{

  std::cout << "\nperf_disparate_multireporter" << std::endl;
  std::cout <<   "____________________________" << std::endl;

  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();
  constexpr std::size_t num_iter = 100 * multiplier;

  std::vector<anon1> a1s;
  std::vector<anon2> a2s;
  a1s.reserve (num_iter);
  a2s.reserve (num_iter);

  for (std::size_t i = 0; i < num_iter; ++i)
    {
      a1s.emplace_back ();
      a2s.emplace_back ();
      auto& a1 = a1s.back ();
      auto& a2 = a2s.back ();
      std::for_each (a2s.begin (), --a2s.end (),
                     [&a1] (anon2& a2)
                     {
                       a1.bind (a2);
                     });

      std::for_each (a1s.begin (), --a1s.end (),
                     [&a2] (anon1& a1)
                     {
                       a2.bind (a1);
                     });
      a1s.back ().bind (a2s.back ());
    }

  std::cout << a1s.back () << std::endl;
  std::cout << a2s.back () << std::endl;

  std::for_each (a1s.begin (), a1s.end (),
                 [] (anon1& a) { a.clear_tracker (); });
  std::for_each (a2s.begin (), a2s.end (),
                 [] (anon2& a) { a.clear_tracker (); });

  std::cout << a1s.back () << std::endl;
  std::cout << a2s.back () << std::endl;

  a1s.clear ();
  a2s.clear ();

  time t2 = clock::now ();
  return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1);
}

std::chrono::duration<double> test_disparate_multireporter (void)
{

  std::cout << "\ntest_disparate_multireporter" << std::endl;
  std::cout <<   "____________________________" << std::endl;

  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();

  std::unique_ptr<named1> n1_1 (new named1 ("n1_1")),
                          n1_2 (new named1 ("n1_2")),
                          n1_3 (new named1 ("n1_3"));

  std::unique_ptr<named2> n2_1 (new named2 ("n2_1")),
                          n2_2 (new named2 ("n2_2")),
                          n2_3 (new named2 ("n2_3"));

  bind ({*n1_1, *n1_2, *n1_3}, {*n2_1, *n2_2, *n2_3});

  constexpr std::size_t w = 29;
  auto print_all = [&] (void)
  {
    std::cout << std::left;
    std::cout.width (w);
    std::cout << n1_1->print () << " | ";
    std::cout.width (w);
    std::cout << n1_2->print () << " | ";
    std::cout.width (w);
    std::cout << n1_3->print () << std::endl;
    std::cout.width (w);
    std::cout << n2_1->print () << " | ";
    std::cout.width (w);
    std::cout << n2_2->print () << " | ";
    std::cout.width (w);
    std::cout << n2_3->print () << std::endl << std::endl;
  };

  std::cout << "initial state" << std::endl;
  print_all ();

  // copy constructor
  std::unique_ptr<named1> ptr (new named1 (*n1_3));

  std::cout << "copy ctor (ptr): " << *ptr << std::endl;
  print_all ();

  // move constructor
  std::unique_ptr<named1> (new named1 (std::move (*n1_2))).swap (ptr);

  std::cout << "move ctor (ptr): " << *ptr << std::endl;
  print_all ();

  // copy assignment operator
  *ptr = *n1_3;

  std::cout << "copy assign (ptr): " << *ptr << std::endl;
  print_all ();

//  std::cout << ptr.get () << std::endl;
//  std::cout << n1_1.get () << std::endl;
//  std::cout << n1_2.get () << std::endl;
//  std::cout << n1_3.get () << std::endl;
//  std::cout << n2_1.get () << std::endl;
//  std::cout << n2_2.get () << std::endl;
//  std::cout << n2_3.get () << std::endl << std::endl;

  // move assignment operator
  *ptr = std::move (*n1_1);

  std::cout << "move assign (ptr): " << *ptr << std::endl;
  print_all ();

  std::cout << "remove ptr" << std::endl;
  ptr.reset ();
  print_all ();

  time t2 = clock::now ();
  return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1);
}

std::chrono::duration<double> test_binding (void)
{

  std::cout << "\ntest_binding" << std::endl;
  std::cout <<   "____________" << std::endl;

  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();

  std::unique_ptr<named1> n1_1 (new named1 ("n1_1")),
                          n1_2 (new named1 ("n1_2")),
                          n1_3 (new named1 ("n1_3"));

  std::unique_ptr<named2> n2_1 (new named2 ("n2_1")),
                          n2_2 (new named2 ("n2_2")),
                          n2_3 (new named2 ("n2_3"));

  n1_1->bind (*n2_1, *n2_2, *n2_3);
  bind (*n1_1, *n2_1);
  n1_1->bind (*n2_1);

  n2_1->bind (*n1_1, *n1_2, *n1_3, *n1_3);

//  n1_1.bind (n2_1, n2_2, n2_3);
//  n1_2.bind (n2_1, n2_2, n2_3);
//  n1_3.bind (n2_1, n2_2, n2_3);
  auto print_all = [&] (void)
  {
    std::cout << n1_1->num_reporters () << " ";
    std::cout << n1_2->num_reporters () << " ";
    std::cout << n1_3->num_reporters () << std::endl;
    std::cout << n2_1->num_reporters () << " ";
    std::cout << n2_2->num_reporters () << " ";
    std::cout << n2_3->num_reporters () << std::endl << std::endl;
  };

  std::cout << "initial state" << std::endl;
  print_all ();

  // copy constructor
  std::unique_ptr<multireporter<named1, named2>> cpy (new multireporter<named1, named2> (*n1_3));

  std::cout << "cpy: " << cpy->num_reporters () << std::endl;
  print_all ();

  std::cout << "remove n1_1" << std::endl;
  n1_1.reset ();
  std::cout << n1_2->num_reporters () << " ";
  std::cout << n1_3->num_reporters () << std::endl;
  std::cout << n2_1->num_reporters () << " ";
  std::cout << n2_2->num_reporters () << " ";
  std::cout << n2_3->num_reporters () << std::endl << std::endl;

  time t2 = clock::now ();
  return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1);
}

template <typename Child, typename Parent>
std::chrono::duration<double> test_reporter (void)
{
  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();

  std::unique_ptr<Parent> p (new Parent ("parent1"));
  std::vector<Child> children;

  children.emplace_back (p->create ("child1"));
  children.emplace_back (p->create ("child2"));
  children.emplace_back (p->create ("child3"));
  children.emplace_back (p->create ("child4"));
  children.emplace_back (p->create ("child5"));
  children.emplace_back (p->create ("child6"));
  children.emplace_back (p->create ("child7"));

  std::cout << *p << std::endl;

  children.emplace_back (children[0]);

  std::cout << *p << std::endl;

  children.emplace_back (std::move (children[1]));

  std::cout << *p << std::endl;

  children[3] = children[2];

  std::cout << *p << std::endl;

  children[6] = std::move (children[4]);

  std::cout << *p << std::endl;

  children.erase (children.begin ());

  std::cout << *p << std::endl;

  Parent q ("parent2");

  Child qc_1 = q.create ("q_child1");
  Child qc_2 = q.create ("q_child2");
  Child qc_3 = q.create ("q_child3");

  std::cout << std::endl;

  auto print_pq = [&p, &q] (void)
  {
    std::cout << *p << std::endl;
    std::cout << q << std::endl << std::endl;
  };

  print_pq ();

  // copy assign q -> p
  std::cout << "copy assign q -> p" << std::endl;
  qc_2 = children[5];

  print_pq ();

  // move assign p -> q
  std::cout << "move assign p -> q" << std::endl;
  children[0] = std::move (qc_3);

  print_pq ();

  // rebind p -> q
  std::cout << "rebind p -> q" << std::endl;
  children[1].rebind (q);

  print_pq ();

  // transfer
  std::cout << "transfer" << std::endl;
  q.transfer_from (*p);

  print_pq ();

  std::cout << children[1].get_position () << std::endl;

  time t2 = clock::now ();
  return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1);
}

struct mix
  : reporter<mix, remote::standalone_tracker<>, tag::intrusive>,
    tracker<mix, remote::standalone_tracker<>, tag::intrusive>
{
  mix (standalone_tracker<remote::reporter<mix, tag::intrusive>>& tkr1,
       standalone_tracker<remote::tracker<mix, tag::intrusive>>& tkr2)
    : reporter (tag::bind, tkr1),
      tracker  (tag::bind, { tkr2 })
  { }
};

int main()
{
  try
  {
    std::cout << test_reporter<child, parent> ().count () << std::endl;
    std::cout
      << test_reporter<nonintruded_child_s, nonintruded_parent_s> ().count ()
      << std::endl;

    std::cout << perf_create<child, parent> ().count () << std::endl;
    std::cout
      << perf_create<nonintruded_child, nonintruded_parent> ().count ()
      << std::endl;

    std::cout << perf_access<child, parent> ().count () << std::endl;
    std::cout
      << perf_access<nonintruded_child, nonintruded_parent> ().count ()
      << std::endl;


    plf::list<int> x = {1, 2, 3, 4};
    plf::list<int>::iterator last = --x.end ();
    std::reverse_iterator<plf::list<int>::iterator> rb (x.end ());
    std::cout << (*last == *rb) << std::endl;

    test_multireporter ();

//      std::cout << "got back" << std::endl;

    std::cout << perf_multireporter ().count () << std::endl;
    std::cout << perf_disparate_multireporter ().count () << std::endl;

    test_disparate_multireporter ();
    test_binding ();

    standalone_tracker<remote::standalone_reporter<>> sa_tkr;
    standalone_reporter<remote::standalone_tracker<>> sa_rptr (tag::bind, sa_tkr);
    
    std::cout << &sa_rptr << std::endl;
    std::cout << &sa_tkr << std::endl << std::endl;

    std::cout << sa_rptr.has_remote () << std::endl;
    std::cout << &sa_rptr.get_remote () << std::endl;
    std::cout << sa_tkr.num_reporters () << std::endl;
    std::cout << &sa_tkr.front () << std::endl << std::endl;

    assert (sa_rptr.get_maybe_remote ().has_value ());

    sa_rptr.debind ();

    assert (! sa_rptr.get_maybe_remote ().has_value ());

    std::cout << sa_rptr.has_remote () << std::endl;
    std::cout << sa_tkr.num_reporters () << std::endl << std::endl;

    standalone_tracker<remote::standalone_reporter<>, std::list> sa_tkr_std;
    standalone_reporter<remote::standalone_tracker<std::list>> sa_rptr_std (tag::bind, sa_tkr_std);

    std::cout << &sa_rptr_std << std::endl;
    std::cout << &sa_tkr_std << std::endl << std::endl;

    std::cout << sa_rptr_std.has_remote () << std::endl;
    std::cout << &sa_rptr_std.get_remote () << std::endl;
    std::cout << sa_tkr_std.num_reporters () << std::endl;
    std::cout << &sa_tkr_std.front () << std::endl << std::endl;

    assert (sa_rptr_std.get_maybe_remote ().has_value ());

    sa_rptr_std.debind ();

    assert (! sa_rptr_std.get_maybe_remote ().has_value ());

    std::cout << sa_rptr_std.has_remote () << std::endl;
    std::cout << sa_tkr_std.num_reporters () << std::endl << std::endl;

    tracker<parent, remote::reporter<>, tag::intrusive> xp;

  }
  catch (std::exception &e)
  {
    std::cout << e.what () << std::endl;
    return 1;
  }

  std::cout << "itracker : ireporter  :" << sizeof (tracker<child,  remote::reporter<parent, tag::intrusive>,  tag::intrusive>)    << std::endl;
  std::cout << "itracker : nreporter  :" << sizeof (tracker<child,  remote::reporter<parent>,                  tag::intrusive>)    << std::endl;
  std::cout << "itracker : itracker   :" << sizeof (tracker<child,  remote::tracker <parent, tag::intrusive>,  tag::intrusive>)    << std::endl;
  std::cout << "itracker : ntracker   :" << sizeof (tracker<child,  remote::tracker <parent>,                  tag::intrusive>)    << std::endl;
  std::cout << "ntracker : ireporter  :" << sizeof (tracker<child,  remote::reporter<parent, tag::intrusive>,  tag::nonintrusive>) << std::endl;
  std::cout << "ntracker : nreporter  :" << sizeof (tracker<child,  remote::reporter<parent>,                  tag::nonintrusive>) << std::endl;
  std::cout << "ntracker : itracker   :" << sizeof (tracker<child,  remote::tracker <parent, tag::intrusive>,  tag::nonintrusive>) << std::endl;
  std::cout << "ntracker : ntracker   :" << sizeof (tracker<child,  remote::tracker <parent>,                  tag::nonintrusive>) << std::endl;
  std::cout << "ireporter : ireporter :" << sizeof (reporter<child, remote::reporter<parent, tag::intrusive>,  tag::intrusive>)    << std::endl;
  std::cout << "ireporter : nreporter :" << sizeof (reporter<child, remote::reporter<parent>,                  tag::intrusive>)    << std::endl;
  std::cout << "ireporter : itracker  :" << sizeof (reporter<child, remote::tracker <parent, tag::intrusive>,  tag::intrusive>)    << std::endl;
  std::cout << "ireporter : ntracker  :" << sizeof (reporter<child, remote::tracker <parent>,                  tag::intrusive>)    << std::endl;
  std::cout << "nreporter : ireporter :" << sizeof (reporter<child, remote::reporter<parent, tag::intrusive>,  tag::nonintrusive>) << std::endl;
  std::cout << "nreporter : nreporter :" << sizeof (reporter<child, remote::reporter<parent>,                  tag::nonintrusive>) << std::endl;
  std::cout << "nreporter : itracker  :" << sizeof (reporter<child, remote::tracker <parent, tag::intrusive>,  tag::nonintrusive>) << std::endl;
  std::cout << "nreporter : ntracker  :" << sizeof (reporter<child, remote::tracker <parent>,                  tag::nonintrusive>) << std::endl;

  std::cout << "iter  :" << sizeof (tracker<child, tag::intrusive::reporter<parent>, tag::nonintrusive>::iter) << std::endl;

  return 0;
}
