#include <google/protobuf/compiler/command_line_interface.h>
#include <google/protobuf/compiler/cpp/cpp_generator.h>
#include <google/protobuf/compiler/perlxs/perlxs_generator.h>


int main(int argc, char* argv[]) {
  google::protobuf::compiler::CommandLineInterface cli;

  // Proto2 C++ (for convenience, so the user doesn't need to call
  // protoc separately)

  google::protobuf::compiler::cpp::CppGenerator cpp_generator;
  cli.RegisterGenerator("--cpp_out", &cpp_generator,
                        "Generate C++ header and source.");

  // Proto2 Perl/XS
  google::protobuf::compiler::perlxs::PerlXSGenerator perlxs_generator;
  cli.RegisterGenerator("--out", &perlxs_generator,
                        "Generate Perl/XS source files.");
  
  return cli.Run(argc, argv);
}
