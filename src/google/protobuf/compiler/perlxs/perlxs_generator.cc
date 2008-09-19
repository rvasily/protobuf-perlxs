#include <iostream>
#include <sstream>
#include <google/protobuf/compiler/perlxs/perlxs_generator.h>
#include <google/protobuf/compiler/perlxs/perlxs_helpers.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream.h>

namespace google {
namespace protobuf {

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

// Now we can move on to the Perl/XS stuff.

namespace perlxs {

PerlXSGenerator::PerlXSGenerator() {}
PerlXSGenerator::~PerlXSGenerator() {}

  
bool
PerlXSGenerator::Generate(const FileDescriptor* file,
			  const string& parameter,
			  OutputDirectory* outdir,
			  string* error) const 
{
  // Each top-level message get its own XS source file, Perl module,
  // and typemap.  Each top-level enum gets its own Perl module.  The
  // files are generated in the perlxs_out directory.

  for ( int i = 0; i < file->message_type_count(); i++ ) {
    const Descriptor* message_type = file->message_type(i);

    GenerateMessageXS(message_type, outdir);
    GenerateMessageModule(message_type, outdir);
    GenerateMessageTypemap(message_type, outdir);
  }

  for ( int i = 0; i < file->enum_type_count(); i++ ) {
    const EnumDescriptor* enum_type = file->enum_type(i);

    GenerateEnumModule(enum_type, outdir);
  }

  return true;
}


void
PerlXSGenerator::GenerateMessageXS(const Descriptor* descriptor,
				   OutputDirectory* outdir) const
{
  string filename = CamelCase(descriptor->name()) + ".xs";
  scoped_ptr<io::ZeroCopyOutputStream> output(outdir->Open(filename));
  io::Printer printer(output.get(), '$'); // '$' works well in the .xs file

  string base = cpp::StripProto(descriptor->file()->name());

  // Boilerplate at the top of the file.

  printer.Print("#ifdef __cplusplus\n"
		"extern \"C\" {\n"
		"#endif\n"
		"#include \"EXTERN.h\"\n"
		"#include \"perl.h\"\n"
		"#include \"XSUB.h\"\n"
		"#ifdef __cplusplus\n"
		"}\n"
		"#endif\n"
		"#ifdef do_open\n"
		"#undef do_open\n"
		"#endif\n"
		"#ifdef do_close\n"
		"#undef do_close\n"
		"#endif\n"
		"#ifdef New\n"
		"#undef New\n"
		"#endif\n"
		"#include <sstream>\n"
		"#include \"$base$.pb.h\"\n"
		"\n"
		"using namespace std;\n"
		"\n",
		"base", base);

  // Typedefs and XS packages

  set<const Descriptor*> seen;

  GenerateFileXSTypedefs(descriptor->file(), printer, seen);

  printer.Print("\n\n");

  GenerateMessageXSPackage(descriptor, printer);
}


void
PerlXSGenerator::GenerateMessageModule(const Descriptor* descriptor,
				       OutputDirectory* outdir) const
{
  string filename = CamelCase(descriptor->name()) + ".pm";
  scoped_ptr<io::ZeroCopyOutputStream> output(outdir->Open(filename));
  io::Printer printer(output.get(), '*'); // '*' works well in the .pm file

  map<string, string> vars;

  vars["package"] = MessageModuleName(descriptor);
  vars["message"] = descriptor->full_name();
  vars["name"]    = descriptor->name();

  printer.Print(vars,
		"package *package*;\n"
		"\n"
		"use strict;\n"
		"use warnings;\n"
		"use vars qw(@ISA $AUTOLOAD $VERSION);\n"
		"\n"
		"$VERSION = '1.0';\n"
		"\n"
		"use Exporter;\n"
		"\n"
		"require DynaLoader;\n"
		"require AutoLoader;\n"
		"\n"
		"@ISA = qw(DynaLoader Exporter);\n"
		"\n"
		"bootstrap *package* $VERSION;\n"
		"\n"
		"1;\n"
		"\n"
		"__END__\n"
		"\n");
  
  // Now generate POD documentation for the module.

  printer.Print(vars,
		"=pod\n"
		"\n"
		"=head1 NAME\n"
		"\n"
		"*package* - Perl/XS interface to *message*\n"
		"\n"
		"=head1 SYNOPSIS\n"
		"\n"
		"=head2 Serializing messages\n"
		"\n"
		" #!/usr/bin/perl\n"
		"\n"
		" use strict;\n"
		" use warnings;\n"
		" use *package*;\n"
		"\n"
		" my $*name* = *package*->new;\n"
		" # Set fields in $*name*...\n"
		" my $pack*name* = $*name*->pack();\n"
		"\n"
		"=head2 Unserializing messages\n"
		"\n"
		" #!/usr/bin/perl\n"
		"\n"
		" use strict;\n"
		" use warnings;\n"
		" use *package*;\n"
		"\n"
		" my $pack*name*; # Read this from somewhere...\n"
		" my $*name* = *package*->new;\n"
		" if ( $*name*->unpack($pack*name*) ) {\n"
		"   print \"OK\"\n"
		" } else {\n"
		"   print \"NOT OK\"\n"
		" }\n"
		"\n"
		"=head1 DESCRIPTION\n"
		"\n"
		"*package* defines the following classes:\n"
		"\n"
		"=over 5\n"
		"\n");

  // List of classes

  GenerateDescriptorClassNamePOD(descriptor, printer);
  
  printer.Print("\n"
		"=back\n"
		"\n");

  GenerateDescriptorMethodPOD(descriptor, printer);

  printer.Print(vars,
		"=head1 AUTHOR\n"
		"\n"
		"Generated from *message* by the protoc compiler.\n"
		"\n"
		"=head1 SEE ALSO\n"
		"\n");

  // Top-level messages in dependency files (recursively expanded)

  printer.Print("http://code.google.com/p/protobuf\n"
		"\n"
		"=cut\n"
		"\n");
}


void
PerlXSGenerator::GenerateDescriptorClassNamePOD(const Descriptor* descriptor,
						io::Printer& printer) const
{
  for ( int i = 0; i < descriptor->enum_type_count(); i++ ) {
    printer.Print("=item C<*name*>\n"
		  "\n"
		  "A wrapper around the *enum* enum\n"
		  "\n",
		  "name", EnumClassName(descriptor->enum_type(i)),
		  "enum", descriptor->enum_type(i)->full_name());    
  }

  for ( int i = 0; i < descriptor->nested_type_count(); i++ ) {
    GenerateDescriptorClassNamePOD(descriptor->nested_type(i), printer);
  }

  printer.Print("=item C<*name*>\n"
		"\n"
		"A wrapper around the *message* message\n"
		"\n",
		"name", MessageClassName(descriptor),
		"message", descriptor->full_name());
}


void
PerlXSGenerator::GenerateDescriptorMethodPOD(const Descriptor* descriptor,
					     io::Printer& printer) const
{
  for ( int i = 0; i < descriptor->enum_type_count(); i++ ) {
    const EnumDescriptor * enum_descriptor = descriptor->enum_type(i);
    printer.Print("=head1 C<*name*> values\n"
		  "\n"
		  "=over 4\n"
		  "\n",
		  "name", EnumClassName(enum_descriptor));

    for ( int j = 0; j < enum_descriptor->value_count(); j++ ) {
      PODPrintEnumValue(enum_descriptor->value(j), printer);
    }

    printer.Print("\n"
		  "=back\n"
		  "\n");
  }

  for ( int i = 0; i < descriptor->nested_type_count(); i++ ) {
    GenerateDescriptorMethodPOD(descriptor->nested_type(i), printer);
  }

  // Constructor

  map<string, string> vars;

  vars["name"]  = MessageClassName(descriptor);
  vars["value"] = descriptor->name();

  printer.Print(vars,
		"=head1 *name* Constructor\n"
		"\n"
		"=over 4\n"
		"\n"
		"=item B<$*value* = *name*-E<gt>new>\n"
		"\n"
		"=back\n"
		"\n"
		"=head1 *name* Methods\n"
		"\n"
		"=over 4\n"
		"\n");

  // Common message methods

  printer.Print(vars,
		"=item B<$*value*2-E<gt>copy_from($*value*1)>\n"	
		"\n"
		"Copies the contents of C<*value*1> into C<*value*2>.\n"
		"\n"
		"=item B<$*value*2-E<gt>merge_from($*value*1)>\n"
		"\n"
		"Merges the contents of C<*value*1> into C<*value*2>.\n"
		"\n"
		"=item B<$*value*-E<gt>clear()>\n"
		"\n"
		"Clears the contents of C<*value*>.\n"
		"\n"
		"=item B<$init = $*value*-E<gt>is_initialized()>\n"
		"\n"
		"Returns 1 if C<*value*> has been initialized with data.\n"
		"\n"
		"=item B<$errstr = $*value*-E<gt>error_string()>\n"
		"\n"
		"Returns a comma-delimited string of initialization errors.\n"
		"\n"
		"=item B<$*value*-E<gt>discard_unknown_fields()>\n"
		"\n"
		"Discards unknown fields from C<*value*>.\n"
		"\n"
		"=item B<$dstr = $*value*-E<gt>debug_string()>\n"
		"\n"
		"Returns a string representation of C<*value*>.\n"
		"\n"
		"=item B<$dstr = $*value*-E<gt>short_debug_string()>\n"
		"\n"
		"Returns a short string representation of C<*value*>.\n"
		"\n"
		"=item B<$ok = $*value*-E<gt>unpack($string)>\n"
		"\n"
		"Attempts to parse C<string> into C<*value*>, returning 1 "
		"on success and 0 on failure.\n"
		"\n"
		"=item B<$string = $*value*-E<gt>pack()>\n"
		"\n"
		"Serializes C<*value*> into C<string>.\n"
		"\n"
		"=item B<$length = $*value*-E<gt>length()>\n"
		"\n"
		"Returns the serialized length of C<*value*>.\n"
		"\n");

  // Message field accessors

  for ( int i = 0; i < descriptor->field_count(); i++ ) {
    const FieldDescriptor* field = descriptor->field(i);

    vars["field"] = field->name();
    vars["type"]  = PODFieldTypeString(field);

    // has_blah or blah_size methods

    if ( field->is_repeated() ) {
      printer.Print(vars,
		    "=item B<$*field*_size = $*value*-E<gt>*field*_size()>\n"
		    "\n"
		    "Returns the number of C<*field*> elements present "
		    "in C<*value*>.\n"
		    "\n");
    } else {
      printer.Print(vars,
		    "=item B<$has_*field* = $*value*-E<gt>has_*field*()>\n"
		    "\n"
		    "Returns 1 if the C<*field*> element of C<*value*> "
		    "is set, 0 otherwise.\n"
		    "\n");
    }

    // clear_blah method

    printer.Print(vars,
		  "=item B<$*value*-E<gt>clear_*field*()>\n"
		  "\n"
		  "Clears the C<*field*> element(s) of C<*value*>.\n"
		  "\n");

    // getters

    if ( field->is_repeated() ) {
      printer.Print(vars,
		    "=item B<@*field*_list = $*value*-E<gt>*field*()>\n"
		    "\n"
		    "Returns all values of C<*field*> in an array.  Each "
		    "element of C<*field*_list> will be *type*.\n"
		    "\n"
		    "=item B<$*field*_elem = $*value*-E<gt>*field*($index)>\n"
		    "\n"
		    "Returns C<*field*> element C<index> from C<*value*>.  "
		    "C<*field*> will be *type*, unless C<index> is out of "
		    "range, in which case it will be undef.\n"
		    "\n");
    } else {
      printer.Print(vars,
		    "=item B<$*field* = $*value*-E<gt>*field*()>\n"
		    "\n"
		    "Returns C<*field*> from C<*value*>.  C<*field*> will "
		    "be *type*.\n"
		    "\n");
    }

    // setters

    if ( field->is_repeated() ) {
      printer.Print(vars,
		    "=item B<$*value*-E<gt>add_*field*($value)>\n"
		    "\n"
		    "Adds C<value> to the list of C<*field*> in C<*value*>.  "
		    "C<value> must be *type*.\n"
		    "\n");
    } else {
      printer.Print(vars,
		    "=item B<$*value*-E<gt>set_*field*($value)>\n"
		    "\n"
		    "Sets the value of C<*field*> in C<*value*> to "
		    "C<value>.  C<value> must be *type*.\n"
		    "\n");
    }
  }

  printer.Print("\n"
		"=back\n"
		"\n");
}


void
PerlXSGenerator::GenerateMessageTypemap(const Descriptor* descriptor,
					OutputDirectory* outdir) const
{
  string filename = descriptor->name() + ".typemap";
  scoped_ptr<io::ZeroCopyOutputStream> output(outdir->Open(filename));
  io::Printer printer(output.get(), '@'); // '@' works well in the typemap

  printer.Print("TYPEMAP\n"
		"\n");

  GenerateTypemapType(descriptor, printer);

  printer.Print("\n"
		"\n"
		"INPUT\n");

  GenerateTypemapInput(descriptor, printer);

  printer.Print("\n"
		"\n"
		"OUTPUT\n");

  GenerateTypemapOutput(descriptor, printer);

  printer.Print("\n");
}


void
PerlXSGenerator::GenerateEnumModule(const EnumDescriptor* enum_descriptor,
				    OutputDirectory* outdir) const
{
  string filename = CamelCase(enum_descriptor->name()) + ".pm";
  scoped_ptr<io::ZeroCopyOutputStream> output(outdir->Open(filename));
  io::Printer printer(output.get(), '*'); // '*' works well in the .pm file

  map<string, string> vars;

  vars["package"] = EnumClassName(enum_descriptor);
  vars["enum"]    = enum_descriptor->full_name();

  printer.Print(vars,
		"package *package*;\n"
		"\n"
		"use strict;\n"
		"use warnings;\n"
		"\n");

  // Each enum value is exported as a constant.

  for ( int i = 0; i < enum_descriptor->value_count(); i++ ) {
    ostringstream ost;
    ost << enum_descriptor->value(i)->number();
    printer.Print("use constant *value* => *number*;\n",
		  "value", enum_descriptor->value(i)->name(),
		  "number", ost.str());
  }

  printer.Print("\n"
		"1;\n"
		"\n"
		"__END__\n"
		"\n");

  // Now generate POD for the enum.

  printer.Print(vars,
		"=pod\n"
		"\n"
		"=head1 NAME\n"
		"\n"
		"*package* - Perl interface to *enum*\n"
		"\n"
		"=head1 SYNOPSIS\n"
		"\n"
		" use *package*;\n"
		"\n");

  for ( int i = 0; i < enum_descriptor->value_count(); i++ ) {
    printer.Print(" my $*value* = *package*::*value*;\n",
		  "package", vars["package"],
		  "value", enum_descriptor->value(i)->name());
  }

  printer.Print(vars,
		"\n"
		"=head1 DESCRIPTION\n"
		"\n"
		"*package* defines the following constants:\n"
		"\n"
		"=over 4\n"
		"\n");

  for ( int i = 0; i < enum_descriptor->value_count(); i++ ) {
    PODPrintEnumValue(enum_descriptor->value(i), printer);
  }
  
  printer.Print(vars,
		"\n"
		"=back\n"
		"\n"
		"=head1 AUTHOR\n"
		"\n"
		"Generated from *enum* by the protoc compiler.\n"
		"\n"
		"=head1 SEE ALSO\n"
		"\n"
		"http://code.google.com/p/protobuf\n"
		"\n"
		"=cut\n"
		"\n");
}


void
PerlXSGenerator::GenerateFileXSTypedefs(const FileDescriptor* file,
					io::Printer& printer,
					set<const Descriptor*>& seen) const
{
  for ( int i = 0; i < file->dependency_count(); i++ ) {
    GenerateFileXSTypedefs(file->dependency(i), printer, seen);
  }

  for ( int i = 0; i < file->message_type_count(); i++ ) {
    GenerateMessageXSTypedefs(file->message_type(i), printer, seen);
  }
}


void
PerlXSGenerator::GenerateMessageXSTypedefs(const Descriptor* descriptor,
					   io::Printer& printer,
					   set<const Descriptor*>& seen) const
{
  for ( int i = 0; i < descriptor->nested_type_count(); i++ ) {
    GenerateMessageXSTypedefs(descriptor->nested_type(i), printer, seen);
  }

  if ( seen.find(descriptor) == seen.end() ) {
    string cn = cpp::ClassName(descriptor, true);
    string un = StringReplace(cn, "::", "__", true);

    seen.insert(descriptor);
    printer.Print("typedef $classname$ $underscores$;\n",
		  "classname", cn,
		  "underscores", un);
  }
}


void
PerlXSGenerator::GenerateMessageXSFieldAccessors(const FieldDescriptor* field,
						 io::Printer& printer,
						 const string& classname) const
{
  map<string, string> vars;
  string cppname   = cpp::FieldName(field);
  string perlclass = MessageClassName(field->containing_type());
  bool   repeated  = field->is_repeated();

  vars["classname"] = classname;
  vars["cppname"]   = cppname;
  vars["perlname"]  = field->name();
  vars["perlclass"] = perlclass;

  FieldDescriptor::CppType fieldtype = field->cpp_type();

  if ( fieldtype == FieldDescriptor::CPPTYPE_MESSAGE ) {
    vars["fieldtype"] = cpp::ClassName(field->message_type(), true);
  }

  // For repeated fields, we need an index argument.

  if ( repeated ) {
    vars["i"] = "index";
  } else {
    vars["i"] = "";
  }

  // -------------------------------------------------------------------
  // First, the has_X method or X_size method.
  // -------------------------------------------------------------------

  if ( repeated ) {
    printer.Print(vars,
		  "I32\n"
		  "$classname$::$perlname$_size()\n"
		  "  CODE:\n"
		  "    RETVAL = THIS->$cppname$_size();\n"
		  "\n"
		  "  OUTPUT:\n"
		  "    RETVAL\n");
  } else {
    printer.Print(vars,
		  "I32\n"
		  "$classname$::has_$perlname$()\n"
		  "  CODE:\n"
		  "    RETVAL = THIS->has_$cppname$();\n"
		  "\n"
		  "  OUTPUT:\n"
		  "    RETVAL\n");
  }

  printer.Print("\n\n");

  // -------------------------------------------------------------------
  // Next, the "clear" method.
  // -------------------------------------------------------------------

  printer.Print(vars,
		"void\n"
		"$classname$::clear_$perlname$()\n"
		"  CODE:\n"
		"    THIS->clear_$cppname$();\n"
		"\n"
		"\n");

  // -------------------------------------------------------------------
  // Next, the "get" method.
  // -------------------------------------------------------------------

  // Repeated fields have an optional index argument.

  if ( repeated ) {
    printer.Print(vars,
		  "void\n"
		  "$classname$::$perlname$(...)\n");
  } else {
    printer.Print(vars,
		  "void\n"
		  "$classname$::$perlname$()\n");
  }

  printer.Print("  PREINIT:\n"
		"    SV * sv;\n");

  if ( repeated ) {
    printer.Print("    int index = 0;\n");
  }

  // We need to store 64-bit integers as strings in Perl.

  if ( fieldtype == FieldDescriptor::CPPTYPE_INT64 ||
       fieldtype == FieldDescriptor::CPPTYPE_UINT64 ) {
    printer.Print("    ostringstream ost;\n");
  }

  if ( fieldtype == FieldDescriptor::CPPTYPE_MESSAGE ) {
    printer.Print(vars,
		  "    $fieldtype$ * val = NULL;\n");
  }

  // We'll use PPCODE in either case, just to make this a little
  // simpler.

  printer.Print("\n"
		"  PPCODE:\n");

  // For repeated fields, we need to check the usage ourselves.

  if ( repeated ) {
    printer.Print(vars,
      "    if ( items == 2 ) {\n"
      "      index = SvIV(ST(1));\n"
      "    } else if ( items > 2 ) {\n"
      "      croak(\"Usage: $perlclass$::$perlname$(CLASS, [index])\");\n"
      "    }\n");
  }

  // There are three possibilities now:
  //
  // 1) The user wants a particular element of a repeated field.
  // 2) The user wants all elements of a repeated field.
  // 3) The user wants the value of a non-repeated field.

  if ( repeated ) {
    printer.Print(vars,
		  "    if ( THIS != NULL ) {\n"
		  "      if ( items == 1 ) {\n"
		  "        int count = THIS->$cppname$_size();\n"
		  "\n"
		  "        EXTEND(SP, count);\n"
		  "        for ( int index = 0; index < count; index++ ) {\n");
    PerlSVGetHelper(printer,vars,fieldtype,5);
    printer.Print(vars,
		  "          PUSHs(sv);\n"
		  "        }\n"
		  "      } else if ( index >= 0 &&\n"
		  "                  index < THIS->$cppname$_size() ) {\n"
		  "        EXTEND(SP,1);\n");
    PerlSVGetHelper(printer,vars,fieldtype,4);
    printer.Print("        PUSHs(sv);\n"
		  "      } else {\n"
		  "        EXTEND(SP,1);\n"
		  "        PUSHs(&PL_sv_undef);\n"
		  "      }\n"
		  "    }\n");
  } else {
    printer.Print("    if ( THIS != NULL ) {\n"
		  "      EXTEND(SP,1);\n");
    PerlSVGetHelper(printer,vars,fieldtype,3);
    printer.Print("      PUSHs(sv);\n"
		  "    }\n");
  }

  printer.Print("\n\n");

  // -------------------------------------------------------------------
  // Finally, the "set" method.
  // -------------------------------------------------------------------

  if ( repeated ) {
    printer.Print(vars,
		  "void\n"
		  "$classname$::add_$perlname$(val)\n");
  } else {
    printer.Print(vars,
		  "void\n"
		  "$classname$::set_$perlname$(val)\n");
  }

  // What is the incoming type?

  switch ( fieldtype ) {
  case FieldDescriptor::CPPTYPE_ENUM:
    vars["etype"] = cpp::ClassName(field->enum_type(), true);
    // Fall through
  case FieldDescriptor::CPPTYPE_INT32:
  case FieldDescriptor::CPPTYPE_BOOL:
    vars["value"] = "val";
    printer.Print("  IV val\n"
		  "\n"
		  "  CODE:\n");
    break;
  case FieldDescriptor::CPPTYPE_UINT32:
    vars["value"] = "val";
    printer.Print("  UV val\n"
		  "\n"
		  "  CODE:\n");
    break;
  case FieldDescriptor::CPPTYPE_FLOAT:
  case FieldDescriptor::CPPTYPE_DOUBLE:
    vars["value"] = "val";
    printer.Print("  NV val\n"
		  "\n"
		  "  CODE:\n");
    break;
  case FieldDescriptor::CPPTYPE_INT64:
    vars["value"] = "lval";
    printer.Print("  char *val\n"
		  "\n"
		  "  PREINIT:\n"
		  "    long long lval;\n"
		  "\n"
		  "  CODE:\n"
		  "    lval = strtoll((val) ? val : \"\", NULL, 0);\n");
    break;
  case FieldDescriptor::CPPTYPE_UINT64:
    vars["value"] = "lval";
    printer.Print("  char *val\n"
		  "\n"
		  "  PREINIT:\n"
		  "    unsigned long long lval;\n"
		  "\n"
		  "  CODE:\n"
		  "    lval = strtoull((val) ? val : \"\", NULL, 0);\n");
    break;
  case FieldDescriptor::CPPTYPE_STRING:
    vars["value"] = "sval";
    printer.Print("  char *val\n"
		  "\n"
		  "  PREINIT:\n"
		  "    string sval = (val) ? val : \"\";\n"
		  "\n"
		  "  CODE:\n");
    break;
  case FieldDescriptor::CPPTYPE_MESSAGE:
    printer.Print(vars,
		  "  $fieldtype$ * val\n"
		  "\n"
		  "  PREINIT:\n"
		  "    $fieldtype$ * mval;\n"
		  "\n"
		  "  CODE:\n");
    break;
  default:
    vars["value"] = "val";
    break;
  }

  if ( repeated ) {
    if ( fieldtype == FieldDescriptor::CPPTYPE_MESSAGE ) {
      printer.Print(vars,
		    "    if ( val != NULL ) {\n"
		    "      mval = THIS->add_$cppname$();\n"
		    "      mval->CopyFrom(*val);\n"
		    "    }\n");
    } else if ( fieldtype == FieldDescriptor::CPPTYPE_ENUM ) {
      printer.Print(vars,
		    "    if ( $etype$_IsValid(val) ) {\n"
		    "      THIS->add_$cppname$(($etype$)val);\n"
		    "    }\n");
    } else {
      printer.Print(vars,
		    "    THIS->add_$cppname$($value$);\n");
    }
  } else {
    if ( fieldtype == FieldDescriptor::CPPTYPE_MESSAGE ) {
      printer.Print(vars,
		    "    if ( val != NULL ) {\n"
		    "      mval = THIS->mutable_$cppname$();\n"
		    "      mval->CopyFrom(*val);\n"
		    "    }\n");
    } else if ( fieldtype == FieldDescriptor::CPPTYPE_ENUM ) {
      printer.Print(vars,
		    "    if ( $etype$_IsValid(val) ) {\n"
		    "      THIS->set_$cppname$(($etype$)val);\n"
		    "    }\n");
    } else {
      printer.Print(vars,
		    "    THIS->set_$cppname$($value$);\n");
    }
  }

  printer.Print("\n\n");
}


void
PerlXSGenerator::GenerateMessageXSCommonMethods(const Descriptor* descriptor,
						io::Printer& printer,
						const string& classname) const
{
  map<string, string> vars;

  vars["classname"] = classname;

  // CopyFrom

  printer.Print(vars,
		"void\n"
		"$classname$::copy_from(other)\n"
		"  $classname$ *other\n"
		"  CODE:\n"
		"    if ( THIS != NULL && other != NULL ) {\n"
		"      THIS->CopyFrom(*other);\n"
		"    }\n"
		"\n"
		"\n");

  // MergeFrom

  printer.Print(vars,
		"void\n"
		"$classname$::merge_from(other)\n"
		"  $classname$ *other\n"
		"  CODE:\n"
		"    if ( THIS != NULL && other != NULL ) {\n"
		"      THIS->MergeFrom(*other);\n"
		"    }\n"
		"\n"
		"\n");
  
  // Clear

  printer.Print(vars,
		"void\n"
		"$classname$::clear()\n"
		"  CODE:\n"
		"    if ( THIS != NULL ) {\n"
		"      THIS->Clear();\n"
		"    }\n"
		"\n"
		"\n");

  // IsInitialized

  printer.Print(vars,
		"int\n"
		"$classname$::is_initialized()\n"
		"  CODE:\n"
		"    if ( THIS != NULL ) {\n"
		"      RETVAL = THIS->IsInitialized();\n"
		"    } else {\n"
		"      RETVAL = 0;\n"
		"    }\n"
		"\n"
		"  OUTPUT:\n"
		"    RETVAL\n"
		"\n"
		"\n");

  // InitializationErrorString

  printer.Print(vars,
		"SV *\n"
		"$classname$::error_string()\n"
		"  PREINIT:\n"
		"    string estr;\n"
		"\n"
		"  CODE:\n"
		"    if ( THIS != NULL ) {\n"
		"      estr = THIS->InitializationErrorString();\n"
		"    }\n"
		"    RETVAL = newSVpv(estr.c_str(), estr.length());\n"
		"\n"
		"  OUTPUT:\n"
		"    RETVAL\n"
		"\n"
		"\n");

  // DiscardUnknownFields

  printer.Print(vars,
		"void\n"
		"$classname$::discard_unkown_fields()\n"
		"  CODE:\n"
		"    if ( THIS != NULL ) {\n"
		"      THIS->DiscardUnknownFields();\n"
		"    }\n"
		"\n"
		"\n");

  // DebugString

  printer.Print(vars,
		"SV *\n"
		"$classname$::debug_string()\n"
		"  PREINIT:\n"
		"    string dstr;\n"
		"\n"
		"  CODE:\n"
		"    if ( THIS != NULL ) {\n"
		"      dstr = THIS->DebugString();\n"
		"    }\n"
		"    RETVAL = newSVpv(dstr.c_str(), dstr.length());\n"
		"\n"
		"  OUTPUT:\n"
		"    RETVAL\n"
		"\n"
		"\n");

  // ShortDebugString

  printer.Print(vars,
		"SV *\n"
		"$classname$::short_debug_string()\n"
		"  PREINIT:\n"
		"    string dstr;\n"
		"\n"
		"  CODE:\n"
		"    if ( THIS != NULL ) {\n"
		"      dstr = THIS->ShortDebugString();\n"
		"    }\n"
		"    RETVAL = newSVpv(dstr.c_str(), dstr.length());\n"
		"\n"
		"  OUTPUT:\n"
		"    RETVAL\n"
		"\n"
		"\n");

  // ParseFromString (actually, we call ParseFromArray)

  printer.Print(vars,
		"int\n"
		"$classname$::unpack(arg)\n"
		"  SV *arg;\n"
		"  PREINIT:\n"
		"    STRLEN len;\n"
		"    char * str;\n"
		"\n"
		"  CODE:\n"
		"    if ( THIS != NULL ) {\n"
		"      str = SvPV(arg, len);\n"
		"      if ( str != NULL ) {\n"
		"        RETVAL = THIS->ParseFromArray(str, len);\n"
		"      } else {\n"
		"        RETVAL = 0;\n"
		"      }\n"
		"    } else {\n"
		"      RETVAL = 0;\n"
		"    }\n"
		"\n"
		"  OUTPUT:\n"
		"    RETVAL\n"
		"\n"
		"\n");

  // SerializeToString

  printer.Print(vars,
		"SV *\n"
		"$classname$::pack()\n"
		"  PREINIT:\n"
		"    string output;\n"
		"\n"
		"  CODE:\n"
		"    if ( THIS != NULL ) {\n"
		"      if ( THIS->SerializeToString(&output) == true ) {\n"
		"        RETVAL = newSVpv(output.c_str(), output.length());\n"
		"      } else {\n"
		"        RETVAL = Nullsv;\n"
		"      }\n"
		"    } else {\n"
		"      RETVAL = Nullsv;\n"
		"    }\n"
		"\n"
		"  OUTPUT:\n"
		"    RETVAL\n"
		"\n"
		"\n");

  // ByteSize

  printer.Print(vars,
		"int\n"
		"$classname$::length()\n"
		"  CODE:\n"
		"    if ( THIS != NULL ) {\n"
		"      RETVAL = THIS->ByteSize();\n"
		"    } else {\n"
		"      RETVAL = 0;\n"
		"    }\n"
		"\n"
		"  OUTPUT:\n"
		"    RETVAL\n"
		"\n"
		"\n");
}


void
PerlXSGenerator::GenerateMessageXSPackage(const Descriptor* descriptor,
					  io::Printer& printer) const
{
  for ( int i = 0; i < descriptor->nested_type_count(); i++ ) {
    GenerateMessageXSPackage(descriptor->nested_type(i), printer);
  }

  string cn = cpp::ClassName(descriptor, true);
  string mn = MessageModuleName(descriptor);
  string pn = MessageClassName(descriptor);

  printer.Print(
    "MODULE = $module$ PACKAGE = $package$\n"
    "PROTOTYPES: ENABLE\n\n\n",
    "module", mn,
    "package", pn
  );

  // BOOT (if there are enum types)

  int enum_count = descriptor->enum_type_count();

  if ( enum_count > 0 ) {
    printer.Print("BOOT:\n"
		  "  {\n"
		  "    HV * stash;\n\n");

    printer.Indent();
    printer.Indent();
    for ( int i = 0; i < enum_count; i++ ) {
      const EnumDescriptor * etype = descriptor->enum_type(i);
      int vcount = etype->value_count();

      printer.Print("stash = gv_stashpv(\"$package$::$name$\", TRUE);\n",
		    "package", pn,
		    "name", etype->name());
      for ( int j = 0; j < vcount; j++ ) {
	const EnumValueDescriptor * vdesc = etype->value(j);
	printer.Print(
	  "newCONSTSUB(stash, \"$name$\", newSViv($classname$::$name$));\n",
	  "classname", cn,
	  "name", vdesc->name()
	);
      }
    }
    printer.Outdent();
    printer.Outdent();
    printer.Print("  }\n\n\n");
  }

  // Constructor

  printer.Print(
    "$classname$ *\n"
    "$classname$::new ()\n"
    "  CODE:\n"
    "    if ( strcmp(CLASS,\"$package$\") ) {\n"
    "      croak(\"invalid class %s\",CLASS);\n"
    "    }\n"
    "    RETVAL = new $classname$;\n"
    "\n"
    "  OUTPUT:\n"
    "    RETVAL\n"
    "\n"
    "\n",
    "classname", cn,
    "package", pn
  );

  // Destructor

  printer.Print(
    "void\n"
    "$classname$::DESTROY()\n"
    "  CODE:\n"
    "    if ( THIS != NULL ) {\n"
    "      delete THIS;\n"
    "    }\n"
    "\n"
    "\n",
    "classname", cn
  );

  // Message methods (copy_from, parse_from, etc).

  GenerateMessageXSCommonMethods(descriptor, printer, cn);

  // Field accessors

  for ( int i = 0; i < descriptor->field_count(); i++ ) {
    GenerateMessageXSFieldAccessors(descriptor->field(i), printer, cn);
  }
}


void
PerlXSGenerator::GenerateTypemapType(const Descriptor* descriptor,
				     io::Printer& printer) const
{
  for ( int i = 0; i < descriptor->nested_type_count(); i++ ) {
    GenerateTypemapType(descriptor->nested_type(i), printer);
  }

  string cn = cpp::ClassName(descriptor, true);
  string tn = TypemapName(descriptor);

  printer.Print("@class_name@ *  @typemap_name@\n",
		"class_name", cn,
		"typemap_name", tn);
}


void
PerlXSGenerator::GenerateTypemapInput(const Descriptor* descriptor,
				      io::Printer& printer) const
{
  for ( int i = 0; i < descriptor->nested_type_count(); i++ ) {
    GenerateTypemapInput(descriptor->nested_type(i), printer);
    printer.Print("\n");
  }

  printer.Print(
    "@typemapname@\n"
    "  if ( sv_derived_from($arg,\\\"@classname@\\\") ) {\n"
    "    IV tmp = SvIV((SV *)SvRV($arg));\n"
    "    $var = INT2PTR($type, tmp);\n"
    "  } else {\n"
    "    croak(\\\"$var is not of type @classname@\\\");\n"
    "  }\n",
    "typemapname", TypemapName(descriptor),
    "classname", MessageClassName(descriptor)
  );
}


void
PerlXSGenerator::GenerateTypemapOutput(const Descriptor* descriptor,
				       io::Printer& printer) const
{
  for ( int i = 0; i < descriptor->nested_type_count(); i++ ) {
    GenerateTypemapOutput(descriptor->nested_type(i), printer);
    printer.Print("\n");
  }

  printer.Print(
    "@typemapname@\n"
    "  sv_setref_pv($arg, \\\"@classname@\\\", (void *)$var);\n",
    "typemapname", TypemapName(descriptor),
    "classname", MessageClassName(descriptor)
  );
}


}  // namespace perlxs
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
