# Directories and Paths -------------------------------------

setenv BLDHOME `pwd`

if ($?LD_LIBRARY_PATH == 1) then
    set pathfind = `echo $LD_LIBRARY_PATH | grep $BLDHOME/lib`
    if ($#pathfind == 0) then
        setenv LD_LIBRARY_PATH "$BLDHOME/lib":$LD_LIBRARY_PATH
    endif
else
    setenv LD_LIBRARY_PATH "$BLDHOME/lib"
endif

set pathfind = `echo $path | grep $BLDHOME/bin`
if ($#pathfind == 0) then
    set path = (. $BLDHOME/bin $path)
endif

