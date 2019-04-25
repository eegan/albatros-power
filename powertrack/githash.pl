# Script to create githash.h for Arduino build, to be embedded in the executable.
# To use:
# Place this script in the Arduino build source folder
# Add the following line to the Arduino platform.txt:
#    recipe.hooks.sketch.prebuild.1.pattern=perl {build.source.path}\githash.pl "{build.source.path}"

use strict;

# used for debugging
# use Tk;
# my $mw = new MainWindow;

my $bsp = shift;  # build source path (this is the folder where the source is kept, from which we need to run git)
my $bp = shift;   # build path (this is the temporary folder where we will overwrite githash.h)

chdir $bsp;
# this is the git incantation to get the currently checked out git commit hash
my $gh = `git rev-parse HEAD`;
chomp $gh;

# This git commmand returns nonzero if there are uncommitted changes. 
# This does not include untracked files.
my $dirty = system('git diff-index --quiet HEAD --');  
$gh .= ' (modified)' if $dirty;

chdir "$bp/sketch";  # descend into the folder that contains the copy of the source files

open (my $fh, '>', 'githash.h') or die "Could not open file";
print $fh "#define GITHASH \"$gh\"" or die "Could not write line";
close $fh or die "Could not close file";

# $mw -> messageBox(-message=>$bsp);
# $mw -> messageBox(-message=>$bp);