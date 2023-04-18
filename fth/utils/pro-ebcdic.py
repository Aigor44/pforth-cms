#!/usr/bin/env python

import sys
import string

def name_to_string( name ) :
    for c in name :
        if c not in string.printable :
            return ''
    return '  // %s' % name

def dump_names( names ) :
    i = t = 0
    n = len( names )
    while i < n :
        if t == 0 :
            a, b, c, d = names[i:i+4]
            v = (a<<24) + (b<<16) + (c<<8) + d
            print '    /* prev. name  : 0x%08X : 0x%08X */ 0x%02X,0x%02X,0x%02X,0x%02X,\n' % ( i, v, a, b, c, d ),
            t = 1
            i = i + 4
        elif t == 1 :
            a, b, c, d = names[i:i+4]
            v = (a<<24) + (b<<16) + (c<<8) + d
            print '    /* code pointer: 0x%08X : 0x%08X */ 0x%02X,0x%02X,0x%02X,0x%02X,\n' % ( i, v, a, b, c, d ),
            t = 2
            i = i + 4
        else :
            nlen = namelen = names[i] & 0x0F
            while ( namelen + 1 ) % 4 > 0 :
                namelen += 1
            print '    /* name length : 0x%08X : %10d */ 0x%02X,%s' % ( i, nlen, names[i], name_to_string( ''.join( map( chr, names[i+1:i+1+nlen] ) ) ) )
            print '                                                ',
            for j in range( namelen ) :
                try :
                    sys.stdout.write( '0x%02X,' % names[i+j+1] )
                except IndexError :
                    pass
            print ''
            t = 0
            i = i + namelen + 1

if __name__ == '__main__' :
    STATE = 'initial'  # possible values are 'initial', 'in_names' and 'final'
    NAMES = []
    for line in open( './pfdicdat.h' ).readlines() :
        if STATE == 'initial' :
            print line,
            if line.find( 'static const uint8 MinDicNames' ) == 0 :
                STATE = 'in_names'
                continue
        if STATE == 'in_names' :
            if line.find( '};' ) == 0 :
                STATE = 'final'
                dump_names( map( lambda x : int( x, 16 ), NAMES ) )
                print line,
                continue
            if line.find( '/* ' ) == 0 :
                NAMES.extend( line[18:].split( ',' )[:-1] )
            else :
                NAMES.extend( line.split( ',' )[:-1] )
        if STATE == 'final' :
            print line,

