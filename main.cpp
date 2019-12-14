#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <random>
#include <list>

#include "tracker.h"

using namespace octave;

class parent;
class child;

class child : public intrusive_reporter<child, parent>
{
public:

  using base_type = intrusive_reporter<child, parent>;

  child (void) = default;

  child (std::nullptr_t, std::string s)
    : m_name (std::move (s))
  { }

  child (base_type::tracker_type& p)
    : intrusive_reporter (p)
  { }

  child (base_type::tracker_type& p, std::string s)
    : intrusive_reporter (p),
      m_name (std::move (s))
  { }

  child (const child& o)
    : intrusive_reporter (o),
      m_name (o.m_name)
  { }

  child (child&& o) noexcept
    : intrusive_reporter (std::move (o)),
      m_name (std::move (o.m_name))
  { }
  
  child& operator= (const child& o)
  {
    if (&o != this)
      {
        intrusive_reporter::operator=(o);
        m_name = o.m_name;
      }
    return *this;
  }

  child& operator= (child&& o) noexcept
  {
    m_name = std::move (o.m_name);
    intrusive_reporter::operator=(std::move (o));
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
  using tracker_type  = tracker<child, parent>;

  parent (void)
    : m_children (*this)
  { }

  explicit parent (std::string s)
    : m_children (*this),
      m_name (std::move (s))
  { }

//  child create (std::string s)
//  {
//    return { *this, std::move (s) };
//  }

  friend void child::rebind (parent&);

  void transfer_from (parent& other)
  {
    m_children.transfer_from (other.m_children);
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

  tracker_type::iter begin (void) noexcept { return m_children.begin (); }
  tracker_type::iter end (void)   noexcept { return m_children.end (); }

private:
  tracker_type m_children;
  std::string m_name;
};

void child::rebind (parent& p)
{
  base_type::rebind (p.m_children);
}

class nonintruded_child_s;
class nonintruded_child;

template <typename T>
class nonintruded_parent_temp;

using nonintruded_parent = nonintruded_parent_temp<nonintruded_child>;
using nonintruded_parent_s = nonintruded_parent_temp<nonintruded_child_s>;

class nonintruded_child_s
{
public:

  using reporter_type = reporter<nonintruded_child_s, nonintruded_parent_s>;

  nonintruded_child_s (reporter_type::tracker_type& remote, std::string name)
    : m_reporter (remote, *this),
      m_name (std::move (name))
  { }

  nonintruded_child_s (const nonintruded_child_s& other)
    : m_reporter (other.m_reporter, *this),
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
        m_reporter = other.m_reporter;
        m_name = other.m_name;
      }
    return *this;
  }

  nonintruded_child_s& operator= (nonintruded_child_s&& other) noexcept
  {
    m_reporter = std::move (other.m_reporter);
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

  using reporter_type = reporter<nonintruded_child, nonintruded_parent>;

  nonintruded_child (reporter_type::tracker_type& p)
    : m_reporter (p, *this)
  { }

  nonintruded_child (const nonintruded_child& other)
    : m_reporter (other.m_reporter, *this)
  { }

  nonintruded_child (nonintruded_child&& other) noexcept
    : m_reporter (std::move (other.m_reporter), *this)
  { }

  nonintruded_child& operator= (const nonintruded_child& other) = default;

  nonintruded_child& operator= (nonintruded_child&& other) noexcept
  {
    m_reporter = std::move (other.m_reporter);
    return *this;
  }

  friend std::ostream& operator<< (std::ostream& o, const nonintruded_child& c)
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

  // works, for some reason, figure out why
  // using tracker_type = tracker_base<reporter<nonintruded_child, nonintruded_parent>, nonintruded_parent>;

  using reporter_type = typename T::reporter_type;
  using tracker_type  = typename reporter_type::tracker_type;

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
//    return { *this, std::move (s) };
//  }

  void transfer_from (nonintruded_parent_temp& other)
  {
    m_children.transfer_from (other.m_children);
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

  explicit self_parent (self_parent& p, std::string name)
    : m_tracker (p.m_tracker, *this),
      m_name (std::move (name))
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
    m_tracker = std::move (other.m_tracker);
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
    m_tracker = std::move (other.m_tracker);
    return *this;
  }
  
  void clear_tracker (void) noexcept
  {
    return m_tracker.clear ();
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
    m_tracker = std::move (other.m_tracker);
    return *this;
  }

  void clear_tracker (void) noexcept
  {
    return m_tracker.clear ();
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
    m_tracker = std::move (other.m_tracker);
    return *this;
  }

  void clear_tracker (void) noexcept
  {
    return m_tracker.clear ();
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
    : m_tracker (other.m_tracker, *this),
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
        m_tracker = other.m_tracker;
        m_name = other.m_name;
      }
    return *this;
  }

