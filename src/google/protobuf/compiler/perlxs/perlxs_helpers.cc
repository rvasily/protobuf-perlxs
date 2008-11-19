#include <vector>
#include <sstream>
#include <google/protobuf/compiler/perlxs/perlxs_helpers.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.pb.h>

namespace google {
namespace protobuf {

extern string StringReplace(const string& s, const string& oldsub,
			    const string& newsub, bool replace_all);

namespace compiler {
namespace perlxs {


string
TypemapName(const Descriptor* descriptor)
{
  string name = descriptor->full_name();

  name = "T_" + StringReplace(name, ".", "_", true);
  for ( string::iterator i = name.begin(); i != name.end(); ++i ) {
    // toupper() changes based on locale.  We don't want this!
    if ('a' <= *i && *i <= 'z') *i += 'A' - 'a';
  }

  return name;
}


// Returns the containing Perl module name for a message descriptor.

string
MessageModuleName(const Descriptor* descriptor)
{
  const Descriptor *container = descriptor;

  while (container->containing_type() != NULL) {
    container = container->containing_type();
  }

  return MessageClassName(container);
}

// Returns the Perl class name for a message descriptor.

string
MessageClassName(const Descriptor* descriptor)
{
  string classname = descriptor->full_name();

  classname = StringReplace(classname, ".", "::", true);

  return classname;
}

// Returns the Perl class name for a message descriptor.

string
EnumClassName(const EnumDescriptor* descriptor)
{
  string classname = descriptor->full_name();

  classname = StringReplace(classname, ".", "::", true);

  return classname;
}


void
PerlSVGetHelper(io::Printer& printer,
		const map<string, string>& vars,
		FieldDescriptor::CppType fieldtype,
		int depth)
{
  for ( int i = 0; i < depth; i++ ) {
    printer.Indent();
  }

  switch ( fieldtype ) {
  case FieldDescriptor::CPPTYPE_INT32:
  case FieldDescriptor::CPPTYPE_BOOL:
  case FieldDescriptor::CPPTYPE_ENUM:
    printer.Print(vars,
		  "sv = sv_2mortal(newSViv(THIS->$cppname$($i$)));\n");
    break;
  case FieldDescriptor::CPPTYPE_UINT32:
    printer.Print(vars,
		  "sv = sv_2mortal(newSVuv(THIS->$cppname$($i$)));\n");
    break;
  case FieldDescriptor::CPPTYPE_FLOAT:
  case FieldDescriptor::CPPTYPE_DOUBLE:
    printer.Print(vars,
		  "sv = sv_2mortal(newSVnv(THIS->$cppname$($i$)));\n");
    break;
  case FieldDescriptor::CPPTYPE_INT64:
  case FieldDescriptor::CPPTYPE_UINT64:
    printer.Print(vars,
		  "ost.str(\"\");\n"
		  "ost << THIS->$cppname$($i$);\n"
		  "sv = sv_2mortal(newSVpv(ost.str().c_str(),\n"
		  "                        ost.str().length()));\n");
    break;
  case FieldDescriptor::CPPTYPE_STRING:
    printer.Print(vars,
		  "sv = sv_2mortal(newSVpv(THIS->$cppname$($i$).c_str(),\n"
		  "                        "
		  "THIS->$cppname$($i$).length()));\n");
    break;
  case FieldDescriptor::CPPTYPE_MESSAGE:
    printer.Print(vars,
		  "val = new $fieldtype$;\n"
		  "val->CopyFrom(THIS->$cppname$($i$));\n"
		  "sv = sv_newmortal();\n"
		  "sv_setref_pv(sv, \"$fieldclass$\", (void *)val);\n");
    break;
  default:
    printer.Print("sv = &PL_sv_undef;\n");
    break;
  }

  for ( int i = 0; i < depth; i++ ) {
    printer.Outdent();
  }
}


void
PODPrintEnumValue(const EnumValueDescriptor *value, io::Printer& printer)
{
  ostringstream ost;
  printer.Print("=item B<*value*>\n"
		"\n",
		"value", value->name());
  ost << value->number();
  printer.Print("This constant has a value of *number*.\n"
		"\n",
		"number", ost.str());
}


string
PODFieldTypeString(const FieldDescriptor* field)
{
  string type;

  switch ( field->cpp_type() ) {
  case FieldDescriptor::CPPTYPE_INT32:
    type = "a 32-bit signed integer";
    break;
  case FieldDescriptor::CPPTYPE_BOOL:
    type = "a Boolean value";
    break;
  case FieldDescriptor::CPPTYPE_ENUM:
    type = "a value of " + EnumClassName(field->enum_type());
    break;
  case FieldDescriptor::CPPTYPE_UINT32:
    type = "a 32-bit unsigned integer";
    break;
  case FieldDescriptor::CPPTYPE_FLOAT:
  case FieldDescriptor::CPPTYPE_DOUBLE:
    type = "a floating point number";
    break;
  case FieldDescriptor::CPPTYPE_INT64:
    type = "a 64-bit signed integer";
    break;
  case FieldDescriptor::CPPTYPE_UINT64:
    type = "a 64-bit unsigned integer";
    break;
  case FieldDescriptor::CPPTYPE_STRING:
    type = "a string";
    break;
  case FieldDescriptor::CPPTYPE_MESSAGE:
    type = "an instance of " + MessageClassName(field->message_type());
    break;
  default:
    type = "an unknown type";
    break;
  }

  return type;
}


}  // namespace perlxs
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
