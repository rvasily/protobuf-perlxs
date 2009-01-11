#ifndef GOOGLE_PROTOBUF_COMPILER_PERLXS_HELPERS_H__
#define GOOGLE_PROTOBUF_COMPILER_PERLXS_HELPERS_H__

#include <map>
#include <string>
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {

namespace io { class Printer; }

extern string StringReplace(const string& s, const string& oldsub,
			    const string& newsub, bool replace_all);

namespace compiler {

// A couple of the C++ code generator headers are not installed, but
// we need to call into that code in a few places.  We duplicate the
// function prototypes here.

namespace cpp {
  extern string ClassName(const Descriptor* descriptor, bool qualified);
  extern string ClassName(const EnumDescriptor* enum_descriptor, 
			  bool qualified);
  extern string FieldName(const FieldDescriptor* field);
  extern string StripProto(const string& filename);
}

namespace perlxs {

// Returns the name used in the typemap.
string TypemapName(const Descriptor* descriptor);

// Returns the containing Perl module name for a message descriptor.
string MessageModuleName(const Descriptor* descriptor);

// Returns the Perl class name for a message descriptor.
string MessageClassName(const Descriptor* descriptor);

// Returns the Perl class name for a message descriptor.
string EnumClassName(const EnumDescriptor* descriptor);

// Help with some code for assigning the SV in the getter functions.
void PerlSVGetHelper(io::Printer& printer,
		     const map<string, string>& vars,
		     FieldDescriptor::CppType fieldtype,
		     int depth);

// Print information about an enum value in POD format.
void PODPrintEnumValue(const EnumValueDescriptor *value, io::Printer& printer);

// Return a POD-friendly string representation of a field descriptor type.
string PODFieldTypeString(const FieldDescriptor* field);

void MessageToHashref(const Descriptor * descriptor,
		      io::Printer& printer,
		      map<string, string>& vars,
		      int depth);

void MessageFromHashref(const Descriptor * descriptor,
			io::Printer& printer,
			map<string, string>& vars,
			int depth);

}  // namespace perlxs
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_PERLXS_HELPERS_H__