  named1& operator= (named1&& other) noexcept
  {
    m_tracker = std::move (other.m_tracker);
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
    return m_tracker.clear ();
  }
  
  const std::string& get_name (void) const noexcept { return m_name; }

  friend std::ostream& operator<< (std::ostream& o, const named1& p);

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
    : m_tracker (other.m_tracker, *this),
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
        m_tracker = other.m_tracker;
        m_name = other.m_name;
      }
    return *this;
  }
  
  named2& operator= (named2&& other) noexcept
  {
    m_tracker = std::move (other.m_tracker);
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
    return m_tracker.clear ();
  }

  const std::string& get_name (void) const noexcept { return m_name; }

  friend std::ostream& operator<< (std::ostream& o, const named2& p);

private:

  multireporter<named2, named1> m_tracker;
  std::string m_name;

};

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
  o << p.m_name << ": { ";
  for (const named2& c : p.m_tracker)
    o << c.get_name () << " ";
  return o << "}";
}

std::ostream& operator<< (std::ostream& o, const named2& p)
{
  o << p.m_name << ": { ";
  for (const named1& c : p.m_tracker)
    o << c.get_name () << " ";
  return o << "}";
}

template <typename Child, typename Parent>
std::chrono::duration<double> perf_create (void)
{
  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();

  Parent p, q;
  std::list<Child> children;
  constexpr std::size_t iter_max = 100000;

  for (std::size_t i = 0; i < iter_max; ++i)
    {
      children.emplace_back (p.create ());
    }

  q.transfer_from (p);
  p.transfer_from (q);

  std::random_device rd;
  std::mt19937 gen (rd());
  std::uniform_int_distribution<std::size_t> dist (0, 100);
  for (std::size_t i = 0; i < 100; ++i)
    {
      for (auto it = children.begin (); it != children.end (); )
      {
//        if (dist (gen) < 15)
        if (std::rand () < RAND_MAX * 0.15)
          {
            children.erase (it++);
            continue;
          }
          ++it;
      }
    }

  q.transfer_from (p);
  p.transfer_from (q);

  children.clear ();

  time t2 = clock::now ();
  return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1);
}

template <typename Child, typename Parent>
std::chrono::duration<double> perf_access (void)
{
  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();

  Parent p;
  std::vector<Child> children;
  constexpr std::size_t iter_max = 100000;

  for (std::size_t i = 0; i < iter_max; ++i)
    {
      children.emplace_back (p.create ());
    }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<std::size_t> dist (0, 100);
  for (std::size_t i = 0; i < 10 ; ++i)
    {
      for (const Child& r : p)
        {
          std::size_t x = r.f (dist (gen));
        }
    }

  children.clear ();

  time t2 = clock::now ();
  return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1);
}

std::chrono::duration<double> test_multireporter (void)
{
  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();

  self_parent p ("p");
  self_parent q ("q");
  self_parent r ("r");
  bind (p, q);
  bind (r, q);
  bind (r, p);
  
  std::cout << "initial state" << std::endl;
  std::cout << p << std::endl;
  std::cout << q << std::endl;
  std::cout << r << std::endl;
  
  std::cout << "remove p" << std::endl;
  p.~self_parent ();
  std::cout << q << std::endl;
  std::cout << r << std::endl;
  
  time t2 = clock::now ();
  return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1);
}

