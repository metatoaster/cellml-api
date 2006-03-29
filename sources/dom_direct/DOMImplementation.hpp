#include "Utilities.hxx"
#include "IfaceDOM-APISPEC.hxx"
#include <string>
#include <list>
#include <map>
#include <inttypes.h>
#include <typeinfo>
#include <assert.h>
#include <sys/time.h>
#include <time.h>

class CDA_DOMImplementation
  : public iface::dom::DOMImplementation
{
public:
  CDA_DOMImplementation() : _cda_refcount(1) {}

  static CDA_DOMImplementation* sDOMImplementation;

  virtual ~CDA_DOMImplementation() {}

  CDA_IMPL_REFCOUNT;
  CDA_IMPL_QI1(dom::DOMImplementation);
  CDA_IMPL_COMPARE_NAIVE(CDA_DOMImplementation);

  bool hasFeature(const wchar_t* feature, const wchar_t* version)
    throw(std::exception&);

  iface::dom::DocumentType* createDocumentType(const wchar_t* qualifiedName,
                                               const wchar_t* publicId,
                                               const wchar_t* systemId)
    throw(std::exception&);
  iface::dom::Document* createDocument(const wchar_t* namespaceURI,
                                       const wchar_t* qualifiedName,
                                       iface::dom::DocumentType* doctype)
    throw(std::exception&);

  // A non-standard function used by the bootstrap code to load documents...
  iface::dom::Document* loadDocument(const wchar_t* aURL,
                                     std::wstring& aErrorMessage);
};

class CDA_Document;
class CDA_MutationEvent;

class CDA_Node
  : public virtual iface::dom::Node
{
public:
  CDA_Node(CDA_Document* aDocument);
  virtual ~CDA_Node();

  void add_ref()
    throw(std::exception&)
  {
    _cda_refcount++;
    
    if (mParent != NULL)
      mParent->add_ref();
  }

  void release_ref()
    throw(std::exception&)
  {
    if (_cda_refcount == 0)
    {
      printf("Warning: release_ref called too many times on %s.\n",
             typeid(this).name());
      assert(0);
    }
    _cda_refcount--;
    if (mParent == NULL)
    {
      if (_cda_refcount == 0)
        delete this;
    }
    else /* if the owner model is non-null, we will be destroyed when there are
          * no remaining references to the model.
          */
      mParent->release_ref();
  }

  wchar_t* nodeName() throw(std::exception&);
  wchar_t* nodeValue() throw(std::exception&);
  void nodeValue(const wchar_t* attr) throw(std::exception&);
  iface::dom::Node* parentNode() throw(std::exception&);
  iface::dom::NodeList* childNodes() throw(std::exception&);
  iface::dom::Node* firstChild() throw(std::exception&);
  iface::dom::Node* lastChild() throw(std::exception&);
  iface::dom::Node* previousSibling() throw(std::exception&);
  iface::dom::Node* nextSibling() throw(std::exception&);
  iface::dom::NamedNodeMap* attributes() throw(std::exception&);
  iface::dom::Document* ownerDocument() throw(std::exception&);
  iface::dom::Node* insertBefore(iface::dom::Node* newChild,
                                 iface::dom::Node* refChild)
    throw(std::exception&);
  iface::dom::Node* insertBeforePrivate(CDA_Node* newChild,
                                        CDA_Node* refChild)
    throw(std::exception&);
  iface::dom::Node* replaceChild(iface::dom::Node* newChild,
                                 iface::dom::Node* oldChild)
    throw(std::exception&);
  iface::dom::Node* removeChild(iface::dom::Node* oldChild)
    throw(std::exception&);
  iface::dom::Node* removeChildPrivate(CDA_Node* oldChild)
    throw(std::exception&);
  iface::dom::Node* appendChild(iface::dom::Node* newChild)
    throw(std::exception&);
  bool hasChildNodes() throw(std::exception&);
  virtual CDA_Node* shallowCloneNode() throw(std::exception&) = 0;
  iface::dom::Node* cloneNode(bool deep) throw(std::exception&);
  void normalize() throw(std::exception&);
  bool isSupported(const wchar_t* feature, const wchar_t* version)
    throw(std::exception&);
  wchar_t* namespaceURI() throw(std::exception&);
  wchar_t* prefix() throw(std::exception&);
  void prefix(const wchar_t* attr) throw(std::exception&);
  wchar_t* localName() throw(std::exception&);
  void addEventListener(const wchar_t* type,
                        iface::events::EventListener* listener,
                        bool useCapture) throw(std::exception&);
  void removeEventListener(const wchar_t* type,
                           iface::events::EventListener* listener,
                           bool useCapture) throw(std::exception&);
  bool dispatchEvent(iface::events::Event* evt) throw(std::exception&);
  // Two methods because it makes sense for inserted to be pre-order and
  // removed to be post-order.
  void dispatchInsertedIntoDocument(CDA_MutationEvent* me) throw(std::exception&);
  void dispatchRemovedFromDocument(CDA_MutationEvent* me) throw(std::exception&);
  void callEventListeners(CDA_MutationEvent* me) throw(std::exception&);
  bool hasAttributes() throw(std::exception&) { return false; }
  void updateDocumentAncestorStatus(bool aStatus);
  void recursivelyChangeDocument(CDA_Document* aNewDocument);
  virtual iface::dom::Element* searchForElementById(const wchar_t* elementId);

  CDA_Node* mParent;
  bool mDocumentIsAncestor;
  CDA_Document* mDocument;
  std::wstring mNodeName, mLocalName, mNodeValue, mNamespaceURI;

  std::list<CDA_Node*> mNodeList;
  std::multimap<std::pair<std::wstring,bool>, iface::events::EventListener*>
    mListeners;
  uint32_t _cda_refcount;
};

