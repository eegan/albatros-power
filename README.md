# albatros-power
(Arduino) power logging and sleep control for ALBATROS

This project can embed its current git commit hash in the code

To make this work:
* naturally, git has to be installed so it works from the command line
* install perl, so that perl works from the command line (or substitute the exact path to the command in the recipe line below)
* add the following line to the Arduino platform.txt file
   recipe.hooks.sketch.prebuild.1.pattern=perl {build.source.path}\githash.pl "{build.source.path}" "{build.path}"

This works by overwriting the temporary copy of githash.h with a #define for the macro GITHASH equal to the current git commit hash.

If the recipe hook is not installed, the GITHASH macro will be set to "(recipe hook not installed, see README.md)"

If the Arduino folder is not pristine, the qualifier " (modified)" is appended to the git hash string.