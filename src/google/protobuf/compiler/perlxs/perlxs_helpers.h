#ifndef GOOGLE_PROTOBUF_COMPILER_PERLXS_HELPERS_H__
#define GOOGLE_PROTOBUF_COMPILER_PERLXS_HELPERS_H__

#include <map>
#include <string>
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {

namespace io { class Printer; }

namespace compiler {
namespace perlxs {

// Converts the field's name to camel-case, e.g. "foo_bar_baz" becomes
// "FooBarBaz".
string CamelCase(const string& input);

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

}  // namespace perlxs
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_PERLXS_HELPERS_H__