class CDA_NodeList
  : public iface::dom::NodeList
{
public:
  CDA_NodeList(CDA_Node* parent)
    : mParent(parent)
  {
    mParent->add_ref();
  }

  virtual ~CDA_NodeList()
  {
    if (mParent != NULL)
      mParent->release_ref();
  }

  CDA_IMPL_QI1(dom::NodeList);
  CDA_IMPL_COMPARE_NAIVE(CDA_NodeList);
  CDA_IMPL_REFCOUNT

  iface::dom::Node* item(uint32_t index) throw(std::exception&);
  uint32_t length() throw(std::exception&);

  CDA_Node* mParent;
};

class CDA_NodeListDFSSearch
  : public iface::dom::NodeList
{
public:
  CDA_NodeListDFSSearch
  (
   CDA_Node* parent, const std::wstring& aNameFilter
  )
    : mParent(parent), mNameFilter(aNameFilter),
      mFilterType(LEVEL_1_NAME_FILTER)
  {
    mParent->add_ref();
  }

  CDA_NodeListDFSSearch
  (
   CDA_Node* parent, const std::wstring& aNamespaceFilter,
   const std::wstring& aLocalnameFilter
   )
    : mParent(parent), mNamespaceFilter(aNamespaceFilter),
      mNameFilter(aLocalnameFilter),
      mFilterType(LEVEL_2_NAME_FILTER)
  {
    mParent->add_ref();
  }

  virtual ~CDA_NodeListDFSSearch()
  {
    if (mParent != NULL)
      mParent->release_ref();
  }

  CDA_IMPL_QI1(dom::NodeList);
  CDA_IMPL_COMPARE_NAIVE(CDA_NodeList);
  CDA_IMPL_REFCOUNT

  iface::dom::Node* item(uint32_t index) throw(std::exception&);
  uint32_t length() throw(std::exception&);

  CDA_Node* mParent;
  std::wstring mNamespaceFilter, mNameFilter;
  enum
  {
    LEVEL_1_NAME_FILTER,
    LEVEL_2_NAME_FILTER
  } mFilterType;
};

