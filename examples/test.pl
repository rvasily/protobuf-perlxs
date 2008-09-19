#!/usr/bin/perl

use strict;
use warnings;
use lib qw(blib/lib blib/arch/auto/Person);
use Time::HiRes qw(tv_interval gettimeofday);
use Person;

my $REPEAT = 10000;
my $id     = 123;
my $name   = 'Bob';
my $email  = 'bob@example.com';
my $person = Person->new;

my $start = [gettimeofday];

# Take the 'Person' protocol buffer through its life cycle.

foreach ( 1 .. $REPEAT ) {

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
}

my $elapsed = tv_interval($start);

print "[$id] $name <$email>: $REPEAT iterations in $elapsed seconds.\n";
