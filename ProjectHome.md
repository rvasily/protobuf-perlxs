# Protocol Buffers for Perl/XS #

This project contains a Perl/XS code generator for Google's
[Protocol Buffers](http://code.google.com/p/protobuf).  The generated code is an XS wrapper around the generated C++ classes, and includes POD documentation for each message type.

## Getting Started ##

Let's assume that you already know what Protocol Buffers are, and have downloaded, built, and installed the main protobuf package.  This gives you the core code generators (in particular, the C++ code generator, which we need), as well as various header files and the libprotoc library.  These are all dependencies for protobuf-perlxs, so you need to be sure that they're installed.

Next, download and install the protobuf-perlxs code:

```
 wget http://protobuf-perlxs.googlecode.com/files/protobuf-perlxs-1.1.tar.gz
 tar zxf protobuf-perlxs-1.1.tar.gz
 cd protobuf-perlxs-1.1
 ./configure --with-protobuf=/usr/local
 make
 sudo make install
```

In the sample instructions above, we assume that your protobuf headers and libraries are installed under /usr/local.  If you've installed them under /usr, you don't need the --with-protobuf argument.

Once you've installed protobuf-perlxs, you can invoke the `protoxs` executable, which gives you the ability to generate Perl/XS wrapper interfaces to protobuf messages.  To see a simple example, try this after you've installed `protoxs`:

```
 cd examples/simple
 perl Makefile.PL
 make
 make test
```

This example gives you a Perl interface to the following message definition:

```
 message Person {
   required int32 id = 1;
   required string name = 2;
   optional string email = 3;
 }
```

You can read the generated POD documentation for the Person message in the usual way:

```
 perldoc Person
```

This documentation shows you what constructors and methods are available for that message type.  Typical usage for the simple example we're using here might be something like the following:

```
    #!/usr/bin/perl

    use strict;
    use warnings;
    use Person;

    my $person = Person->new;

    # Set each of the fields.

    $person->set_id($id);
    $person->set_name($name);
    $person->set_email($email);

    # Serialize the object to a string.

    my $packed = $person->pack();

    # Clear the object contents.

    $person->clear();

    # Deserialize the object from the string.

    $person->unpack($packed);

    # Get each of the fields.

    $id = $person->id();
    $name = $person->name();
    $email = $person->email();
```

This is a contrived example.  However, the performance is quite good.  It takes only 0.2 seconds for 10,000 iterations through the Perl code shown above (20 microseconds per iteration) on a 3.2 GHz Pentium 4 CPU, whereas the same number of Storable freeze/thaw iterations on the same data takes 0.7 seconds.  Storable is the fastest general-purpose Perl serialization module, and it is outperformed here by a considerable margin.

## How you can use it ##

Really, this is up to you.  You have to decide whether you are willing to tolerate the extra work that is required in order to get your Perl/XS interfaces up and running.  If you prefer to work in a pure-Perl environment without any XS code, you may want to have a look at the [protobuf-perl](http://code.google.com/p/protobuf-perl) project.

If you want the best performance possible and are willing to jump through some extra hoops to get it, read on.  Depending on the sophistication of your usage of Protocol Buffers, you have a few options:

### Entry level ###

You can generate and compile Perl/XS and C++ code together from protobuf message definitions.  This is the approach taken in the example that is supplied with the source code.  A single call to `protoxs` on a single message definition generates all of the source files that are needed, and a fairly simple Makefile.PL can be used to build the Perl package.  In fact, with the `person.proto` message definition shown above, the corresponding Makefile.PL can be reproduced here in its entirety:

```
use ExtUtils::MakeMaker;

$CC = 'g++';

# Generate the C++, XS, and Perl files that we need.

my $protoxs = "protoxs --cpp_out=. --out=. person.proto";

print "Generating C++ and Perl/XS sources...\n";
print "$protoxs\n";

`$protoxs`;

# Now write the Makefile.

WriteMakefile(
              'NAME'          => 'Person',
              'VERSION_FROM'  => 'Person.pm',
              'OPTIMIZE'      => '-O2 -Wall',
              'CC'            => $CC,
              'LD'            => '$(CC)',
              'CCFLAGS'       => '-fno-strict-aliasing',
              'OBJECT'        => 'person.pb.o Person.o',
              'TYPEMAPS'      => ['Person.typemap'],
              'INC'           =>  " -I/usr/local/include",
              'LIBS'          => [" -L/usr/local/lib -lprotobuf"],
              'XSOPT'         => '-C++',
              clean           => { FILES => "Person.* person.pb.* lib" }
             );

```

### Next level ###

While the entry level approach might get you started, you may find at a certain point that your messages start having dependencies on other messages (imports).  If you have multiple "top-level" message types that share dependency on one or more other message types, the entry level compilation method will result in duplicate copies of the dependent code.  This is not a fatal problem, as the Perl/XS modules are built into dynamic objects that are dlopened by the Perl interpreter.  You should not run into any symbol conflicts.  However, you may wish instead to build a shared library out of the generated C++ code, and link your Perl/XS wrapper code with this shared library.  In this case, you will need to generate and compile the C++ shared object first, then generate, compile, and link the Perl/XS wrapper.  Since I myself am still at the Entry level with Protocol Buffers, I do not yet have an example of this "Next level" approach.

## What works now ##

As of now, all of the data types are supported, including enums and embedded messages.  Imports are also supported.

## What doesn't work yet ##

Extensions are not yet supported, nor is the RPC interface.  I plan to add support for Extensions next.  After that, I will look at the RPC interface.

## Other things that need to be added ##

We have been assigned a field ID of 1001 for [custom options](http://code.google.com/apis/protocolbuffers/docs/proto.html#options) in protobuf-perlxs.  This will make it possible to specify file-level, message-level, and even field-level options that are recognized by protobuf-perlxs.  As of now, we have an immediate need to use these custom options, since more sophisticated package and message layouts in Perl/XS will require the suppression of certain .pm files from the output, the generation of others, and so on.  There will undoubtedly be other reasons to use custom options to customize the behavior of protobuf-perlxs on the .proto files that it processes.