class CDA_EmptyNamedNodeMap
  : public iface::dom::NamedNodeMap
{
public:
  CDA_EmptyNamedNodeMap() {};
  ~CDA_EmptyNamedNodeMap() {}

  CDA_IMPL_QI1(dom::NamedNodeMap);
  CDA_IMPL_COMPARE_NAIVE(CDA_NamedNodeMap);
  CDA_IMPL_REFCOUNT

  iface::dom::Node* getNamedItem(const wchar_t* name)
    throw(std::exception&)
  {
    return NULL;
  }

  iface::dom::Node* setNamedItem(iface::dom::Node* arg)
    throw(std::exception&)
  {
    throw iface::dom::DOMException();
  }
  iface::dom::Node* removeNamedItem(const wchar_t* name)
    throw(std::exception&)
  {
    throw iface::dom::DOMException();
  }

  iface::dom::Node* item(uint32_t index)
    throw(std::exception&)
  {
    return NULL;
  }

  uint32_t length()
    throw(std::exception&)
  {
    return 0;
  }

  iface::dom::Node* getNamedItemNS(const wchar_t* namespaceURI,
                                   const wchar_t* localName)
    throw(std::exception&)
  {
    return NULL;
  }

  iface::dom::Node* setNamedItemNS(iface::dom::Node* arg)
    throw(std::exception&)
  {
    throw iface::dom::DOMException();
  }

  iface::dom::Node* removeNamedItemNS(const wchar_t* namespaceURI,
                                      const wchar_t* localName)
    throw(std::exception&)
  {
    throw iface::dom::DOMException();
  }
};

class CDA_Element;

class CDA_NamedNodeMap
  : public iface::dom::NamedNodeMap
{
public:
  CDA_NamedNodeMap(CDA_Element* aElement);
  ~CDA_NamedNodeMap();

  CDA_IMPL_QI1(dom::NamedNodeMap);
  CDA_IMPL_COMPARE_NAIVE(CDA_NamedNodeMap);
  CDA_IMPL_REFCOUNT

  iface::dom::Node* getNamedItem(const wchar_t* name) throw(std::exception&);
  iface::dom::Node* setNamedItem(iface::dom::Node* arg) throw(std::exception&);
  iface::dom::Node* removeNamedItem(const wchar_t* name)
    throw(std::exception&);
  iface::dom::Node* item(uint32_t index) throw(std::exception&);
  uint32_t length() throw(std::exception&);
  iface::dom::Node* getNamedItemNS(const wchar_t* namespaceURI,
                                   const wchar_t* localName)
    throw(std::exception&);
  iface::dom::Node* setNamedItemNS(iface::dom::Node* arg)
    throw(std::exception&);
  iface::dom::Node* removeNamedItemNS(const wchar_t* namespaceURI,
                                      const wchar_t* localName)
    throw(std::exception&);

  CDA_Element* mElement;
};

class CDA_DocumentType;

class CDA_NamedNodeMapDT
  : public iface::dom::NamedNodeMap
{
public:
  CDA_NamedNodeMapDT(CDA_DocumentType* aDocType,
                     uint16_t aType);
  ~CDA_NamedNodeMapDT();

  CDA_IMPL_QI1(dom::NamedNodeMap);
  CDA_IMPL_COMPARE_NAIVE(CDA_NamedNodeMap);
  CDA_IMPL_REFCOUNT

  iface::dom::Node* getNamedItem(const wchar_t* name) throw(std::exception&);
  iface::dom::Node* setNamedItem(iface::dom::Node* arg) throw(std::exception&);
  iface::dom::Node* removeNamedItem(const wchar_t* name)
    throw(std::exception&);
  iface::dom::Node* item(uint32_t index) throw(std::exception&);
  uint32_t length() throw(std::exception&);
  iface::dom::Node* getNamedItemNS(const wchar_t* namespaceURI,
                                   const wchar_t* localName)
    throw(std::exception&);
  iface::dom::Node* setNamedItemNS(iface::dom::Node* arg)
    throw(std::exception&);
  iface::dom::Node* removeNamedItemNS(const wchar_t* namespaceURI,
                                      const wchar_t* localName)
    throw(std::exception&);

  CDA_DocumentType* mDocType;

  uint16_t mType;
};

