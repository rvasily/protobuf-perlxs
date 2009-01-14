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


static void
SetupDepthVars(map<string, string>& vars, int depth)
{
  ostringstream ost_pdepth;
  ostringstream ost_depth;
  ostringstream ost_ndepth;

  ost_pdepth << depth;
  ost_depth  << depth + 1;
  ost_ndepth << depth + 2;

  vars["pdepth"] = ost_pdepth.str();
  vars["depth"]  = ost_depth.str();
  vars["ndepth"] = ost_ndepth.str();
}


static void
StartFieldToHashref(const FieldDescriptor * field,
		    io::Printer& printer,
		    map<string, string>& vars,
		    int depth)
{
  SetupDepthVars(vars, depth);

  if ( field->is_repeated() ) {
    vars["i"] = "i" + vars["pdepth"];
    printer.Print(vars,
		  "if ( msg$pdepth$->$cppname$_size() > 0 ) {\n");
    printer.Indent();
    printer.Print(vars,
		  "AV * av$pdepth$ = newAV();\n"
		  "SV * sv$pdepth$ = newRV_noinc((SV *)av$pdepth$);\n"
		  "\n"
		  "for ( int $i$ = 0; "
		  "$i$ < msg$pdepth$->$cppname$_size(); $i$++ ) {\n");
  } else {
    vars["i"] = "";
    printer.Print(vars,
		  "if ( msg$pdepth$->has_$cppname$() ) {\n");
  }
  printer.Indent();
}


static void
FieldToHashrefHelper(io::Printer& printer,
		     map<string, string>& vars,
		     const FieldDescriptor* field)
{
  vars["msg"] = "msg" + vars["pdepth"];
  if ( field->is_repeated() ) {
    vars["sv"] = "sv" + vars["depth"];
  } else {
    vars["sv"] = "sv" + vars["pdepth"];
  }

  switch ( field->cpp_type() ) {
  case FieldDescriptor::CPPTYPE_INT32:
  case FieldDescriptor::CPPTYPE_BOOL:
  case FieldDescriptor::CPPTYPE_ENUM:
    printer.Print(vars,
		  "SV * $sv$ = newSViv($msg$->$cppname$($i$));\n");
    break;
  case FieldDescriptor::CPPTYPE_UINT32:
    printer.Print(vars,
		  "SV * $sv$ = newSVuv($msg$->$cppname$($i$));\n");
    break;
  case FieldDescriptor::CPPTYPE_FLOAT:
  case FieldDescriptor::CPPTYPE_DOUBLE:
    printer.Print(vars,
		  "SV * $sv$ = newSVnv($msg$->$cppname$($i$));\n");
    break;
  case FieldDescriptor::CPPTYPE_INT64:
  case FieldDescriptor::CPPTYPE_UINT64:
    printer.Print(vars,
		  "ostringstream ost$pdepth$;\n"
		  "\n"
		  "ost$pdepth$ << $msg$->$cppname$($i$);\n"
		  "SV * $sv$ = newSVpv(ost$pdepth$.str().c_str(),"
		  " ost$pdepth$.str().length());\n");
    break;
  case FieldDescriptor::CPPTYPE_STRING:
  case FieldDescriptor::CPPTYPE_MESSAGE:
  default:
    printer.Print(vars,
		  "SV * $sv$ = newSVpv($msg$->"
		  "$cppname$($i$).c_str(), $msg$->"
		  "$cppname$($i$).length());\n");
    break;
  }
}


static void
EndFieldToHashref(const FieldDescriptor * field,
		  io::Printer& printer,
		  map<string, string>& vars,
		  int depth)
{
  vars["cppname"] = cpp::FieldName(field);
  SetupDepthVars(vars, depth);

  if ( field->is_repeated() ) {
    printer.Print(vars,
		  "av_push(av$pdepth$, sv$depth$);\n");
    printer.Outdent();
    printer.Print(vars,
		  "}\n"
		  "hv_store(hv$pdepth$, \"$cppname$\", "
		  "sizeof(\"$cppname$\") - 1, sv$pdepth$, 0);\n");
  } else {
    if ( field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ) {
      printer.Print(vars,
		    "hv_store(hv$pdepth$, \"$cppname$\", "
		    "sizeof(\"$cppname$\") - 1, sv$depth$, 0);\n");
    } else {
      printer.Print(vars,
		    "hv_store(hv$pdepth$, \"$cppname$\", "
		    "sizeof(\"$cppname$\") - 1, sv$pdepth$, 0);\n");
    }
  }
  printer.Outdent();
  printer.Print("}\n");
}
  

void MessageToHashref(const Descriptor * descriptor,
		      io::Printer& printer,
		      map<string, string>& vars,
		      int depth)
{
  int i;

  // Iterate the fields

  for ( i = 0; i < descriptor->field_count(); i++ ) {
    const FieldDescriptor* field = descriptor->field(i);
    FieldDescriptor::CppType fieldtype = field->cpp_type();

    vars["field"] = field->name();
    vars["cppname"] = cpp::FieldName(field);
    
    StartFieldToHashref(field, printer, vars, depth);

    if ( fieldtype == FieldDescriptor::CPPTYPE_MESSAGE ) {
      vars["fieldtype"] = cpp::ClassName(field->message_type(), true);
      printer.Print(vars,
		    "$fieldtype$ * msg$ndepth$ = msg$pdepth$->"
		    "mutable_$cppname$($i$);\n"
		    "HV * hv$ndepth$ = newHV();\n"
		    "SV * sv$depth$ = newRV_noinc((SV *)hv$ndepth$);\n"
		    "\n");
      MessageToHashref(field->message_type(), printer, vars, depth + 2);
      SetupDepthVars(vars, depth);
    } else {
      FieldToHashrefHelper(printer, vars, field);
    }

    EndFieldToHashref(field, printer, vars, depth);
  }
}


