# Directories and Paths -------------------------------------

BLDHOME=`pwd`
CDPATH=.:$BLDHOME:$BLDHOME/examples

if [ $LD_LIBRARY_PATH ]; then
    pathfind=`echo $LD_LIBRARY_PATH | grep $BLDHOME/lib`
    if [ $pathfind ]; then : ; else
        LD_LIBRARY_PATH="$BLDHOME/lib":$LD_LIBRARY_PATH
    fi
else
    LD_LIBRARY_PATH="$BLDHOME/lib"
fi

pathfind=`echo $PATH | grep $BLDHOME/bin`
if [ $pathfind ]; then : ; else
    PATH=$BLDHOME/bin:$PATH;
fi

# Clean up -------------------------------------------------

unset pathfind 
export BLDHOME CDPATH LD_LIBRARY_PATH PATH

