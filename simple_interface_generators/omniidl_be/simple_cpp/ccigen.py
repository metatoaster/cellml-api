# -*- python -*-

from omniidl import idlvisitor, idlast, idltype;
from omniidl import output;
import os;
import conversionutils;
import simplecxx;
import string
import identifier

class Walker(idlvisitor.AstVisitor):
    """Walks over the AST once and writes the CCI header or CCI as it goes.
    """
    def visitAST(self, node):
        """Visit all the declarations in an AST"""

        self.beenIncluded = {}
        self.masterFile = node.file()
        self.masterGuard = ''
        self.scope = ["CCI"]
        self.scopeEntryDepth = 0
        for i in node.filebase:
            if (i >= 'a' and i <= 'z') or (i >= 'A' and i <= 'Z'):
                self.masterGuard = self.masterGuard + i
        self.cci.out("/* This file is automatically generated from " +\
                     node.filename + """
 * DO NOT EDIT DIRECTLY OR CHANGES WILL BE LOST!
 */""")
        if not self.doing_header:
            self.cci.out("""
#include <omniORB4/CORBA.h>
#include "cda_compiler_support.h"
#include <strings.h>
#include <wchar.h>
#include <exception>
#include "corba_support/WrapperRepository.hxx"
            """)
            self.cci.out('#ifndef MODULE_CONTAINS_' + self.masterGuard)
            self.cci.out('#define MODULE_CONTAINS_' + self.masterGuard)
            self.cci.out('#endif')
            self.cci.out('#include "CCI' + node.filebase + '.hxx"')
            self.cci.out('#include "SCI' + node.filebase + '.hxx"')
        else:
            self.cci.out('#include "cda_compiler_support.h"')
            self.cci.out('#ifndef _CCI' + self.masterGuard + '_hxx')
            self.cci.out('#define _CCI' + self.masterGuard + '_hxx')
            self.cci.out('')
            self.cci.out('#include "Iface' + node.filebase + '.hxx"')
            self.cci.out('#include "' + node.filebase + '.hh"')
            self.cci.out('#ifdef MODULE_CONTAINS_' + self.masterGuard)
            self.cci.out('#define PUBLIC_' + self.masterGuard + '_PRE CDA_EXPORT_PRE')
            self.cci.out('#define PUBLIC_' + self.masterGuard + '_POST CDA_EXPORT_POST')
            self.cci.out('#else')
            self.cci.out('#define PUBLIC_' + self.masterGuard + '_PRE CDA_IMPORT_PRE')
            self.cci.out('#define PUBLIC_' + self.masterGuard + '_POST CDA_IMPORT_POST')
            self.cci.out('#endif')
        for n in node.declarations():
            if n.file() == self.masterFile:
                n.accept(self)
            elif self.doing_header:
                self.considerIncluding(n.file())
        if self.doing_header:
            self.escapeScopes()
            self.cci.out('#undef PUBLIC_' + self.masterGuard + '_PRE')
            self.cci.out('#undef PUBLIC_' + self.masterGuard + '_POST')
            self.cci.out('#endif // _CCI' + self.masterGuard + '_hxx')

    def escapeScopes(self):
        for i in range(0, self.scopeEntryDepth):
            self.cci.dec_indent()
            self.cci.out('};')
        self.scopeEntryDepth = 0

    def writeScopes(self):
        for i in range(self.scopeEntryDepth, len(self.scope)):
            self.cci.out('namespace ' + self.scope[i])
            self.cci.out('{')
            self.cci.inc_indent()
        self.scopeEntryDepth = len(self.scope)

    def enterScope(self, node):
        self.scope.append(node.simplename)

    def leaveScope(self):
        self.scope = self.scope[:-1]
        if self.scopeEntryDepth > len(self.scope):
            self.scopeEntryDepth = len(self.scope)
            self.cci.dec_indent()
            self.cci.out('};')

    def considerIncluding(self, name):
        if (self.beenIncluded.has_key(name)):
            return
        self.beenIncluded[name] = 1;
        self.escapeScopes()
        basename,ext = os.path.splitext(name)
        self.cci.out('#include "CCI' + basename  + '.hxx"')
        self.cci.inModule = 0

    def visitModule(self, node):
        """Visit all the definitions in a module."""
        self.enterScope(node)

        for n in node.definitions():
            if n.file() == self.masterFile:
                if self.doing_header:
                    self.writeScopes()
                n.accept(self)
            elif self.doing_header:
                self.considerIncluding(n.file())
        self.leaveScope()

    def processBase(self, active, base):
        # Interface active has base(or active == base). Called only once
        # per active per base. We only care about callables here.

        psemi = ''
        pfq = 'CCI::' + active.finalcciscoped + '::'
        if self.doing_header:
            psemi = ';'
            pfq = ''

        downcastName = '_downcast_' + string.join(base.scopedName(), '_')
        self.cci.out('::' + base.corbacxxscoped + '_ptr ' + pfq +\
                     downcastName + '()' + psemi)
        if not self.doing_header:
            self.cci.out('{')
            self.cci.inc_indent()
            self.cci.out('return _objref;')
            self.cci.dec_indent()
            self.cci.out('}')

    def writeAddRef(self):
        self.cci.out('{')
        self.cci.inc_indent()
        self.cci.out('_refcount++;')
        self.cci.dec_indent()
        self.cci.out('}')

    def writeReleaseRef(self):
        self.cci.out('{')
        self.cci.inc_indent()
        self.cci.out('_refcount--;')
        self.cci.out('if (_refcount == 0)')
        self.cci.out('{')
        self.cci.inc_indent()
        # Delete...
        self.cci.out('try')
        self.cci.out('{')
        self.cci.inc_indent()
        self.cci.out('delete this;')
        self.cci.dec_indent()
        self.cci.out('}')
        self.cci.out('catch (CORBA::Exception& e)')
        self.cci.out('{')
        self.cci.out('}')
        self.cci.dec_indent()
        self.cci.out('}')
        self.cci.dec_indent()
        self.cci.out('}')

    def writeQueryInterface(self):
        self.cci.out('{')
        self.cci.inc_indent()
        # self.cci.out('CORBA::String_var cid = (const char*)id;')
        # Write a CORBA query_interface...
        self.cci.out('::XPCOM::IObject_var cobj = ' +\
                     '_downcast_XPCOM_IObject()->query_interface(id);')
        # From this point, cobj needs to be release_refd or used...
        # Attempt to wrap it...
        self.cci.out('void* sobj = ' +\
                     'CORBA::is_nil(cobj) ? NULL : gWrapperRepository().NewCCI(id, cobj, _getPOA());')
        self.cci.out('if (sobj != NULL)')
        self.cci.inc_indent()
        self.cci.out('cobj->release_ref();')
        self.cci.dec_indent()
        self.cci.out('return sobj;')
        self.cci.dec_indent()
        self.cci.out('}')

    def visitOperation(self, op):
        active = self.active_interface
        downcastStr = '_downcast_' + string.join(active.scopedName(), '_') +\
                      '()'
        psemi = ''
        pfq = 'CCI::' + active.corbacxxscoped + '::'
        if self.doing_header:
            psemi = ';'
            pfq = ''
        # Firstly, get the return type...
        rtype = simplecxx.typeToSimpleCXX(op.returnType())
        if op.simplename == 'query_interface':
            rtype = 'void*'
        # Next, build up all the parameters...
        parstr = ''
        callstr = ''
        needcomma = 0
        for p in op.parameters():
            if needcomma:
                parstr = parstr + ', '
            else:
                needcomma = 1
            if p.is_out():
                extrapointer = 1
            else:
                extrapointer = 0
            if simplecxx.doesTypeNeedLength(p.paramType()):
                parstr = parstr + 'uint32_t'
                if p.is_out():
                    parstr = parstr + '*'
                parstr = parstr + ' _length_' + p.simplename + ', '
            parstr = parstr + simplecxx.typeToSimpleCXX(p.paramType(), \
                                                        extrapointer, not p.is_out()) +\
                     ' ' + p.simplename

        if simplecxx.doesTypeNeedLength(op.returnType()):
            if needcomma:
                parstr = parstr + ', '
            parstr = parstr + 'uint32_t* _length__return'

        self.cci.out(rtype + ' ' + pfq + op.simplename + '(' + parstr +\
                     ') throw(std::exception&)' + psemi)
        if self.doing_header:
            return
        self.cci.ci_count = 0
        if op.simplename == 'add_ref':
            self.writeAddRef()
        elif op.simplename == 'release_ref':
            self.writeReleaseRef()
        elif op.simplename == 'query_interface':
            self.writeQueryInterface()
        else:
            self.cci.out('{')
            self.cci.inc_indent()

            # All in parameters get converted to CORBA parameters
            for p in op.parameters():
                if callstr != '':
                    callstr = callstr + ','
                callstr = callstr + '_corba_' + p.simplename
                self.declareCORBAStorage(p.paramType(), '_corba_' + p.simplename)
                # If it isn't an in parameter, leave it for now...
                if not p.is_in():
                    continue
                if simplecxx.doesTypeNeedLength(p.paramType()):
                    sname = p.simplename
                    slength = '_length_' + p.simplename
                    if p.is_out():
                        sname = '(*' + p.simplename + ')'
                        slength = '(*_length_' + p.simplename + ')'
                    self.simpleSequenceToCORBA(p.paramType(), sname, slength, \
                                               '_corba_' + p.simplename)
                else:
                    sname = p.simplename
                    if p.is_out():
                        sname = '(*' + sname + ')'
                    self.simpleValueToCORBA(p.paramType(), sname, \
                                            '_corba_' + p.simplename)
            # Declare storage for the return value...
            rt = op.returnType()
            returns = (rt.kind() != idltype.tk_void)
            retprefix = ''
            if returns:
                self.declareCORBAStorage(rt, '_corba_return')
                retprefix = '_corba_return = '
            self.cci.out('try')
            self.cci.out('{')
            self.cci.inc_indent()
            # Next, make the call...
            self.cci.out(retprefix + downcastStr + '->' +\
                         op.simplename + '(' + callstr + ');')

            for p in op.parameters():
                if p.is_out():
                    if p.is_in():
                        if simplecxx.doesTypeNeedLength(p.paramType()):
                            conversionutils.destroySimpleSequence(\
                                self.cci, p.paramType(), '(*' + p.simplename + ')',
                                '(*_length_' + p.simplename + ')')
                        else:
                            conversionutils.destroySimpleValue(\
                                self.cci, \
                                p.paramType(), \
                                '(*' + p.simplename + ')')
                    # Assign the simple value from the CORBA value.
                    if simplecxx.doesTypeNeedLength(p.paramType()):
                        sname = '(*' + p.simplename + ')'
                        slength = '(*_length_' + p.simplename + ')'
                        self.CORBASequenceToSimple(p.paramType(),\
                                                   '_corba_' + p.simplename,\
                                                   sname, slength, needAlloc=1)
                    else:
                        sname = '(*' + p.simplename + ')'
                        self.CORBAValueToSimple(p.paramType(),\
                                                   '_corba_' + p.simplename,\
                                                   sname)
                if p.is_in():
                    # Free the CORBA value...
                    if simplecxx.doesTypeNeedLength(p.paramType()):
                        conversionutils.destroyCORBASequence(\
                            self.cci, \
                            p.paramType(), \
                            '_corba_' + p.simplename)
                    else:
                        conversionutils.destroyCORBAValue(\
                            self.cci, \
                            p.paramType(), \
                            '_corba_' + p.simplename)

            if returns:
                self.declareSimpleStorage(rt, '_simple_return')
                if simplecxx.doesTypeNeedLength(rt):
                    self.CORBASequenceToSimple(rt, '_corba_return',
                                               '_simple_return', '(*_length__return)',
                                               needAlloc=1)
                    conversionutils.destroyCORBASequence(\
                        self.cci, rt, '_corba_return')
                else:
                    self.CORBAValueToSimple(rt, '_corba_return', '_simple_return')
                    conversionutils.destroyCORBAValue(\
                        self.cci, rt, '_corba_return')
                self.cci.out('return _simple_return;')

            self.cci.dec_indent()
            self.cci.out('}')
            self.cci.out('catch (CORBA::Exception& e)')
            self.cci.out('{')
            self.cci.inc_indent()
            self.cci.out('throw std::exception(/*"A CORBA exception ' +\
                         'occurred."*/);');
            self.cci.dec_indent()
            self.cci.out('}')
            self.cci.dec_indent()
            self.cci.out('}')

    def visitAttribute(self, at):
        active = self.active_interface
        downcastStr = '_downcast_' + string.join(active.scopedName(), '_') +\
                      '()'
        psemi = ''
        pfq = 'CCI::' + active.corbacxxscoped + '::'
        if self.doing_header:
            psemi = ';'
            pfq = ''
        typename = simplecxx.typeToSimpleCXX(at.attrType())
        typenameC = simplecxx.typeToSimpleCXX(at.attrType(), is_const=1)
        if simplecxx.doesTypeNeedLength(at.attrType()):
            extra_getter_params = 'uint32_t* _length_attr'
            extra_setter_params = ', uint32_t _length_attr'
        else:
            extra_getter_params = ''
            extra_setter_params = ''
        for n in at.declarators():
            self.cci.out(typename + ' ' + pfq + n.simplename + '(' +
                         extra_getter_params +
                         ') throw(std::exception&)' + psemi)
            if not self.doing_header:
                self.cci.out('{')
                self.cci.inc_indent()
                self.cci.out('try')
                self.cci.out('{')
                self.cci.inc_indent()
                # 1) Declare storage...
                self.declareCORBAStorage(at.attrType(), '_corba_value')
                # 2) Call the getter and assign...
                self.cci.out('_corba_value = ' + downcastStr + '->' +\
                             n.simplename + '();')
                self.cci.ci_count = 0
                self.declareSimpleStorage(at.attrType(), '_simple_value')
                if simplecxx.doesTypeNeedLength(at.attrType()):
                    self.CORBASequenceToSimple(at.attrType(), '_corba_value',
                                               '_simple_value', '(*_length_attr)',
                                               needAlloc=1)
                    conversionutils.destroyCORBASequence(self.cci,\
                                                         at.attrType(),\
                                                         '_corba_value')
                else:
                    self.CORBAValueToSimple(at.attrType(), '_corba_value',\
                                            '_simple_value')
                    conversionutils.destroyCORBAValue(self.cci,\
                                                      at.attrType(),\
                                                      '_corba_value')
                self.cci.out('return _simple_value;')
                self.cci.dec_indent()
                self.cci.out('}')
                self.cci.out('catch (CORBA::Exception& cce)')
                self.cci.out('{')
                self.cci.inc_indent()
                self.cci.out('throw std::exception(/*"A CORBA error occurred"*/);')
                self.cci.dec_indent()
                self.cci.out('}')
                self.cci.dec_indent()
                self.cci.out('}')
            if not at.readonly():
                self.cci.out('void ' + pfq + n.simplename + '(' + typenameC +
                             ' attr' + extra_setter_params +
                             ') throw(std::exception&)' + psemi)
                if not self.doing_header:
                    self.cci.out('{')
                    self.cci.inc_indent()
                    self.cci.out('try')
                    self.cci.out('{')
                    self.cci.inc_indent()
                    self.cci.ci_count = 0
                    self.declareCORBAStorage(at.attrType(), '_corba_value')
                    if simplecxx.doesTypeNeedLength(at.attrType()):
                        self.simpleSequenceToCORBA(at.attrType(), 'attr',
                                                   '_length_attr', '_corba_value')
                    else:
                        self.simpleValueToCORBA(at.attrType(), 'attr', \
                                                '_corba_value')
                    # Finally, call the setter...
                    self.cci.out(downcastStr + '->' + n.simplename +\
                                 '(_corba_value);')
                    self.cci.dec_indent()
                    self.cci.out('}')
                    self.cci.out('catch (CORBA::Exception& cce)')
                    self.cci.out('{')
                    self.cci.inc_indent()
                    self.cci.out('throw std::exception(' +\
                                 '/*"A CORBA error occurred"*/);')
                    self.cci.dec_indent()
                    self.cci.out('}')
                    self.cci.dec_indent()
                    self.cci.out('}')

    def declareSimpleStorage(self, type, name):
        self.cci.out(simplecxx.typeToSimpleCXX(type) + ' ' + name + ';')

    def declareCORBAStorage(self, type, name):
        # We need to get a storage string...
        self.cci.out(conversionutils.getCORBAVarType(type) + ' ' + name +\
                     ';')

    def simpleSequenceToCORBA(self, type, sarray, slength, cname):
        conversionutils.writeSimpleSequenceToCORBA(self.cci, type, sarray,
                                                   slength, cname)

    def simpleValueToCORBA(self, type, sname, cname):
        conversionutils.writeSimpleToCORBA(self.cci, type, sname, cname)

    def CORBASequenceToSimple(self, type, cname, sarray, slength, fromCall=0,
                              needAlloc=0):
        conversionutils.writeCORBASequenceToSimple(self.cci, type, cname,
                                                   sarray, slength, fromCall,
                                                   needAlloc)

    def CORBAValueToSimple(self, type, cname, sname):
        conversionutils.writeCORBAValueToSimple(self.cci, type, cname, sname)

    def writeIObjectSpecials(self):
        # XPCOM::IObject gets some special treatment, since it is the base...
        self.cci.out('public:')
        self.cci.inc_indent()
        self.cci.out('IObject();')
        self.cci.dec_indent()
        self.cci.out('protected:')
        self.cci.inc_indent()
        self.cci.out('::PortableServer::POA_ptr _getPOA();')
        self.cci.out('void _setPOA(::PortableServer::POA_ptr aPOA);')
        self.cci.dec_indent()
        self.cci.out('private:')
        self.cci.inc_indent()
        self.cci.out('::PortableServer::POA_var _poa;')
        self.cci.out('uint32_t _refcount;')
        self.cci.dec_indent()

    def writeIObjectSpecialImpl(self):
        # XPCOM::IObject gets some special treatment, since it is the base...
        self.cci.out('CCI::XPCOM::IObject::IObject() : _refcount(1) {};')
        self.cci.out('void CCI::XPCOM::IObject::_setPOA(::PortableServer::POA_ptr aPOA)')
        self.cci.out('{')
        self.cci.inc_indent()
        self.cci.out('_poa = aPOA;')
        self.cci.dec_indent()
        self.cci.out('}')
        self.cci.out('::PortableServer::POA_ptr CCI::XPCOM::IObject::_getPOA()')
        self.cci.out('{')
        self.cci.inc_indent()
        self.cci.out('return _poa;')
        self.cci.dec_indent()
        self.cci.out('}')

    def writeUnwrap(self, node):
        unwrapName = '_unwrap_' + string.join(node.scopedName(), '_')
        downcastName = '_downcast_' + string.join(node.scopedName(), '_')
        psemi = ''
	ppre = ''
        pfq = 'CCI::' + node.corbacxxscoped + '::'
        if self.doing_header:
	    ppre = 'PUBLIC_' + self.masterGuard + '_PRE '
            psemi = 'PUBLIC_' + self.masterGuard + '_POST;'
            pfq = ''
        self.cci.out(ppre + '::' + node.corbacxxscoped + '_ptr ' + pfq +\
                     unwrapName + '()' + psemi)
        if self.doing_header:
            self.cci.out('virtual ::' + node.corbacxxscoped + '_ptr ' +\
                         downcastName + '() = 0;')
        else:
            self.cci.out('{')
            self.cci.inc_indent()
            self.cci.out('::' + node.corbacxxscoped + '_ptr tmp = ' +\
                         downcastName + '();')
            self.cci.out('if (!CORBA::is_nil(tmp))')
            self.cci.out('  tmp->add_ref();')
            self.cci.out('return ::' + node.corbacxxscoped + '::_duplicate(tmp);')
            self.cci.dec_indent()
            self.cci.out('}')

    def visitInterface(self, node):
        isTerminal = 0
        exportFrom = 0
        if self.doing_header:
            #self.cci.out('#ifdef HAVE_VISIBILITY_EXPORT')
            #self.cci.out('extern C {')
            # This is a really ugly hack to export the typeinfo without
            # exporting the whole class on gcc...
            #mangledName = '_ZTIN3CCI' + node.lengthprefixed + 'E'
            #self.cci.out('PUBLIC_' + self.masterGuard + '_PRE void* ' +\
            #             mangledName + ' PUBLIC_' + self.masterGuard + '_POST;')
            #self.cci.out('}')
            #self.cci.out('#endif')

            for p in node.pragmas():
                if p.text() == "terminal-interface":
                    isTerminal = 1
                if p.text() == "cross-module-inheritance" or \
                       p.text() == "cross-module-argument":
                    exportFrom = 1
	    if exportFrom:
                self.cci.out('PUBLIC_' + self.masterGuard + '_PRE ')
                self.cci.out('class PUBLIC_' + self.masterGuard + '_POST ' +
                             node.simplename)
            else:
                self.cci.out('class ' + node.simplename)

	    inheritstr = node.simplecxxscoped

            for c in node.inherits():
                isAmbiguous = 0
                target = 'ambiguous-inheritance(' + c.corbacxxscoped + ')'
                for p in node.pragmas():
                    if p.text() == target:
                        isAmbiguous = 1
                        break
                if c.corbacxxscoped == 'XPCOM::IObject':
                    isAmbiguous = 1
                if isAmbiguous:
		     inheritstr = inheritstr +\
			          ', public virtual CCI::' + c.corbacxxscoped
                else:
		    inheritstr = inheritstr +\
				 ', public CCI::' + c.corbacxxscoped

            if isTerminal:
                self.cci.out('  : public ' + inheritstr)
            else:
                self.cci.out('  : public virtual ' + inheritstr)

            self.cci.out('{')
            if (node.corbacxxscoped == 'XPCOM::IObject'):
                self.writeIObjectSpecials()
            self.cci.out('public:')
            self.cci.inc_indent()

            # Also put a trivial virtual destructor...
            self.cci.out('virtual ~' + node.simplename + '(){}')
        else:
            if (node.corbacxxscoped == 'XPCOM::IObject'):
                self.writeIObjectSpecialImpl()

        self.active_interface = node
        self.writeUnwrap(node)
        for c in node.callables():
            c.accept(self)

        if self.doing_header:
            self.cci.dec_indent()
            self.cci.out('};')
            self.cci.out('class _final_' + node.simplename)
            self.cci.out('  : public ' + node.simplename)
            self.cci.out('{')
            self.cci.out('private:')
            self.cci.inc_indent()
            self.cci.out('::' + node.corbacxxscoped + '_var _objref;')
            self.cci.dec_indent()
            self.cci.out('public:')
            self.cci.inc_indent()
            self.cci.out('PUBLIC_' + self.masterGuard + '_PRE _final_' +\
		         node.simplename + '(::' + node.corbacxxscoped +\
                         '_ptr _aobjref, ::PortableServer::POA_ptr aPp) ' +\
                         'PUBLIC_' + self.masterGuard + '_POST;')
            self.cci.out('virtual ~_final_' + node.simplename + '()')
            self.cci.out('{')
            self.cci.inc_indent()
            self.cci.out('if (!CORBA::is_nil(_objref))')
            self.cci.out('  _objref->release_ref();')
            self.cci.dec_indent()
            self.cci.out('}')
        else:
            self.cci.out('CCI::' + node.finalcciscoped + '::_final_' +\
                         node.simplename + '(::' +\
                         node.corbacxxscoped + '_ptr _aobjref,' +
                         '::PortableServer::POA_ptr aPp)')
            self.cci.out('{')
            self.cci.inc_indent()
            self.cci.out('_objref = ::' + node.corbacxxscoped +\
                         '::_duplicate(_aobjref);')
            self.cci.out('_objref->add_ref();')
            self.cci.out('_setPOA(::PortableServer::POA::_duplicate(aPp));')
            self.cci.dec_indent()
            self.cci.out('}')

        stack = [node]
        seen = {node.simplecxxscoped: 1}
        self.processBase(node, node)
        while len(stack) != 0:
            current = stack.pop()
            for ifa in current.inherits():
                if not seen.has_key(ifa.simplecxxscoped):
                    seen[ifa.simplecxxscoped] = 1
                    while isinstance(ifa, idlast.Declarator):
                        ifa = ifa.alias().aliasType().decl()
                        identifier.AnnotateByRepoID(ifa)
                    stack.append(ifa)
                    self.processBase(node, ifa)

        if self.doing_header:
            self.cci.dec_indent()
            self.cci.out('};')
            self.cci.out('class _factory_' + node.simplename)
            self.cci.out('  : public ::CCIFactory')
            self.cci.out('{')
            self.cci.out('public:')
            self.cci.inc_indent()
            self.cci.out('_factory_' + node.simplename + '();')
            self.cci.out('const char* Name() const { return "' +\
                         node.corbacxxscoped + '"; }')
            self.cci.out('void* MakeCCI(::XPCOM::IObject' +\
                         '_ptr aObj, ::PortableServer::POA_ptr aPp) const')
            self.cci.out('{')
            self.cci.inc_indent()
            self.cci.out('::' + node.corbacxxscoped + '_var obj = ::' +\
                         node.corbacxxscoped + '::_narrow(aObj);')
            self.cci.out('if (CORBA::is_nil(obj)) return NULL;')
            self.cci.out('return static_cast< ' + node.simplecxxscoped +\
                         '* >(new ::CCI::' + node.finalcciscoped + '(' +\
                         'obj, aPp));')
            self.cci.dec_indent()
            self.cci.out('}')
            self.cci.dec_indent()
            self.cci.out('};')
            self.cci.out('PUBLIC_' + self.masterGuard + '_PRE void prod' + node.simplename +\
			 '() PUBLIC_' + self.masterGuard + '_POST;')
        else:
            self.cci.out('::CCI::' + node.factoryscoped + '::_factory_' +\
                         node.simplename + '()')
            self.cci.out('{')
            self.cci.inc_indent()
            self.cci.out('gWrapperRepository().RegisterCCIFactory(this);')
            self.cci.dec_indent()
            self.cci.out('}')
            self.cci.out('::CCI::' + node.factoryscoped + ' gCCIFactory' +\
                         node.simplecscoped + ';')
            self.writeScopes()
            self.cci.out('void prod' + node.simplename + '() {'+\
                         ' gCCIFactory' + node.simplecscoped + '.Name(); }')
            self.escapeScopes()

def run(tree):
    w = Walker()
    w.cci = output.Stream(open("CCI" + tree.filebase + ".cxx", "w"), 2)
    w.doing_header = 0
    tree.accept(w)
    w.cci = output.Stream(open("CCI" + tree.filebase + ".hxx", "w"), 2)
    w.doing_header = 1
    tree.accept(w)
