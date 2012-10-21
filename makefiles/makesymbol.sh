#!/usr/bin/env bash

# Time-stamp: <2012-08-16 09:41:35 Thursday by ubuntu>
# encoding: utf-8-unix (alias: mule-utf-8-unix)
# @version 1.0
# @author liangyaozhan
# usage: bash makesymbol.sh nm /path/to/  /path/to/$(TARGET).elf sym.c

#srcfile="`ls $2/*.elf`"
srcfile=$3
symbolc=$4
NM=$1
funcfile=$2/func.lst
funcfiletmp=$2/tmp.lst
objsfile_b=$2/objs_b.lst
objsfile_d=$2/objs_d.lst
objsfile_r=$2/objs_r.lst
objsfile_s=$2/objs_s.lst
objsfile_c=$2/objs_c.lst
objsfile_w=$2/objs_w.lst
objsfile_v=$2/objs_v.lst

BASE="`dirname $0`"

rm -f $funcfile $objfile

for i in $srcfile; do 
    # function
    $NM -g $i | sed -n 's/.*\ T\ \(.*\)/\1/gp' | sed '/^_.*/d' >>$funcfile;
    # obj, remove __sylixos_version
    $NM -g $i | sed -n 's/.*\ [B]\ \(.*\)/\1/gp'|sed -e '/^_.*/d' >>$objsfile_b;
    $NM -g $i | sed -n 's/.*\ [D]\ \(.*\)/\1/gp'|sed -e '/^_.*/d' >>$objsfile_d;
    $NM -g $i | sed -n 's/.*\ [R]\ \(.*\)/\1/gp'|sed -e '/^_.*/d' >>$objsfile_r;
    $NM -g $i | sed -n 's/.*\ [S]\ \(.*\)/\1/gp'|sed -e '/^_.*/d' >>$objsfile_s;
    $NM -g $i | sed -n 's/.*\ [C]\ \(.*\)/\1/gp'|sed -e '/^_.*/d' >>$objsfile_c;
    $NM -g $i | sed -n 's/.*\ [W]\ \(.*\)/\1/gp'|sed -e '/^_.*/d' >>$objsfile_w;
    $NM -g $i | sed -n 's/.*\ [V]\ \(.*\)/\1/gp'|sed -e '/^_.*/d' >>$objsfile_v;
done

# add extra functions
if [ -f $BASE/extra-funcs ]; then
    all_extra_symbols="`cat $BASE/extra-funcs |sed -e '/.*#.*/d' -e '/^[ \t]*$/d' -e 's/[ \t]*\(.*\)[ \t]*.*/\1/g'`";
    for i in ${all_extra_symbols}; do
        rm -f $funcfiletmp;
        mv $funcfile $funcfiletmp;
        cat $funcfiletmp |sed -e "/$i/d" >$funcfile;
    done;
    echo "$all_extra_symbols" >>$funcfile;
fi

cat << EOF >$symbolc

#define NO_API_DECL 1
#include "symbol.h"

#undef malloc
#undef free

#ifndef SYM_TABLE_SIZE
#define SYM_TABLE_SIZE   `wc $funcfile $objsfile_b $objsfile_d $objsfile_r $objsfile_s $objsfile_c $objsfile_w $objsfile_v -l | sed -n 's/\ *\([0-9]*\)\ total/\1/pg'`
#endif

#define SYMBOL_ITEM_FUNC(sym)   {sym, SYM_TEXT, #sym },
#define SYMBOL_ITEM_OBJ(sym)   {&sym, SYM_DATA, #sym },

EOF

# extern int xxxx();
sed 's/\(.*\)/extern int \1\(\)\;/g' $funcfile >>$symbolc

# extern int xxxx;
sed 's/\(.*\)/extern int \1\;/g' $objsfile_b >>$symbolc
sed 's/\(.*\)/extern int \1\;/g' $objsfile_d >>$symbolc
sed 's/\(.*\)/extern int \1\;/g' $objsfile_r >>$symbolc
sed 's/\(.*\)/extern int \1\;/g' $objsfile_s >>$symbolc
sed 's/\(.*\)/extern int \1\;/g' $objsfile_c >>$symbolc
sed 's/\(.*\)/extern int \1\;/g' $objsfile_w >>$symbolc
sed 's/\(.*\)/extern int \1\;/g' $objsfile_v >>$symbolc


cat << EOF >>$symbolc


static sym_t __g_sym_static[] = {
EOF

    sed 's/\(.*\)/    SYMBOL_ITEM_FUNC\(\1\)/g' $funcfile   >>$symbolc;
    sed 's/\(.*\)/    SYMBOL_ITEM_OBJ\(\1\)/g' $objsfile_b   >>$symbolc;
    sed 's/\(.*\)/    SYMBOL_ITEM_OBJ\(\1\)/g' $objsfile_d   >>$symbolc;
    sed 's/\(.*\)/    SYMBOL_ITEM_OBJ\(\1\)/g' $objsfile_r   >>$symbolc;
    sed 's/\(.*\)/    SYMBOL_ITEM_OBJ\(\1\)/g' $objsfile_s   >>$symbolc;
    sed 's/\(.*\)/    SYMBOL_ITEM_OBJ\(\1\)/g' $objsfile_c   >>$symbolc;
    sed 's/\(.*\)/    SYMBOL_ITEM_OBJ\(\1\)/g' $objsfile_w   >>$symbolc;
    sed 's/\(.*\)/    SYMBOL_ITEM_OBJ\(\1\)/g' $objsfile_v   >>$symbolc;


cat << EOF >>$symbolc
};

int __static_sym_get( sym_t **ppsym, int *pcount )
{
    *ppsym  = __g_sym_static;
    *pcount = SYM_TABLE_SIZE;
    return -!SYM_TABLE_SIZE;
}

EOF

#
rm -f $funcfile $objsfile_b $objsfile_d $objsfile_r
rm -f $objsfile_s $objsfile_c $objsfile_w $objsfile_v
rm -f $funcfiletmp