static void
FieldFromHashrefHelper(io::Printer& printer,
		       map<string, string>& vars,
		       const FieldDescriptor * field)
{
  vars["msg"] = "msg" + vars["pdepth"];
  vars["var"] = "*sv" + vars["depth"];

  if ( field->is_repeated() ) {
    vars["do"]  = "add";
  } else {
    vars["do"]  = "set";
  }

  switch ( field->cpp_type() ) {
  case FieldDescriptor::CPPTYPE_INT32:
  case FieldDescriptor::CPPTYPE_BOOL:
    printer.Print(vars,
		  "$msg$->$do$_$cppname$(SvIV($var$));\n");
    break;
  case FieldDescriptor::CPPTYPE_ENUM:
    vars["type"] = EnumClassName(field->enum_type());
    printer.Print(vars,
		  "$msg$->$do$_$cppname$"
		  "(($type$)SvIV($var$));\n");
    break;
  case FieldDescriptor::CPPTYPE_UINT32:
    printer.Print(vars,
		  "$msg$->$do$_$cppname$(SvUV($var$));\n");
    break;
  case FieldDescriptor::CPPTYPE_FLOAT:
  case FieldDescriptor::CPPTYPE_DOUBLE:
    printer.Print(vars,
		  "$msg$->$do$_$cppname$(SvNV($var$));\n");
    break;
  case FieldDescriptor::CPPTYPE_INT64:
    printer.Print(vars,
		  "int64_t iv$pdepth$ = "
		  "strtoll(SvPV_nolen($var$), NULL, 0);\n"
		  "\n"
		  "$msg$->$do$_$cppname$(iv$pdepth$);\n");
    break;
  case FieldDescriptor::CPPTYPE_UINT64:
    printer.Print(vars,
		  "uint64_t uv$pdepth$ = "
		  "strtoull(SvPV_nolen($var$), NULL, 0);\n"
		  "\n"
		  "$msg$->$do$_$cppname$(uv$pdepth$);\n");
    break;
  case FieldDescriptor::CPPTYPE_STRING:
    printer.Print(vars,
		  "$msg$->$do$_$cppname$(SvPV_nolen($var$));\n");
    break;
  case FieldDescriptor::CPPTYPE_MESSAGE:
    /* Should never get here. */
  default:
    break;
  }
}


void MessageFromHashref(const Descriptor * descriptor,
			io::Printer& printer,
			map<string, string>& vars,
			int depth)
{
  int i;

  SetupDepthVars(vars, depth);

  printer.Print(vars,
		"if ( SvROK(sv$pdepth$) && "
		"SvTYPE(SvRV(sv$pdepth$)) == SVt_PVHV ) {\n");
  printer.Indent();
  printer.Print(vars,
		"HV *  hv$pdepth$ = (HV *)SvRV(sv$pdepth$);\n"
		"SV ** sv$depth$;\n"
		"\n");

  // Iterate the fields

  for ( i = 0; i < descriptor->field_count(); i++ ) {
    const FieldDescriptor* field = descriptor->field(i);

    vars["field"]   = field->name();
    vars["cppname"] = cpp::FieldName(field);

    if ( field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ) {
      vars["fieldtype"] = cpp::ClassName(field->message_type(), true);
    }

    printer.Print(vars,
		  "if ( (sv$depth$ = hv_fetch(hv$pdepth$, "
		  "\"$field$\", sizeof(\"$field$\") - 1, 0)) != NULL ) {\n");

    printer.Indent();

    if ( field->is_repeated() ) {
      printer.Print(vars,
		    "if ( SvROK(*sv$depth$) && "
		    "SvTYPE(SvRV(*sv$depth$)) == SVt_PVAV ) {\n");
      printer.Indent();
      printer.Print(vars,
		    "AV * av$depth$ = (AV *)SvRV(*sv$depth$);\n"
		    "\n"
		    "for ( int i$depth$ = 0; "
		    "i$depth$ < av_len(av$depth$); i$depth$++ ) {\n");
      printer.Indent();

      if ( field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ) {
	printer.Print(vars,
		      "$fieldtype$ * msg$ndepth$ = "
		      "msg$pdepth$->add_$cppname$();\n"
		      "SV ** sv$depth$;\n"
		      "SV *  sv$ndepth$;\n"
		      "\n"
		      "if ( (sv$depth$ = "
		      "av_fetch(av$depth$, i$depth$, 0)) != NULL ) {\n"
		      "  sv$ndepth$ = *sv$depth$;\n");
      } else {
	printer.Print(vars,
		      "SV ** sv$depth$;\n"
		      "\n"
		      "if ( (sv$depth$ = "
		      "av_fetch(av$depth$, i$depth$, 0)) != NULL ) {\n");
      }
      printer.Indent();
    } else {
      if ( field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ) {
	printer.Print(vars,
		      "$fieldtype$ * msg$ndepth$ = "
		      "msg$pdepth$->mutable_$cppname$();\n"
		      "SV * sv$ndepth$ = *sv$depth$;\n"
		      "\n");
      }
    }

    if ( field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE ) {
      MessageFromHashref(field->message_type(), printer, vars, depth + 2);
      SetupDepthVars(vars, depth);
    } else {
      FieldFromHashrefHelper(printer, vars, field);
    }

    if ( field->is_repeated() ) {
      printer.Outdent();
      printer.Print("}\n");
      printer.Outdent();
      printer.Print("}\n");
      printer.Outdent();
      printer.Print("}\n");
    }

    printer.Outdent();
    printer.Print("}\n");
  }

  printer.Outdent();
  printer.Print("}\n");
}


}  // namespace perlxs
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