std::chrono::duration<double> perf_multireporter (void)
{
  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();
  constexpr std::size_t num_iter = 10000;
  
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
  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();
  constexpr std::size_t num_iter = 10000;

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
  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();

  named1 n1_1 ("n1_1"), n1_2 ("n1_2"), n1_3 ("n1_3");
  named2 n2_1 ("n2_1"), n2_2 ("n2_2"), n2_3 ("n2_3");

  bind ({n1_1, n1_2, n1_3}, {n2_1, n2_2, n2_3});

  auto print_all = [&] (void)
  {
    std::cout << n1_1 << " | ";
    std::cout << n1_2 << " | ";
    std::cout << n1_3 << std::endl;
    std::cout << n2_1 << " | ";
    std::cout << n2_2 << " | ";
    std::cout << n2_3 << std::endl << std::endl;
  };

  std::cout << "initial state" << std::endl;
  print_all ();

  // copy constructor
  std::unique_ptr<named1> ptr (new named1 (n1_3));

  std::cout << "copy ctor: " << *ptr << std::endl;
  print_all ();
  
  // move constructor
  std::unique_ptr<named1> (new named1 (std::move (n1_2))).swap (ptr);

  std::cout << "move ctor: " << *ptr << std::endl;
  print_all ();
  
  // copy assignment operator
  *ptr = n1_3;

  std::cout << "copy assign: " << *ptr << std::endl;
  print_all ();

  // copy assignment operator
  *ptr = std::move (n1_1);

  std::cout << "move assign: " << *ptr << std::endl;
  print_all ();

  std::cout << "remove n1_1" << std::endl;
  n1_3.~named1 ();
  print_all ();

  time t2 = clock::now ();
  return std::chrono::duration_cast<std::chrono::duration<double>> (t2 - t1);
}

std::chrono::duration<double> test_binding (void)
{
  using clock = std::chrono::high_resolution_clock;
  using time = clock::time_point;
  time t1 = clock::now ();

  multireporter<named1, named2> n1_1, n1_2, n1_3;
  multireporter<named2, named1> n2_1, n2_2, n2_3;

  n1_1.bind (n2_1, n2_2, n2_3);
  bind (n1_1, n2_1);
  n1_1.bind (n2_1);
  
  n2_1.bind (n1_1, n1_2, n1_3, n1_3);

//  n1_1.bind (n2_1, n2_2, n2_3);
//  n1_2.bind (n2_1, n2_2, n2_3);
//  n1_3.bind (n2_1, n2_2, n2_3);
  auto print_all = [&] (void)
  {
    std::cout << n1_1.num_reporters () << " ";
    std::cout << n1_2.num_reporters () << " ";
    std::cout << n1_3.num_reporters () << std::endl;
    std::cout << n2_1.num_reporters () << " ";
    std::cout << n2_2.num_reporters () << " ";
    std::cout << n2_3.num_reporters () << std::endl << std::endl;
  };

  std::cout << "initial state" << std::endl;
  print_all ();

  // copy constructor
  multireporter<named1, named2> cpy (n1_3);

  std::cout << "cpy: " << cpy.num_reporters () << std::endl;
  print_all ();

  std::cout << "remove n1_1" << std::endl;
  n1_1.~multireporter ();
  print_all ();
  
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

int main()
{
  try
    {
      std::cout << test_reporter <child, parent> ().count () << std::endl;
      std::cout << test_reporter <nonintruded_child_s, nonintruded_parent_s> ().count () << std::endl;

      std::cout << perf_create<child, parent> ().count () << std::endl;
      std::cout << perf_create<nonintruded_child, nonintruded_parent> ().count () << std::endl;

      std::cout << perf_access<child, parent> ().count () << std::endl;
      std::cout << perf_access<nonintruded_child, nonintruded_parent> ().count () << std::endl;

      plf::list<int> x = { 1, 2, 3, 4 };
      plf::list<int>::iterator last = --x.end ();
      std::reverse_iterator<plf::list<int>::iterator> rb (x.end ());
      std::cout << (*last == *rb) << std::endl;

      test_multireporter ();

      std::cout << perf_multireporter ().count () << std::endl;
      std::cout << perf_disparate_multireporter ().count () << std::endl;

      test_disparate_multireporter ();
      test_binding ();
    }
    catch (std::exception& e)
      {
        std::cout << e.what () << std::endl;
        return 1;
      }
      
  return 0;

}