/** set_tracker.h
 * Short description here. 
 * 
 * Copyright © 2019 Gene Harvey
 *
 * This software may be modified and distributed under the terms
 * of the MIT license. See the LICENSE file for details.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef _SET_TRACKER_H_
#define _SET_TRACKER_H_

#include <iostream>
#include <memory>
#include <unordered_set>

class set_child;
class set_parent;

class set_child
{
public:

  set_child (set_parent *parent)
    : m_parent (parent)
  { }

  set_child (set_parent *parent, std::string s)
    : m_parent (parent),
      m_name (std::move (s))
  { }

  set_child (const set_child& other);
  set_child (set_child&& other);

  set_child& operator= (const set_child& other);
  set_child& operator= (set_child&& other);

  ~set_child (void);

  friend std::ostream& operator<< (std::ostream& o, const set_child& c)
  {
    return o << c.m_name;
  }

private:
  set_parent *m_parent = nullptr;
  std::string m_name;
};

class set_parent
{
public:

  set_parent (void) = default;

  explicit set_parent (std::string s)
    : m_name (std::move (s))
  { }

  void transfer_from (set_parent& other)
  {
    m_children.insert (other.m_children.begin (), other.m_children.end ());
    other.m_children.clear ();
  }

  template <typename ...Args>
  set_child create (Args... args)
  {
    return { this, std::forward<Args> (args)... };
  }

  friend std::ostream& operator<< (std::ostream& o, const set_parent& p)
  {
    o << p.m_name << ": { ";
    for (const set_child *c : p.m_children)
      o << c << " ";
    return o << "}";
  }

  void track (set_child *c)
  {
    m_children.insert (c);
  }

  void erase (set_child *c)
  {
    m_children.erase (c);
  }

private:
  std::unordered_set<set_child *> m_children;
  std::string m_name;
};

set_child::set_child (const set_child& other)
  : m_parent (other.m_parent),
    m_name (other.m_name)
{
  if (m_parent != nullptr)
    m_parent->track (this);
}

set_child::set_child (set_child&& other)
  : m_parent (other.m_parent),
    m_name (std::move (other.m_name))
{
  if (m_parent != nullptr)
    {
      m_parent->track (this);
      m_parent->erase (&other);
      other.m_parent = nullptr;
    }
}

set_child& set_child::operator= (const set_child& other)
{
  if (m_parent != nullptr)
    m_parent->erase (this);

  m_parent = other.m_parent;
  m_name   = other.m_name;

  if(m_parent != nullptr)
    m_parent->track (this);
  return *this;
}

set_child& set_child::operator= (set_child&& other)
{
  if (m_parent != nullptr)
    m_parent->erase (this);

  m_parent = other.m_parent;
  m_name   = std::move (other.m_name);

  if (m_parent != nullptr)
    {
      m_parent->track (this);
      m_parent->erase (&other);
      other.m_parent = nullptr;
    }
  return *this;
}

set_child::~set_child (void)
{
  if (m_parent != nullptr)
    m_parent->erase (this);
}

#endif /* _SET_TRACKER_H_ */
