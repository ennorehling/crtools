#ifndef CLASS_CROBJECT_H
#define CLASS_CROBJECT_H

#include <map>
#include <vector>
#include <string>
#include <set>
#include <boost/any.hpp>

struct blocktype;

class CRObject {
public:
  CRObject();
  ~CRObject();

  void initialize(const blocktype * type, const int * ids, size_t size);

  const blocktype * getType() const { return m_pcType; }
  const std::vector<int>& getIds() { return m_cIds; }

  void set(const std::string& key, const boost::any& value);
  const boost::any& get(const std::string& key) const;
  bool has_key(const std::string& key) const;

  typedef std::set<CRObject*> ChildList;
  void addChild(CRObject * child);
  void removeChild(CRObject * child);
  CRObject * getParent() const;
  const ChildList& getChildren() const { return m_cChildren; }

  typedef std::vector<std::string> Strings;
  const Strings& getStrings() const { return m_cStrings; }
  void addString(const std::string& str) { m_cStrings.push_back(str); }

private:
  void setParent(CRObject * parent);

  const blocktype * m_pcType;
  std::vector<int> m_cIds;
  typedef std::map<std::string, boost::any> Map;
  Map m_cValues;
  CRObject* m_pcParent;
  ChildList m_cChildren;
  Strings m_cStrings;
};

#endif
