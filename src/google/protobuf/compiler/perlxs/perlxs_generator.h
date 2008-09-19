#ifndef GOOGLE_PROTOBUF_COMPILER_PERLXS_GENERATOR_H__
#define GOOGLE_PROTOBUF_COMPILER_PERLXS_GENERATOR_H__

#include <string>
#include <vector>
#include <map>
#include <set>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {

class Descriptor;
class EnumDescriptor;
class EnumValueDescriptor;
class FieldDescriptor;
class ServiceDescriptor;

namespace io { class Printer; }

namespace compiler {
namespace perlxs {

// CodeGenerator implementation for generated Perl/XS protocol buffer
// classes.  If you create your own protocol compiler binary and you
// want it to support Perl/XS output, you can do so by registering an
// instance of this CodeGenerator with the CommandLineInterface in
// your main() function.
class LIBPROTOC_EXPORT PerlXSGenerator : public CodeGenerator {
 public:
  PerlXSGenerator();
  virtual ~PerlXSGenerator();

  // implements CodeGenerator ----------------------------------------
  virtual bool Generate(const FileDescriptor* file,
			const string& parameter,
			OutputDirectory* output_directory,
			string* error) const;
  
 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(PerlXSGenerator);

 private:
  void GenerateXS(const FileDescriptor* file,
		  OutputDirectory* output_directory,
		  string& base) const;

  void GenerateMessageXS(const Descriptor* descriptor,
			 OutputDirectory* outdir) const;

  void GenerateMessageModule(const Descriptor* descriptor,
			     OutputDirectory* outdir) const;

  void GenerateDescriptorClassNamePOD(const Descriptor* descriptor,
				      io::Printer& printer) const;

  void GenerateDescriptorMethodPOD(const Descriptor* descriptor,
				   io::Printer& printer) const;

  void GenerateMessageTypemap(const Descriptor* descriptor,
			      OutputDirectory* outdir) const;

  void GenerateEnumModule(const EnumDescriptor* enum_descriptor,
			  OutputDirectory* outdir) const;
  
  void GenerateMessageXSFieldAccessors(const FieldDescriptor* field,
				       io::Printer& printer,
				       const string& classname) const;

  void GenerateMessageXSCommonMethods(const Descriptor* descriptor,
				      io::Printer& printer,
				      const string& classname) const;

  void GenerateFileXSTypedefs(const FileDescriptor* file,
			      io::Printer& printer,
			      set<const Descriptor*>& seen) const;

  void GenerateMessageXSTypedefs(const Descriptor* descriptor,
				 io::Printer& printer,
				 set<const Descriptor*>& seen) const;
  
  void GenerateMessageXSPackage(const Descriptor* descriptor,
				io::Printer& printer) const;

  void GenerateTypemapType(const Descriptor* descriptor,
			   io::Printer& printer) const;

  void GenerateTypemapInput(const Descriptor* descriptor,
			    io::Printer& printer) const;

  void GenerateTypemapOutput(const Descriptor* descriptor,
			    io::Printer& printer) const;
};
 
}  // namespace perlxs
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_PERLXS_GENERATOR_H__