class CDA_CharacterData
  : public virtual iface::dom::CharacterData, public CDA_Node
{
public:
  CDA_CharacterData(CDA_Document* aDocument) : CDA_Node(aDocument) {}
  virtual ~CDA_CharacterData() {}

  wchar_t* data() throw(std::exception&);
  void data(const wchar_t* attr) throw(std::exception&);
  uint32_t length() throw(std::exception&);
  wchar_t* substringData(uint32_t offset, uint32_t count)
    throw(std::exception&);
  void appendData(const wchar_t* arg) throw(std::exception&);
  void insertData(uint32_t offset, const wchar_t* arg)
    throw(std::exception&);
  void deleteData(uint32_t offset, uint32_t count) throw(std::exception&);
  void replaceData(uint32_t offset, uint32_t count, const wchar_t* arg)
    throw(std::exception&);

  void dispatchCharDataModified(const std::wstring& oldValue);
};

#define CDA_IMPL_NODETYPE(type) \
  uint16_t nodeType() throw(std::exception&) \
  { return iface::dom::Node::type##_NODE; }

class CDA_Attr
  : public iface::dom::Attr, public CDA_Node
{
public:
  CDA_Attr(CDA_Document* aDocument) : CDA_Node(aDocument), mSpecified(false) {}
  virtual ~CDA_Attr() {}

  CDA_IMPL_QI2(dom::Node, dom::Attr);
  CDA_IMPL_COMPARE_NAIVE(CDA_Attr);
  CDA_IMPL_NODETYPE(ATTRIBUTE);

  CDA_Node* shallowCloneNode() throw(std::exception&);
  wchar_t* name() throw(std::exception&);
  bool specified() throw(std::exception&);
  wchar_t* value() throw(std::exception&);
  void value(const wchar_t* attr) throw(std::exception&);
  iface::dom::Element* ownerElement() throw(std::exception&);

  bool mSpecified;
};

class CDA_Element
  : public iface::dom::Element, public CDA_Node
{
public:
  CDA_Element(CDA_Document* aDocument) : CDA_Node(aDocument) {}
  virtual ~CDA_Element() {}

  CDA_IMPL_QI2(dom::Node, dom::Element);
  CDA_IMPL_COMPARE_NAIVE(CDA_Element);
  CDA_IMPL_NODETYPE(ELEMENT)

  iface::dom::NamedNodeMap* attributes() throw(std::exception&);
  CDA_Node* shallowCloneNode() throw(std::exception&);
  wchar_t* tagName() throw(std::exception&);
  wchar_t* getAttribute(const wchar_t* name) throw(std::exception&);
  void setAttribute(const wchar_t* name, const wchar_t* value)
    throw(std::exception&);
  void removeAttribute(const wchar_t* name) throw(std::exception&);
  iface::dom::Attr* getAttributeNode(const wchar_t* name)
    throw(std::exception&);
  iface::dom::Attr* setAttributeNode(iface::dom::Attr* newAttr)
    throw(std::exception&);
  iface::dom::Attr* removeAttributeNode(iface::dom::Attr* oldAttr)
    throw(std::exception&);
  iface::dom::NodeList* getElementsByTagName(const wchar_t* name)
    throw(std::exception&);
  wchar_t* getAttributeNS(const wchar_t* namespaceURI, const wchar_t* localName)
    throw(std::exception&);
  void setAttributeNS(const wchar_t* namespaceURI, const wchar_t* qualifiedName,
                      const wchar_t* value) throw(std::exception&);
  void removeAttributeNS(const wchar_t* namespaceURI, const wchar_t* localName)
    throw(std::exception&);
  iface::dom::Attr* getAttributeNodeNS(const wchar_t* namespaceURI,
                                       const wchar_t* localName)
    throw(std::exception&);
  iface::dom::Attr* setAttributeNodeNS(iface::dom::Attr* newAttr)
    throw(std::exception&);
  iface::dom::NodeList* getElementsByTagNameNS(const wchar_t* namespaceURI,
                                               const wchar_t* localName)
    throw(std::exception&);
  bool hasAttribute(const wchar_t* name) throw(std::exception&);
  bool hasAttributeNS(const wchar_t* namespaceURI, const wchar_t* localName)
    throw(std::exception&);
  bool hasAttributes() throw(std::exception&);
  iface::dom::Element* searchForElementById(const wchar_t* elementId);

  std::map<std::pair<std::wstring, std::wstring>, CDA_Attr*> attributeMap;
};

class CDA_TextBase
  : public virtual iface::dom::Text, public CDA_CharacterData
{
public:
  CDA_TextBase(CDA_Document* aDocument) : CDA_CharacterData(aDocument) {}
  virtual ~CDA_TextBase() {}
  virtual iface::dom::Text* splitText(uint32_t offset) throw(std::exception&);
};

class CDA_Text
  : public CDA_TextBase
{
public:
  CDA_Text(CDA_Document* aDocument) : CDA_TextBase(aDocument) {}
  virtual ~CDA_Text() {}

  CDA_IMPL_QI3(dom::Node, dom::CharacterData, dom::Text);
  CDA_IMPL_COMPARE_NAIVE(CDA_Text);
  CDA_IMPL_NODETYPE(TEXT);
  CDA_Node* shallowCloneNode() throw(std::exception&);
};

class CDA_Comment
  : public iface::dom::Comment, public CDA_CharacterData
{
public:
  CDA_Comment(CDA_Document* aDocument) : CDA_CharacterData(aDocument) {}
  virtual ~CDA_Comment() {}

  CDA_IMPL_QI3(dom::Node, dom::CharacterData, dom::Comment);
  CDA_IMPL_COMPARE_NAIVE(CDA_Comment);

  CDA_Node* shallowCloneNode() throw(std::exception&);
  CDA_IMPL_NODETYPE(COMMENT);
};

class CDA_CDATASection
  : public iface::dom::CDATASection, public CDA_TextBase
{
public:
  CDA_CDATASection(CDA_Document* aDocument) : CDA_TextBase(aDocument) {}
  virtual ~CDA_CDATASection() {}

  CDA_IMPL_QI4(dom::Node, dom::CharacterData, dom::Text, dom::CDATASection);
  CDA_IMPL_COMPARE_NAIVE(CDA_CDATASection);

  CDA_Node* shallowCloneNode() throw(std::exception&);
  CDA_IMPL_NODETYPE(CDATA_SECTION);
};

class CDA_DocumentType
  : public virtual iface::dom::DocumentType, public CDA_Node
{
public:
  CDA_DocumentType(
                   CDA_Document* aDocument,
                   const std::wstring& qualifiedName,
                   const std::wstring& publicId,
                   const std::wstring& systemId)
    : CDA_Node(aDocument)
  {
    mNodeName = qualifiedName;
    mPublicId = publicId;
    mSystemId = systemId;
  }

  virtual ~CDA_DocumentType() {}

  CDA_IMPL_QI2(dom::Node, dom::DocumentType);
  CDA_IMPL_COMPARE_NAIVE(CDA_DocumentType);
  CDA_IMPL_NODETYPE(DOCUMENT_TYPE)

  CDA_Node* shallowCloneNode() throw(std::exception&);
  wchar_t* name() throw(std::exception&);
  iface::dom::NamedNodeMap* entities() throw(std::exception&);
  iface::dom::NamedNodeMap* notations() throw(std::exception&);
  wchar_t* publicId() throw(std::exception&);
  wchar_t* systemId() throw(std::exception&);
  wchar_t* internalSubset() throw(std::exception&);

  std::wstring mPublicId, mSystemId;
  std::map<std::wstring,iface::dom::Node*> mNotations;
  std::map<std::wstring,iface::dom::Node*> mEntities;
};

class CDA_Notation
  : public iface::dom::Notation, public CDA_Node
{
public:
  CDA_Notation(CDA_Document* aDocument,
               const std::wstring& aPublicId,
               const std::wstring& aSystemId)
    : CDA_Node(aDocument), mPublicId(aPublicId), mSystemId(aSystemId)
  {}
  virtual ~CDA_Notation() {}

  CDA_IMPL_QI2(dom::Node, dom::Notation);
  CDA_IMPL_COMPARE_NAIVE(CDA_Notation);

  CDA_Node* shallowCloneNode() throw(std::exception&);
  CDA_IMPL_NODETYPE(NOTATION);
  wchar_t* publicId() throw(std::exception&);
  wchar_t* systemId() throw(std::exception&);

  std::wstring mPublicId, mSystemId;
};

class CDA_Entity
  : public iface::dom::Entity, public CDA_Node
{
public:
  CDA_Entity(CDA_Document* aDocument,
             std::wstring& aPublicId, std::wstring& aSystemId,
             std::wstring& aNotationName) 
    : CDA_Node(aDocument), mPublicId(aPublicId), mSystemId(aSystemId),
      mNotationName(aNotationName) {}
  virtual ~CDA_Entity() {}

  CDA_IMPL_QI2(dom::Node, dom::Entity);
  CDA_IMPL_COMPARE_NAIVE(CDA_Entity);

  CDA_Node* shallowCloneNode() throw(std::exception&);
  CDA_IMPL_NODETYPE(ENTITY);
  wchar_t* publicId() throw(std::exception&);
  wchar_t* systemId() throw(std::exception&);
  wchar_t* notationName() throw(std::exception&);

  std::wstring mPublicId, mSystemId, mNotationName;
};

class CDA_EntityReference
  : public iface::dom::EntityReference, public CDA_Node
{
public:
  CDA_EntityReference(CDA_Document* aDocument) : CDA_Node(aDocument) {}
  virtual ~CDA_EntityReference() {}

  CDA_IMPL_QI2(dom::Node, dom::EntityReference);
  CDA_IMPL_COMPARE_NAIVE(CDA_EntityReference);

  CDA_Node* shallowCloneNode() throw(std::exception&);
  CDA_IMPL_NODETYPE(ENTITY_REFERENCE);
};

class CDA_ProcessingInstruction
  : public iface::dom::ProcessingInstruction, public CDA_Node
{
public:
  CDA_ProcessingInstruction(CDA_Document* aDocument,
                            std::wstring aTarget, std::wstring aData)
    : CDA_Node(aDocument)
  {
    mNodeName = aTarget;
    mNodeValue = aData;
  }
  virtual ~CDA_ProcessingInstruction() {}

  CDA_IMPL_QI2(dom::Node, dom::ProcessingInstruction);
  CDA_IMPL_COMPARE_NAIVE(CDA_ProcessingInstruction);

  CDA_Node* shallowCloneNode() throw(std::exception&);
  CDA_IMPL_NODETYPE(PROCESSING_INSTRUCTION);
  wchar_t* target() throw(std::exception&);
  wchar_t* data() throw(std::exception&);
  void data(const wchar_t* attr) throw(std::exception&);
};

class CDA_DocumentFragment
  : public iface::dom::DocumentFragment, public CDA_Node
{
public:
  CDA_DocumentFragment(CDA_Document* aDocument) : CDA_Node(aDocument) {}
  virtual ~CDA_DocumentFragment() {}

  CDA_IMPL_QI2(dom::Node, dom::DocumentFragment);
  CDA_IMPL_COMPARE_NAIVE(CDA_DocumentFragment);
  CDA_IMPL_NODETYPE(DOCUMENT_FRAGMENT)

  CDA_Node* shallowCloneNode() throw(std::exception&);
};

class CDA_Document
  : public virtual iface::dom::Document, public CDA_Node
{
public:
  CDA_Document(const wchar_t* namespaceURI,
               const wchar_t* qualifiedName,
               iface::dom::DocumentType* doctype)
    : CDA_Node(this)
  {
    mNamespaceURI = namespaceURI;
    mNodeName = qualifiedName;
    const wchar_t* pos = wcschr(qualifiedName, L':');
    if (pos == NULL)
    {
      mLocalName = qualifiedName;
    }
    else
    {
      mLocalName = pos + 1;
    }

    appendChild(doctype)->release_ref();
  }

  virtual ~CDA_Document()
  {
  }

  CDA_IMPL_QI2(dom::Node, dom::Document);
  CDA_IMPL_COMPARE_NAIVE(CDA_Document);
  CDA_IMPL_NODETYPE(DOCUMENT)

  iface::dom::DocumentType* doctype() throw(std::exception&);
  iface::dom::DOMImplementation* implementation() throw(std::exception&);
  iface::dom::Element* documentElement() throw(std::exception&);
  iface::dom::Element* createElement(const wchar_t* tagName)
    throw(std::exception&);
  iface::dom::DocumentFragment* createDocumentFragment()
    throw(std::exception&);
  iface::dom::Text* createTextNode(const wchar_t* data) throw(std::exception&);
  iface::dom::Comment* createComment(const wchar_t* data)
    throw(std::exception&);
  iface::dom::CDATASection* createCDATASection(const wchar_t* data)
    throw(std::exception&);
  iface::dom::ProcessingInstruction* createProcessingInstruction
  (const wchar_t* target, const wchar_t* data) throw(std::exception&);
  iface::dom::Attr* createAttribute(const wchar_t* name) throw(std::exception&);
  iface::dom::EntityReference* createEntityReference(const wchar_t* name)
    throw(std::exception&);
  iface::dom::NodeList* getElementsByTagName(const wchar_t* tagname)
    throw(std::exception&);
  iface::dom::Node* importNode(iface::dom::Node* importedNode, bool deep)
    throw(std::exception&);
  iface::dom::Element* createElementNS(const wchar_t* namespaceURI,
                                       const wchar_t* qualifiedName)
    throw(std::exception&);
  iface::dom::Attr* createAttributeNS(const wchar_t* namespaceURI,
                                      const wchar_t* qualifiedName)
    throw(std::exception&);
  iface::dom::NodeList* getElementsByTagNameNS(const wchar_t* namespaceURI,
                                               const wchar_t* localName)
    throw(std::exception&);
  iface::dom::Element* getElementById(const wchar_t* elementId)
    throw(std::exception&);
  iface::events::Event* createEvent(const wchar_t* domEventType)
    throw(std::exception&);
  CDA_Node* shallowCloneNode() throw(std::exception&);
  iface::dom::Element* searchForElementById(const wchar_t* elementId);
};

class CDA_MutationEvent
  : public iface::events::MutationEvent
{
public:
  CDA_MutationEvent()
    : mCancelable(false), mBubbles(true), mCanceled(false),
      mPropagationStopped(false),
      mPhase(iface::events::Event::CAPTURING_PHASE),
      mAttrChange(iface::events::MutationEvent::MODIFICATION)
  {
#ifdef _WIN32
    mTimeStamp = ((uint64_t)time(0)) * 1000;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    mTimeStamp = ((uint64_t)tv.tv_sec) * 1000 + (tv.tv_usec / 1000);
#endif
  }
  virtual ~CDA_MutationEvent() {}

  CDA_IMPL_REFCOUNT;
  CDA_IMPL_QI2(events::Event, events::MutationEvent);
  CDA_IMPL_COMPARE_NAIVE(CDA_MutationEvent);

  wchar_t* type() throw(std::exception&);
  iface::dom::Node* target() throw(std::exception&);
  iface::dom::Node* currentTarget() throw(std::exception&);
  uint16_t eventPhase() throw(std::exception&);
  bool bubbles() throw(std::exception&);
  bool cancelable() throw(std::exception&);
  uint64_t timeStamp() throw(std::exception&);
  void stopPropagation() throw(std::exception&);
  void preventDefault() throw(std::exception&);
  void initEvent(const wchar_t* eventTypeArg, bool canBubbleArg,
                 bool cancelableArg)
    throw(std::exception&);
  iface::dom::Node* relatedNode() throw(std::exception&);
  wchar_t* prevValue() throw(std::exception&);
  wchar_t* newValue() throw(std::exception&);
  wchar_t* attrName() throw(std::exception&);
  uint16_t attrChange() throw(std::exception&);
  void initMutationEvent(const wchar_t* typeArg, bool canBubbleArg,
                         bool cancelableArg, iface::dom::Node* relatedNodeArg,
                         const wchar_t* prevValueArg,
                         const wchar_t* newValueArg,
                         const wchar_t* attrNameArg,
                         uint16_t attrChangeArg)
    throw(std::exception&);

  bool mCancelable, mBubbles, mCanceled, mPropagationStopped;
  std::wstring mType, mPrevValue, mNewValue, mAttrName;
  ObjRef<CDA_Node> mTarget, mCurrentTarget;
  ObjRef<iface::dom::Node> mRelatedNode;
  uint16_t mPhase, mAttrChange;
  uint64_t mTimeStamp;
};